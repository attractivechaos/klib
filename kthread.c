#include <pthread.h>
#include <stdlib.h>

typedef long long ktint64_t;
struct kt_for_t;

typedef struct {
	struct kt_for_t *t;
	int tid, i;
} ktf_worker_t;

typedef struct kt_for_t {
	int n_threads, n;
	ktf_worker_t *w;
	void (*func)(void*,int,int);
	void *data;
} kt_for_t;

static inline int64_t steal_work(kt_for_t *t)
{
	int i, k, min = t->n, min_i = -1;
	for (i = 0; i < t->n_threads; ++i)
		if (min > t->w[i].i) min = t->w[i].i, min_i = i;
	if (min_i < 0) return -1;
	k = __sync_fetch_and_add(&t->w[min_i].i, t->n_threads);
	return k >= t->n? -1 : (ktint64_t)min_i<<32 | k;
}

static void *ktf_worker(void *data)
{
	ktf_worker_t *w = (ktf_worker_t*)data;
	int64_t x;
	for (;;) {
		int i = __sync_fetch_and_add(&w->i, w->t->n_threads);
		if (i >= w->t->n) break;
		w->t->func(w->t->data, i, w->tid);
	}
	while ((x = steal_work(w->t)) >= 0)
		w->t->func(w->t->data, (unsigned)x, x>>32);
	pthread_exit(0);
}

void kt_for(int n_threads, void (*func)(void*,int,int), void *data, int n)
{
	int i;
	kt_for_t t;
	pthread_t *tid;
	t.func = func, t.data = data, t.n_threads = n_threads, t.n = n;
	t.w = (ktf_worker_t*)alloca(n_threads * sizeof(ktf_worker_t));
	tid = (pthread_t*)alloca(n_threads * sizeof(pthread_t));
	for (i = 0; i < n_threads; ++i)
		t.w[i].t = &t, t.w[i].tid = t.w[i].i = i;
	for (i = 0; i < n_threads; ++i) pthread_create(&tid[i], 0, ktf_worker, &t.w[i]);
	for (i = 0; i < n_threads; ++i) pthread_join(tid[i], 0);
}
