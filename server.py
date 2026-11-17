import argparse
import functools
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import threading
import webbrowser
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer


ROOT = Path(__file__).resolve().parent
FIELDS = ("op", "value", "key", "index", "size", "priority")


def check(op, expected, throws=None, **arguments):
    return dict(op=op, expect=expected, throws=throws, **arguments)


def ready_checks(structure, value_type):
    a, b, c = {"int": ("2", "4", "6"), "double": ("1.25", "2.5", "3.75"),
               "string": ("apple", "pear", "plum")}[value_type]
    missing = "std::out_of_range"
    invalid = "std::invalid_argument"
    if structure == "array":
        return [check("resize", "3", size="3"), check("set", a, index="0", value=a),
                check("set", b, index="1", value=b), check("get", a, index="0"),
                check("resize", "5", size="5"), check("get", b, index="1"),
                check("get", "" if value_type == "string" else "0", index="4"),
                check("copy", a, value=c), check("resize", "1", size="1"),
                check("get", missing, missing, index="1"),
                check("resize", invalid, invalid, size="-1"), check("get", a, index="0"),
                check("resize", "0", size="0"), check("get", missing, missing, index="0")]
    if structure == "tree":
        return [check("insert", "true", value=a), check("insert", "true", value=b),
                check("insert", "true", value=c), check("height", "2"),
                check("insert", "false", value=b), check("contains", "true", value=b),
                check("inorder", f"{a} {b} {c}"), check("threads", f"{a} {b} {c}"),
                check("reverse", f"{c} {b} {a}"), check("remove", "true", value=b),
                check("contains", "false", value=b), check("threads", f"{a} {c}"),
                check("insert", "true", value=b), check("pop", c), check("top", b),
                check("remove", "true", value=a), check("remove", "true", value=b),
                check("top", missing, missing), check("threads", "")]
    if structure == "hash":
        keys = [f"key{i}" if value_type == "string" else str(100 + i * 8) for i in range(8)]
        return [check("add", b, key=a, value=b), check("get", b, key=a),
                check("update", c, key=a, value=c), check("get", c, key=a),
                check("contains", "true", key=a), check("add", invalid, invalid, key=a, value=a),
                *[check("add", b, key=key, value=b) for key in keys],
                check("capacity", "16"), check("get", b, key=keys[0]),
                *[check("remove", "удалено", key=key) for key in keys],
                check("capacity", "8"), check("get", c, key=a), check("remove", "удалено", key=a),
                check("get", missing, missing, key=a), check("remove", missing, missing, key=a)]
    if structure == "queue":
        return [check("top", missing, missing), check("push", "добавлено", value=a, priority="1"),
                check("push", "добавлено", value=b, priority="3"),
                check("push", "добавлено", value=c, priority="3"), check("top", c),
                check("pop", c), check("pop", b), check("pop", a),
                check("pop", missing, missing), check("empty", "true")]
    empty = "std::logic_error"
    low, high = (a, c) if structure == "minheap" else (c, a)
    return [check("top", empty, empty), check("push", "добавлено", value=c),
            check("push", "добавлено", value=a), check("push", "добавлено", value=b),
            check("top", low), check("pop", low), check("pop", b), check("pop", high),
            check("pop", empty, empty), check("push", "добавлено", value="-2.5"),
            check("push", "добавлено", value="-2.5"), check("pop", "-2.5"),
            check("pop", "-2.5"), check("empty", "true")]


