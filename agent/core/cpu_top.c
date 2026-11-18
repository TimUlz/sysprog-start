#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "core.h"

static int sample(int64_t pid, int64_t *total) {
    struct core_cpu cpu;
    int err = core_proc_cpu(pid, &cpu);
    if (err) {
        fprintf(stderr, "core_proc_cpu: error %d\n", err);
        return err;
    }
    printf("pid %" PRId64 ": user %" PRId64 " us, kernel %" PRId64 " us\n",
           pid, cpu.user_us, cpu.kernel_us);
    *total = cpu.user_us + cpu.kernel_us;
    return 0;
}

int main(int argc, char **argv) {
    int64_t first, last;
    if (argc == 2) {
        char *end;
        errno = 0;
        int64_t pid = strtoll(argv[1], &end, 10);
        if (errno || end == argv[1] || *end || pid < 0 || pid > INT32_MAX)
            return 1;
        return sample(pid, &first) != 0;
    }
    if (argc != 1)
        return 1;
    char *load[] = {"python3", "-c", "import time\nt = time.monotonic() + 2\nwhile time.monotonic() < t: pass", NULL};
    struct core_child child;
    int err = core_spawn(load, &child);
    if (err) {
        fprintf(stderr, "core_spawn: error %d\n", err);
        return 1;
    }
    usleep(200000);
    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    err = sample(child.pid, &first);
    sleep(1);
    if (!err)
        err = sample(child.pid, &last);
    double wall = (clock_gettime_nsec_np(CLOCK_MONOTONIC) - start) / 1e9;
    if (!err)
        printf("elapsed %.4f s, load %.1f%%\n", wall, (last - first) / (wall * 10000.0));
    int32_t code;
    return core_wait(&child, -1, &code) || err || code;
}
