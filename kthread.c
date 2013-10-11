#include <pthread.h>
#include <stdlib.h>
#include "kthread.h"

/**************
 *** kh_for ***
 **************/

struct kt_for_t;

typedef struct {
	struct kt_for_t *t;
	int i;
} ktf_worker_t;

typedef struct kt_for_t {
	int n_threads, n;
	ktf_worker_t *w;
	void (*func)(void*,int,int);
	void *data;
} kt_for_t;

static inline int ktf_steal_work(kt_for_t *t)
{
	int i, k, min = 0x7fffffff, min_i = 0;
	for (i = 0; i < t->n_threads; ++i)
		if (min > t->w[i].i) min = t->w[i].i, min_i = i;
	k = __sync_fetch_and_add(&t->w[min_i].i, t->n_threads);
	return k >= t->n? -1 : k;
}

static void *ktf_worker(void *data)
{
	ktf_worker_t *w = (ktf_worker_t*)data;
	int i;
	for (;;) {
		i = __sync_fetch_and_add(&w->i, w->t->n_threads);
		if (i >= w->t->n) break;
		w->t->func(w->t->data, i, w - w->t->w);
	}
	while ((i = ktf_steal_work(w->t)) >= 0)
		w->t->func(w->t->data, i, w - w->t->w);
	pthread_exit(0);
}

void kt_for(int n_threads, int n, void (*func)(void*,int,int), void *data)
{
	int i;
	kt_for_t t;
	pthread_t *tid;
	t.func = func, t.data = data, t.n_threads = n_threads, t.n = n;
	t.w = (ktf_worker_t*)alloca(n_threads * sizeof(ktf_worker_t));
	tid = (pthread_t*)alloca(n_threads * sizeof(pthread_t));
	for (i = 0; i < n_threads; ++i)
		t.w[i].t = &t, t.w[i].i = i;
	for (i = 0; i < n_threads; ++i) pthread_create(&tid[i], 0, ktf_worker, &t.w[i]);
	for (i = 0; i < n_threads; ++i) pthread_join(tid[i], 0);
}

/************************
 *** kt_spawn/kt_sync ***
 ************************/

typedef long long ktint64_t;

typedef struct {
	kthread_t *t;
	int i; // slot ID
	pthread_t tid;
	int pending;
} kts_worker_t;

typedef struct {
	int n;
	void (*func)(void*,int,int);
	void *data;
} kts_task_t;

struct kthread_t {
	kts_worker_t *w;
	int n_threads, to_sync;
	int n_tasks, n_slots;
	kts_task_t tasks[KT_MAX_TASKS];
	pthread_mutex_t lock;
	pthread_cond_t cv;
};

static inline void process_slot(const kthread_t *t, int i, int tid)
{
	int j;
	for (j = 0;; ++j) {
		kts_task_t task = t->tasks[j];
		if (i < task.n) {
			task.func(task.data, i, tid);
			break;
		} else i -= task.n;
	}
}

static void *kts_worker(void *data)
{
	kts_worker_t *w = (kts_worker_t*)data;
	for (;;) {
		int i, to_sync, n_slots;
		// update the task and slot information
		pthread_mutex_lock(&w->t->lock);
		while (w->i >= w->t->n_slots && !w->t->to_sync)
			pthread_cond_wait(&w->t->cv, &w->t->lock);
		to_sync = w->t->to_sync, n_slots = w->t->n_slots;
		pthread_mutex_unlock(&w->t->lock);
		// process the pending slot if there is any
		if (w->pending >= 0 && w->pending < n_slots) {
			process_slot(w->t, w->pending, w - w->t->w);
			w->pending = -1;
		}
		// process slots assigned to the current worker
		for (;;) {
			i = __sync_fetch_and_add(&w->i, w->t->n_threads);
			if (i >= n_slots) break;
			process_slot(w->t, i, w - w->t->w);
		}
		// steal slots from other workers
		for (;;) {
			int min = 0x7fffffff, min_i = 0;
			for (i = 0; i < w->t->n_threads; ++i)
				if (min > w->t->w[i].i) min = w->t->w[i].i, min_i = i;
			i = __sync_fetch_and_add(&w->t->w[min_i].i, w->t->n_threads);
			if (i >= n_slots) {
				w->pending = i;
				break;
			} else process_slot(w->t, i, w - w->t->w);
		}
		if (to_sync) break;
	}
	pthread_exit(0);
}

kthread_t *kt_init(int n_threads)
{
	kthread_t *t;
	int i;
	t = (kthread_t*)calloc(1, sizeof(kthread_t));
	t->n_threads = n_threads;
	t->w = (kts_worker_t*)calloc(t->n_threads, sizeof(kts_worker_t));
	pthread_mutex_init(&t->lock, 0);
	pthread_cond_init(&t->cv, 0);
	for (i = 0; i < t->n_threads; ++i) {
		t->w[i].i = i, t->w[i].t = t, t->w[i].pending = -1;
		pthread_create(&t->w[i].tid, 0, kts_worker, &t->w[i]);
	}
	return t;
}

void kt_sync(kthread_t *t)
{
	int i;
	pthread_mutex_lock(&t->lock);
	t->to_sync = 1;
	pthread_cond_broadcast(&t->cv);
	pthread_mutex_unlock(&t->lock);
	for (i = 0; i < t->n_threads; ++i)
		pthread_join(t->w[i].tid, 0);
	free(t->w); free(t);
}

int kt_spawn(kthread_t *t, int n, void (*func)(void*,int,int), void *data)
{
	kts_task_t *task;
	if (t->n_tasks >= KT_MAX_TASKS) return -1;
	pthread_mutex_lock(&t->lock);
	task = &t->tasks[t->n_tasks++];
	task->n = n, task->func = func, task->data = data;
	t->n_slots += n;
	pthread_cond_broadcast(&t->cv);
	pthread_mutex_unlock(&t->lock);
	return 0;
}