def operation_label(structure, operation):
    op = operation["op"]
    value, key = operation.get("value", ""), operation.get("key", "")
    if op == "resize":
        return f'Resize({operation.get("size", "")})'
    if op == "set":
        return f'Set({operation.get("index", "")}, {value})'
    if op == "copy":
        return "Копия: изменить [0], проверить оригинал"
    if op == "get" and structure == "array":
        return f'Get({operation.get("index", "")})'
    if op in ("add", "update"):
        return f'{op.capitalize()}({key}, {value})'
    if op == "push" and structure == "queue":
        return f'Push({value}, {operation.get("priority", "")})'
    names = {"insert": "Insert", "remove": "Remove", "get": "Get", "push": "Push",
             "contains": "ContainsKey" if structure == "hash" else "Contains"}
    if op in names:
        return f'{names[op]}({key if structure == "hash" else value})'
    return {"height": "GetHeight()", "inorder": "LKP()", "threads": "LKPThreads()",
            "reverse": "PKLThreads()", "capacity": "GetCapacity()", "empty": "IsEmpty()",
            "top": "GetMax()" if structure == "tree" else "Top()",
            "pop": "ExtractMax()" if structure == "tree" else "Pop()"}.get(op, op)


def integer(value, label, minimum=-2147483648, maximum=2147483647):
    if type(value) is not int or not minimum <= value <= maximum:
        raise ValueError(f"{label}: ожидается целое число от {minimum} до {maximum}")
    return value


def encode_fields(fields):
    output = bytearray()
    for field in fields:
        data = str(field).encode("utf-8")
        output.extend(str(len(data)).encode("ascii") + b"\n" + data + b"\n")
    return bytes(output)


