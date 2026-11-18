# Мозг агента (Python)

`brain.py` загружает ядро через `ctypes`, вызывает его функции и собирает сообщения агента в JSON.

| Файл | Что в нём |
|---|---|
| [`brain.py`](brain.py) | классы `CoreInfo` (280 байт), `CoreProc` (80 байт), `CoreChild` и `CoreCpu` — повторяют структуры из `core.h`; загрузка `libsentry_core` и типы аргументов всех шести функций ядра; код ошибки ядра превращается в `OSError` |

Функции:

| Функция | Что делает |
|---|---|
| `get_info()` | сведения о системе из `core_get_info` |
| `list_procs(room=256)` | список процессов из `core_list_procs`; если ядро ответило «мало места» (`ERANGE`, в Windows 122), повторяет вызов с `count + 64` местами |
| `parents(procs, pid)` | цепочка родителей процесса по полю `ppid` до процесса, который сам себе родитель (`kernel_task` 0), не длиннее 64 звеньев |
| `spawn(args)` | запуск программы через `core_spawn`: список строк превращается в массив `char *` с `None` в конце |
| `wait(child, timeout_ms=-1)` | ожидание через `core_wait`, возвращает код завершения |
| proc_cpu(pid) | суммарное процессорное время в мкс, None при ошибке |
| cpu_load(interval) | загрузка в процентах одного ядра по двум снимкам, в порядке убывания |
| proc_threads(pid) | число потоков доступного процесса, None при ошибке |
| most_threads(n) | первые n процессов по числу потоков |
| `send(message, url)` | POST-запрос с сообщением в n8n (`urllib.request`), адрес вебхука — аргумент командной строки |

При запуске brain.py дочерний Python создаёт четыре рабочих потока. Агент считает загрузку процессов за секунду и читает число потоков ребёнка. После завершения ребёнка агент печатает один JSON. Поля cpu_top и most_threads содержат по три первых результата. В запуске на Mac child_threads равнялся 5, child_code — 3.

    cd agent/core
    clang -Wall -Wextra -shared -fPIC -o libsentry_core.dylib core_*.c
    cd ../brain
    python3 brain.py

Отправка в n8n (сценарий [`agent/n8n/agent-started.json`](../n8n/agent-started.json): Webhook → Basic LLM Chain с моделью `qwen2.5:3b` в Ollama → Send an Email):

```
$ python3 brain.py http://localhost:5678/webhook/agent
...
n8n: 200 {"message":"Workflow was started"}
```

Письмо пришло с темой «Агент-сторож: agent_started на MacBook-Air-Tim-2.local». Модель написала, что агент «запущен в системе 10 CPU-процессоров» — это число процессоров компьютера, а не сколько их занимает агент, поэтому в письмо вместе с пояснением модели идёт исходный JSON агента.
