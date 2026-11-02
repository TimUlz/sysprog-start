# Ядро агента (C)

Ядро собирается в динамическую библиотеку: `sentry_core.dll` в Windows, `libsentry_core.dylib` в macOS,
`libsentry_core.so` в Linux.

| Файл | Что в нём |
|---|---|
| [`core.h`](core.h) | структура `struct core_info` (280 байт: `pid` int64, `cpus` и `page_size` int32, `host[256]`, `uptime_s` int64 на смещении 272) и объявление `core_get_info` |
| [`core_info.c`](core_info.c) | функция `core_get_info`: `getpid`, `sysconf(_SC_NPROCESSORS_ONLN)`, `sysconf(_SC_PAGESIZE)`, `gethostname`, время работы системы — `sysctlbyname("kern.boottime")` минус текущее время; возвращает 0 или код ошибки, ничего не печатает |
| [`show_info.c`](show_info.c) | программа проверки: вызывает `core_get_info` по адресу структуры и печатает поля |

CORE_API в `core.h` отмечает функцию для экспорта: в Windows это `__declspec(dllexport)`, в macOS и Linux — пусто.

Сборка библиотеки и проверка таблицы экспорта:

```
$ clang -Wall -Wextra -shared -fPIC -o libsentry_core.dylib core_info.c
$ nm -gU libsentry_core.dylib
0000000000000410 T _core_get_info
```

Проверка на macOS (Apple Silicon):

```
$ clang -Wall -Wextra -o show_info show_info.c core_info.c && ./show_info
pid       52575
cpus      10
page_size 16384
host      MacBook-Air-Tim-2.local
uptime_s  1233997
sizeof    280
```
