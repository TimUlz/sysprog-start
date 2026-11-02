# Мозг агента (Python)

`brain.py` загружает ядро через `ctypes`, вызывает его функции и собирает сообщения агента в JSON.

| Файл | Что в нём |
|---|---|
| [`brain.py`](brain.py) | класс `CoreInfo` (272 байта, смещения 0, 8, 12, 16 — как в `core.h`), загрузка `libsentry_core`, функция `get_info()`: код ошибки ядра превращается в `OSError`; при запуске печатает сообщение `agent_started`; функция `send(message, url)` отправляет его POST-запросом в n8n (`urllib.request`), адрес вебхука — аргумент командной строки |

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

Отправка в n8n (сценарий [`agent/n8n/agent-started.json`](../n8n/agent-started.json): Webhook → Basic LLM Chain с моделью `qwen2.5:3b` в Ollama → Send an Email):

```
$ python3 brain.py http://localhost:5678/webhook/agent
...
n8n: 200 {"message":"Workflow was started"}
```

Письмо пришло с темой «Агент-сторож: agent_started на MacBook-Air-Tim-2.local». Модель написала, что агент «запущен в системе 10 CPU-процессоров» — это число процессоров компьютера, а не сколько их занимает агент, поэтому в письмо вместе с пояснением модели идёт исходный JSON агента.
