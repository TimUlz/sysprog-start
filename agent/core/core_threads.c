#include <errno.h>
#include <libproc.h>
#include "core.h"

int core_proc_threads(int64_t pid, int32_t *count) {
    if (count == NULL || pid < 0 || pid > INT32_MAX)
        return EINVAL;
    struct proc_taskinfo task;
    int size = proc_pidinfo((int)pid, PROC_PIDTASKINFO, 0, &task, sizeof task);
    if (size != sizeof task)
        return size <= 0 && errno ? errno : ESRCH;
    *count = task.pti_threadnum;
    return 0;
}
