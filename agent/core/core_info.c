#include <errno.h>
#include <string.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "core.h"

int core_get_info(struct core_info *out) {
    if (out == NULL)
        return EINVAL;
    memset(out, 0, sizeof *out);

    out->pid = getpid();
    out->cpus = (int32_t)sysconf(_SC_NPROCESSORS_ONLN);
    out->page_size = (int32_t)sysconf(_SC_PAGESIZE);

    if (gethostname(out->host, sizeof out->host) != 0)
        return errno;
    out->host[sizeof out->host - 1] = '\0';

    struct timeval boot;
    size_t len = sizeof boot;
    if (sysctlbyname("kern.boottime", &boot, &len, NULL, 0) != 0)
        return errno;
    out->uptime_s = (int64_t)(time(NULL) - boot.tv_sec);
    return 0;
}
