#ifndef KTHREAD_H
#define KTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

void kt_for(int n_threads, void (*func)(void*,long,int), void *data, long n);
void kt_pipeline(int n_threads, void *(*func)(void*, int, void*), void *shared_data, int n_steps);

void *kt_forpool_init(int n_threads);
void kt_forpool_destroy(void *_fp);
void kt_forpool(void *_fp, void (*func)(void*,long,int), void *data, long n);

#ifdef __cplusplus
}
#endif

#endif
