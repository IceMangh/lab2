import io
import json
from pathlib import Path
import random
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import server


BACKEND = server.find_backend(sys.argv[1] if len(sys.argv) > 1 else None)


def request(action="structure", **parameters):
    return server.handle_request(BACKEND, dict(action=action, **parameters))


def structure(kind="array", value_type="int", **parameters):
    return request(structure=kind, valueType=value_type, **parameters)["lab"]


class MemoryConnection:
    def __init__(self, data):
        self.input = io.BytesIO(data)
        self.output = bytearray()

    def makefile(self, *args):
        return self.input

    def sendall(self, data):
        self.output.extend(data)


def http_request(method, path, body=b""):
    message = f"{method} {path} HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Length: {len(body)}\r\n\r\n".encode() + body
    connection = MemoryConnection(message)
    server.WebsiteHandler(connection, ("127.0.0.1", 12345), None, backend=BACKEND)
    headers, content = bytes(connection.output).split(b"\r\n\r\n", 1)
    return int(headers.split()[1]), content


class WebTests(unittest.TestCase):
    def test_all_ready_checks_and_replay(self):
        total = 0
        for kind in ("array", "tree", "hash", "queue", "minheap", "maxheap"):
            types = ("double",) if kind.endswith("heap") else ("int", "double", "string")
            for value_type in types:
                with self.subTest(structure=kind, value_type=value_type):
                    result = structure(kind, value_type, mode="tests")
                    self.assertEqual(result["failed"], 0)
                    self.assertEqual(result["passed"], result["total"])
                    total += result["total"]
                    for count in (0, 1, result["total"] // 2, result["total"]):
                        prefix = structure(kind, value_type, mode="tests", count=count)
                        self.assertEqual(prefix["steps"], result["steps"][:count])
        self.assertEqual(total, 244)

    def test_strings_and_error_recovery(self):
        value = 'Привет <html> "\\\n\r\t\x00🙂'
        result = structure(value_type="string", mode="manual", operations=[
            dict(op="resize", size="2"), dict(op="set", index="0", value=value),
            dict(op="copy", value="copy"), dict(op="get", index="-1"), dict(op="get", index="0")])
        self.assertEqual(result["snapshot"]["values"], [value, ""])
        self.assertEqual(result["steps"][2]["actual"], value)
        self.assertTrue(result["steps"][3]["error"])
        self.assertEqual(result["steps"][-1]["actual"], value)

    def test_solvers_and_real_trace(self):
        rng = random.Random(2026)
        for case in range(16):
            items = [dict(volume=rng.randrange(8), value=rng.randrange(-5, 30)) for _ in range(case % 9)]
            capacity = rng.randrange(16)
            solved = request("solve", method="compare", capacity=capacity, items=items)["results"]
            self.assertEqual(solved["brute"]["cost"], solved["optimized"]["cost"])
            for method in ("brute", "optimized"):
                trace = request("trace", method=method, capacity=capacity, items=items)["trace"]
                self.assertEqual(trace["result"]["cost"], solved[method]["cost"])
                self.assertEqual(trace["result"]["indices"], solved[method]["indices"])
                self.assertEqual(trace["steps"][-1]["type"], "finish")
                for step in trace["steps"]:
                    self.assertEqual(step["cost"], sum(items[i]["value"] for i in step["path"]))
                    self.assertEqual(step["volume"], sum(items[i]["volume"] for i in step["path"]))
                    self.assertLessEqual(step["volume"], capacity)

    def test_large_dataset_and_trace_limit(self):
        rng = random.Random(2026)
        items = [dict(volume=rng.randrange(1, 16), value=rng.randrange(1, 101)) for _ in range(40)]
        result = request("solve", method="compare", capacity=80, items=items)["results"]
        self.assertTrue(result["brute"]["skipped"])
        trace = request("trace", method="optimized", capacity=80, items=items)["trace"]
        self.assertEqual(trace["result"]["cost"], result["optimized"]["cost"])
        self.assertLessEqual(len(trace["steps"]), 2001)
        self.assertEqual(trace["steps"][-1]["type"], "finish")

    def test_invalid_input(self):
        for capacity in (-1, 1.5, True, "10"):
            with self.assertRaises(ValueError):
                request("solve", method="optimized", capacity=capacity, items=[])
        for value in ("NaN", "inf", "1e999", "2.5bad"):
            result = structure("minheap", "double", mode="manual", operations=[dict(op="push", value=value)])
            self.assertTrue(result["steps"][0]["error"])
            self.assertEqual(result["snapshot"]["values"], [])
        with self.assertRaises(ValueError):
            structure("minheap", "string", mode="tests")
        with self.assertRaises(ValueError):
            request("solve", method="brute", capacity=1, items=[dict(volume=1, value=1)] * 29)
        with self.assertRaises(ValueError):
            structure(mode="manual", operations=[dict(op="get")] * 101)

    def test_http_routes_and_static_assets(self):
        for path in ("/", "/app.js", "/style.css", "/search.js", "/structures.js"):
            status, body = http_request("GET", path)
            self.assertEqual(status, 200)
            self.assertTrue(body)
        payload = dict(action="solve", method="optimized", capacity=3, items=[dict(volume=2, value=10)])
        status, body = http_request("POST", "/api", json.dumps(payload).encode())
        self.assertEqual(status, 200)
        self.assertEqual(json.loads(body)["results"]["optimized"]["cost"], 10)
        status, body = http_request("POST", "/api", b"not json")
        self.assertEqual(status, 400)
        self.assertIn("error", json.loads(body))
        self.assertEqual(http_request("POST", "/unknown", b"{}")[0], 404)
        self.assertEqual(http_request("GET", "/../Knapsack.h")[0], 404)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]], verbosity=2)
