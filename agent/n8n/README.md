# Сценарии n8n

Сценарий «Агент-сторож: запуск»: **Webhook** (POST `/webhook/agent`) → **Basic LLM Chain** с моделью **Ollama Chat Model** `qwen2.5:3b` → **Send an Email** (SMTP). Модель пишет пояснение к сообщению агента, в письмо уходит пояснение и исходный JSON агента. Подключения в выгрузку не попадают — только их имена.

| Файл | Что в нём |
|---|---|
| [`agent-started.json`](agent-started.json) | сценарий главы 1.3: пояснение к событию `agent_started` |
| [`agent-uptime.json`](agent-uptime.json) | тот же сценарий после задания модуля 1: запрос Б — дни и часы считает выражение n8n, модель только объясняет |
| [`ai-log.md`](ai-log.md) | журнал: три ответа модели на запрос А и три на запрос Б, проверка по `uptime` |

Запуск: n8n (`n8n start`, http://localhost:5678), Ollama (`ollama serve`, http://localhost:11434), из `agent/brain`:

```
python3 brain.py http://localhost:5678/webhook/agent
```
