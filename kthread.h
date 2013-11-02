#ifndef KTHREAD_H
#define KTHREAD_H

#define KT_MAX_TASKS 1024

struct kthread_t;
typedef struct kthread_t kthread_t;

#ifdef __cplusplus
extern "C" {
#endif

kthread_t *kt_init(int n_threads);
int kt_spawn(kthread_t *t, int n, void (*func)(void*,int,int), void *data);
void kt_sync(kthread_t *t);

void kt_for(int n_threads, int n, void (*func)(void*,int,int), void *data);

#ifdef __cplusplus
}
#endif

#endif
