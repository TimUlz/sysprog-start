# Ядро агента (C)

Ядро собирается в динамическую библиотеку: `sentry_core.dll` в Windows, `libsentry_core.dylib` в macOS,
`libsentry_core.so` в Linux.

| Файл | Что в нём |
|---|---|
| [`core.h`](core.h) | структуры `struct core_info` (280 байт), `struct core_proc` (80 байт: `pid`, `ppid` int64, `name[64]`), `struct core_child` (`pid`, `handle` int64) и объявления четырёх функций ядра |
| [`core_info.c`](core_info.c) | `core_get_info`: `getpid`, `sysconf(_SC_NPROCESSORS_ONLN)`, `sysconf(_SC_PAGESIZE)`, `gethostname`, время работы системы — на macOS `sysctlbyname("kern.boottime")`, на Linux `sysinfo().uptime` |
| [`core_procs.c`](core_procs.c) | `core_list_procs(out, max, count)`: номера процессов из `proc_listallpids`, для каждого `proc_pidinfo(PROC_PIDT_SHORTBSDINFO)` — родитель `pbsi_ppid` и имя `pbsi_comm`; в `count` всегда пишется, сколько процессов найдено, если места мало — код `ERANGE` (34) |
| [`core_spawn.c`](core_spawn.c) | `core_spawn(argv, child)` запускает программу через `posix_spawnp` (поиск по `PATH`), `core_wait(child, timeout_ms, exit_code)` опрашивает `waitpid(WNOHANG)` раз в 10 мс; по истечении времени — `ETIMEDOUT` (60), код завершения из `WEXITSTATUS`, для сигнала — 128 + номер сигнала |
| [`show_info.c`](show_info.c) | проверка `core_get_info`: печатает поля структуры |
| [`list_procs.c`](list_procs.c) | проверка `core_list_procs`: таблица `pid ppid name`; необязательный аргумент — сколько мест дать ядру |
| [`run_child.c`](run_child.c) | проверка `core_spawn` и `core_wait`: `run_child timeout_ms программа [аргументы]` |
| [`leak.c`](leak.c) | 20 запусков `true` без `core_wait` и подсчёт зомби через `ps` до и после |

CORE_API в `core.h` отмечает функцию для экспорта: в Windows это `__declspec(dllexport)`, в macOS и Linux — пусто.

## Сборка библиотеки

```
$ clang -Wall -Wextra -shared -fPIC -o libsentry_core.dylib core_info.c core_procs.c core_spawn.c core_cpu.c core_threads.c
$ nm -gU libsentry_core.dylib
000000000000067c T _core_get_info
0000000000000790 T _core_list_procs
00000000000004f8 T _core_proc_cpu
0000000000000ba4 T _core_proc_threads
0000000000000998 T _core_spawn
0000000000000a50 T _core_wait
```

## Список процессов (2.1)

```
$ clang -Wall -Wextra -o list_procs list_procs.c core_procs.c && ./list_procs | tail -4
    360       1 fseventsd
      1       0 launchd
      0       0 kernel_task
processes 829, sizeof 80
$ ps -A | wc -l
     830
$ ./list_procs 10; echo code $?
core_list_procs: error 34, need 828 places, have 10
code 1
```

`ps -A | wc -l` печатает на одну строку больше: первая строка — заголовок. Разница в один-два процесса
между запусками — процессы, которые успели родиться или завершиться между двумя командами.
При нехватке места ядро не обрезает список молча: возвращает `ERANGE` и сообщает, сколько мест нужно.

## Запуск процесса (2.2)

```
$ clang -Wall -Wextra -o run_child run_child.c core_spawn.c
$ ./run_child 5000 bash -c 'exit 7'; echo code $?
started pid 90367
pid 90367 exited, code 7
code 0
$ ./run_child 500 sleep 3; echo code $?
started pid 90381
core_wait: error 60, pid 90381 still running
code 1
$ ./run_child 1000 nosuch; echo code $?
core_spawn: error 2
code 1
$ clang -Wall -Wextra -o leak leak.c core_spawn.c && ./leak
before: zombies 0
after 20 core_spawn without core_wait: zombies 20
```

Код 7 ребёнка дошёл до родителя через `waitpid`. `sleep 3` не уложился в 500 мс — `ETIMEDOUT` (60).
Программы `nosuch` нет ни в одной папке `PATH` — `posix_spawnp` вернул `ENOENT` (2).
Без `core_wait` завершившиеся дети остаются зомби, пока родитель жив: 20 запусков — 20 зомби.

## Проверка `core_get_info`

```
$ clang -Wall -Wextra -o show_info show_info.c core_info.c && ./show_info
pid       52575
cpus      10
page_size 16384
host      MacBook-Air-Tim-2.local
uptime_s  1233997
sizeof    280
```

## Процессорное время и потоки (2.3 и 2.4)

Сборка из каталога agent/core на macOS:

    clang -Wall -Wextra -o cpu_top cpu_top.c core_cpu.c core_spawn.c
    clang -Wall -Wextra -pthread -o count_threads count_threads.c core_threads.c
    ./cpu_top
    ./count_threads

core_proc_cpu получает из PROC_PIDTASKINFO пользовательское и системное время. На Apple Silicon тики Mach переводятся в микросекунды с учётом mach_timebase_info. На нагрузочном процессе за 1,0051 с получилось 94,9% одного ядра. Для PID 2147483646 получен ESRCH (3), для системного процесса PID 1 — EPERM (1).

core_proc_threads читает поле pti_threadnum того же набора данных. В count_threads главный поток ждёт готовности четырёх рабочих и снимает показание, пока они заблокированы на условной переменной. Последний снимок сделан после pthread_join и завершения учёта потоков в ядре. На Mac получена последовательность 1 → 5 → 1.

Проверка единиц на тех же данных даёт контрастный результат. Если принять тики Mach за наносекунды и пропустить коэффициент 125/3 этого Mac, вместо 94,9% получится около 2,3%.

В отдельном опыте четыре потока прибавляли по единице миллион раз. Одна атомарная операция fetch_add дала 4 000 000. При раздельных атомарных чтении и записи два запуска дали 1 373 201 и 980 309. Два потока могут прочитать одно старое значение и записать одну и ту же сумму. Для счётчика операция чтения с изменением должна выполняться целиком под блокировкой или одной атомарной инструкцией.
