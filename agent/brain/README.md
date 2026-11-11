# Мозг агента (Python)

`brain.py` загружает ядро через `ctypes`, вызывает его функции и собирает сообщения агента в JSON.

| Файл | Что в нём |
|---|---|
| [`brain.py`](brain.py) | классы `CoreInfo` (280 байт), `CoreProc` (80 байт) и `CoreChild` — повторяют структуры из `core.h`; загрузка `libsentry_core` и типы аргументов всех четырёх функций ядра; код ошибки ядра превращается в `OSError` |

Функции:

| Функция | Что делает |
|---|---|
| `get_info()` | сведения о системе из `core_get_info` |
| `list_procs(room=256)` | список процессов из `core_list_procs`; если ядро ответило «мало места» (`ERANGE`, в Windows 122), повторяет вызов с `count + 64` местами |
| `parents(procs, pid)` | цепочка родителей процесса по полю `ppid` до процесса, который сам себе родитель (`kernel_task` 0), не длиннее 64 звеньев |
| `spawn(args)` | запуск программы через `core_spawn`: список строк превращается в массив `char *` с `None` в конце |
| `wait(child, timeout_ms=-1)` | ожидание через `core_wait`, возвращает код завершения |
| `send(message, url)` | POST-запрос с сообщением в n8n (`urllib.request`), адрес вебхука — аргумент командной строки |

При запуске агент запускает дочерний Python, который спит секунду и завершается с кодом 3, пока ребёнок жив — снимает список процессов и цепочку его родителей, затем ждёт код завершения.

Запуск из Терминала (ядро собрано в `agent/core`):

```
$ python3 brain.py
{
  "event": "agent_started",
  "os": "darwin",
  "pid": 90688,
  "cpus": 10,
  "page_size": 16384,
  "host": "MacBook-Air-Tim-2.local",
  "uptime_s": 2003017,
  "procs": 830,
  "child": 90689,
  "child_code": 3,
  "parents": [
    "python3.14",
    "python3.14",
    "zsh",
    "zsh",
    "login",
    "Terminal",
    "launchd",
    "kernel_task"
  ]
}
```

Цепочка читается снизу вверх по дереву: ребёнок `python3.14` 90689 → агент `python3.14` 90688 → оболочки `zsh` →
`login` → `Terminal` → `launchd` → `kernel_task`. Ребёнок вернул код 3 — агент получил его через `core_wait`.

Отправка в n8n (сценарий [`agent/n8n/agent-started.json`](../n8n/agent-started.json): Webhook → Basic LLM Chain с моделью `qwen2.5:3b` в Ollama → Send an Email):

```
$ python3 brain.py http://localhost:5678/webhook/agent
...
n8n: 200 {"message":"Workflow was started"}
```

Письмо пришло с темой «Агент-сторож: agent_started на MacBook-Air-Tim-2.local». Модель написала, что агент «запущен в системе 10 CPU-процессоров» — это число процессоров компьютера, а не сколько их занимает агент, поэтому в письмо вместе с пояснением модели идёт исходный JSON агента.
