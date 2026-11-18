#include <errno.h>
#include <libproc.h>
#include <mach/mach_time.h>
#include "core.h"

static int64_t microseconds(uint64_t ticks, mach_timebase_info_data_t scale) {
    /* Widen before multiplying: a busy, long-lived process can accumulate many ticks. */
    return (int64_t)((__uint128_t)ticks * scale.numer / scale.denom / 1000);
}

int core_proc_cpu(int64_t pid, struct core_cpu *out) {
    if (out == NULL || pid < 0 || pid > INT32_MAX)
        return EINVAL;
    struct proc_taskinfo task;
    int size = proc_pidinfo((int)pid, PROC_PIDTASKINFO, 0, &task, sizeof task);
    if (size != sizeof task)
        return size <= 0 && errno ? errno : ESRCH;
    mach_timebase_info_data_t scale;
    if (mach_timebase_info(&scale) != KERN_SUCCESS)
        return EIO;
    out->user_us = microseconds(task.pti_total_user, scale);
    out->kernel_us = microseconds(task.pti_total_system, scale);
    return 0;
}
