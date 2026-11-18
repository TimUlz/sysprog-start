#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "core.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static int ready, finish;

static void *worker(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lock);
    ready++;
    pthread_cond_broadcast(&changed);
    while (!finish)
        pthread_cond_wait(&changed, &lock);
    pthread_mutex_unlock(&lock);
    return NULL;
}

static int show(const char *stage) {
    int32_t count;
    int err = core_proc_threads(getpid(), &count);
    if (err)
        fprintf(stderr, "core_proc_threads: %d\n", err);
    else
        printf("%s: %d\n", stage, count);
    return err;
}

int main(void) {
    pthread_t threads[4];
    int created = 0;
    int err = show("before");
    pthread_mutex_lock(&lock);
    while (!err && created < 4) {
        err = pthread_create(&threads[created], NULL, worker, NULL);
        if (!err)
            created++;
    }
    while (ready < created)
        pthread_cond_wait(&changed, &lock);
    if (!err)
        err = show("running");
    finish = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
    for (int i = 0; i < created; i++)
        pthread_join(threads[i], NULL);
    /* Darwin may still count a terminating Mach thread just after pthread_join. */
    int32_t remaining;
    for (int attempt = 0; attempt < 200; attempt++) {
        if (core_proc_threads(getpid(), &remaining))
            return 1;
        if (remaining == 1) {
            printf("joined: %d\n", remaining);
            return err != 0;
        }
        usleep(5000);
    }
    fprintf(stderr, "joined: still %d threads\n", remaining);
    return 1;
}