def call_cpp(backend, fields):
    try:
        result = subprocess.run([str(backend)], input=encode_fields(fields),
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
    except subprocess.TimeoutExpired as error:
        raise ValueError("Расчёт занял больше двух минут. Уменьшите количество предметов.") from error
    if result.returncode:
        message = result.stderr.decode("utf-8", errors="replace")
        raise ValueError(message or "C++ программа завершилась с ошибкой")
    return json.loads(result.stdout)


def handle_request(backend, request):
    if not isinstance(request, dict):
        raise ValueError("Ожидается объект с параметрами запроса")
    action = request.get("action")
    if action in ("solve", "trace"):
        method = request.get("method")
        if method not in ("optimized", "brute", "compare"):
            raise ValueError("Неизвестный метод поиска")
        capacity = integer(request.get("capacity"), "Вместимость", 0)
        items = request.get("items")
        if not isinstance(items, list) or len(items) > 200:
            raise ValueError("Ожидается список из не более 200 предметов")
        fields = [action, method, capacity, len(items)]
        for item in items:
            if not isinstance(item, dict):
                raise ValueError("Некорректный предмет")
            fields.extend([integer(item.get("volume"), "Объём", 0), integer(item.get("value"), "Стоимость")])
        return call_cpp(backend, fields)
    if action != "structure":
        raise ValueError("Неизвестный запрос")
    structure, value_type = request.get("structure"), request.get("valueType")
    if structure not in ("array", "tree", "hash", "queue", "minheap", "maxheap"):
        raise ValueError("Неизвестная структура данных")
    if value_type not in ("int", "double", "string"):
        raise ValueError("Неизвестный тип значений")
    if structure.endswith("heap") and value_type != "double":
        raise ValueError("BinaryHeap в этом проекте хранит только double")
    testing = request.get("mode") == "tests"
    operations = ready_checks(structure, value_type) if testing else request.get("operations")
    if not isinstance(operations, list) or len(operations) > 100:
        raise ValueError("Можно выполнить до 100 операций. Очистите историю, чтобы начать заново.")
    count = len(operations)
    if testing and "count" in request:
        count = max(0, min(integer(request["count"], "Номер шага"), count))
    fields = [action, structure, value_type, count]
    for operation in operations:
        if not isinstance(operation, dict) or not isinstance(operation.get("op"), str):
            raise ValueError("Некорректная операция")
        for name in FIELDS:
            value = operation.get(name, "")
            if not isinstance(value, str) or len(value) > 80:
                raise ValueError("Введите значение длиной до 80 символов")
    for operation in operations[:count]:
        fields.extend(operation.get(name, "") for name in FIELDS)
    result = call_cpp(backend, fields)
    steps = []
    for operation, output in zip(operations, result["outputs"]):
        expected = operation.get("expect") if testing else None
        passed = (output["actual"] == expected and output["errorType"] == (operation.get("throws") or "")) if testing else None
        steps.append(dict(call=operation_label(structure, operation), actual=output["actual"],
                          detail=output["detail"], expected=expected, passed=passed, error=bool(output["errorType"])))
    return {"lab": dict(steps=steps, plan=[operation_label(structure, op) for op in operations],
                        expectations=[op.get("expect") if testing else None for op in operations],
                        count=count, total=len(operations), passed=sum(step["passed"] is True for step in steps),
                        failed=sum(step["passed"] is False for step in steps), snapshot=result["snapshot"])}


def find_backend(override=None):
    if override:
        backend = Path(override).resolve()
        if not backend.is_file():
            raise ValueError(f"Не найден файл C++ программы: {backend}")
        return backend
    folder = ROOT / "cmake-build-debug"
    backend = folder / ("lab2_web_backend.exe" if os.name == "nt" else "lab2_web_backend")
    sources = [ROOT / "gui" / "WebBackend.cpp", *ROOT.glob("*.h")]
    if backend.is_file() and all(path.stat().st_mtime <= backend.stat().st_mtime for path in sources):
        return backend
    compiler = next((path for name in ("c++", "g++", "clang++", "cl") if (path := shutil.which(name))), None)
    if not compiler:
        raise ValueError("Не найден компилятор C++. Соберите lab2_web_backend в CLion и запустите сервер с --backend ПУТЬ_К_ПРОГРАММЕ.")
    folder.mkdir(exist_ok=True)
    source = ROOT / "gui" / "WebBackend.cpp"
    if Path(compiler).name.lower() in ("cl", "cl.exe"):
        command = [compiler, "/nologo", "/std:c++20", "/EHsc", "/utf-8", "/O2", f"/I{ROOT}", str(source), f"/Fe:{backend}", f"/Fo:{folder / 'WebBackend.obj'}"]
    else:
        command = [compiler, "-std=c++20", "-O2", "-I", str(ROOT), str(source), "-o", str(backend)]
    print("Сборка C++ программы…", flush=True)
    subprocess.run(command, check=True)
    return backend


class WebsiteHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, backend, **kwargs):
        self.backend = backend
        super().__init__(*args, directory=str(ROOT / "web"), **kwargs)

    def log_message(self, format, *args):
        pass

    def json_response(self, status, data):
        body = json.dumps(data, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        if self.path != "/api":
            self.json_response(404, {"error": "Неизвестный адрес"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 1048576:
                raise ValueError("Некорректный размер запроса")
            request = json.loads(self.rfile.read(length))
            result = handle_request(self.backend, request)
            self.json_response(200, result)
        except (ValueError, UnicodeError) as error:
            self.json_response(400, {"error": str(error)})
        except (OSError, subprocess.SubprocessError):
            self.json_response(500, {"error": "Не удалось запустить C++ программу. Перезапустите сервер."})


def main():
    parser = argparse.ArgumentParser(description="Рюкзак: HTML-интерфейс и C++ алгоритмы")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--backend", help="Путь к собранному lab2_web_backend")
    parser.add_argument("--no-browser", action="store_true", help="Не открывать браузер автоматически")
    args = parser.parse_args()
    try:
        backend = find_backend(args.backend)
        handler = functools.partial(WebsiteHandler, backend=backend)
        with ThreadingHTTPServer(("127.0.0.1", args.port), handler) as server:
            url = f"http://127.0.0.1:{server.server_port}/"
            print(f"Рюкзак: {url}\nРасчёты: C++\nОстановить сервер: Ctrl+C", flush=True)
            if not args.no_browser:
                threading.Timer(0.3, webbrowser.open, args=(url,)).start()
            server.serve_forever()
    except KeyboardInterrupt:
        pass
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print(f"Ошибка запуска: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
