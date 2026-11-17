
## Запуск

Из папки проекта:

```bash
python3 server.py
```

Сервер при необходимости соберёт C++ программу и откроет браузер: http://127.0.0.1:8765/.
Остановка — `Ctrl+C` в терминале.

## Проверки

```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --target check
```

[Дизайн системы](Описание.md)
