# Мозг агента (Python)

`brain.py` загружает ядро через `ctypes`, вызывает его функции и собирает сообщения агента в JSON.

| Файл | Что в нём |
|---|---|
| [`brain.py`](brain.py) | класс `CoreInfo` (272 байта, смещения 0, 8, 12, 16 — как в `core.h`), загрузка `libsentry_core`, функция `get_info()`: код ошибки ядра превращается в `OSError`; при запуске печатает сообщение `agent_started` |

Запуск (сначала собрать ядро в `agent/core`):

```
$ python3 brain.py
{
  "event": "agent_started",
  "os": "darwin",
  "pid": 49014,
  "cpus": 10,
  "page_size": 16384,
  "host": "MacBook-Air-Tim-2.local"
}
```
