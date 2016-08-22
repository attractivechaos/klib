#ifndef __AC_KDQ_H
#define __AC_KDQ_H

#include <stdlib.h>
#include <string.h>

#define __KDQ_TYPE(type) \
	typedef struct { \
		size_t front:58, bits:6, count, mask; \
		type *a; \
	} kdq_##type##_t;

#define kdq_t(type) kdq_##type##_t
#define kdq_size(q) ((q)->count)
#define kdq_first(q) ((q)->a[(q)->front])
#define kdq_last(q) ((q)->a[((q)->front + (q)->count - 1) & (q)->mask])
#define kdq_at(q, i) ((q)->a[((q)->front + (i)) & (q)->mask])

#define __KDQ_IMPL(type, SCOPE) \
	SCOPE kdq_##type##_t *kdq_init_##type() \
	{ \
		kdq_##type##_t *q; \
		q = (kdq_##type##_t*)calloc(1, sizeof(kdq_##type##_t)); \
		q->bits = 2, q->mask = (1ULL<<q->bits) - 1; \
		q->a = (type*)malloc((1<<q->bits) * sizeof(type)); \
		return q; \
	} \
	SCOPE void kdq_destroy_##type(kdq_##type##_t *q) \
	{ \
		if (q == 0) return; \
		free(q->a); free(q); \
	} \
	SCOPE int kdq_resize_##type(kdq_##type##_t *q, int new_bits) \
	{ \
		size_t new_size = 1ULL<<new_bits, old_size = 1ULL<<q->bits; \
		if (new_size < q->count) { /* not big enough */ \
			int i; \
			for (i = 0; i < 64; ++i) \
				if (1ULL<<i > q->count) break; \
			new_bits = i, new_size = 1ULL<<new_bits; \
		} \
		if (new_bits == q->bits) return q->bits; /* unchanged */ \
		if (new_bits > q->bits) q->a = (type*)realloc(q->a, (1ULL<<new_bits) * sizeof(type)); \
		if (q->front + q->count <= old_size) { /* unwrapped */ \
			if (q->front + q->count > new_size) /* only happens for shrinking */ \
				memmove(q->a, q->a + new_size, (q->front + q->count - new_size) * sizeof(type)); \
		} else { /* wrapped */ \
			memmove(q->a + (new_size - (old_size - q->front)), q->a + q->front, (old_size - q->front) * sizeof(type)); \
			q->front = new_size - (old_size - q->front); \
		} \
		q->bits = new_bits, q->mask = (1ULL<<q->bits) - 1; \
		if (new_bits < q->bits) q->a = (type*)realloc(q->a, (1ULL<<new_bits) * sizeof(type)); \
		return q->bits; \
	} \
	SCOPE type *kdq_pushp_##type(kdq_##type##_t *q) \
	{ \
		if (q->count == 1ULL<<q->bits) kdq_resize_##type(q, q->bits + 1); \
		return &q->a[((q->count++) + q->front) & (q)->mask]; \
	} \
	SCOPE void kdq_push_##type(kdq_##type##_t *q, type v) \
	{ \
		if (q->count == 1ULL<<q->bits) kdq_resize_##type(q, q->bits + 1); \
		q->a[((q->count++) + q->front) & (q)->mask] = v; \
	} \
	SCOPE type *kdq_unshiftp_##type(kdq_##type##_t *q) \
	{ \
		if (q->count == 1ULL<<q->bits) kdq_resize_##type(q, q->bits + 1); \
		++q->count; \
		q->front = q->front? q->front - 1 : (1ULL<<q->bits) - 1; \
		return &q->a[q->front]; \
	} \
	SCOPE void kdq_unshift_##type(kdq_##type##_t *q, type v) \
	{ \
		type *p; \
		p = kdq_unshiftp_##type(q); \
		*p = v; \
	} \
	SCOPE type *kdq_pop_##type(kdq_##type##_t *q) \
	{ \
		return q->count? &q->a[((--q->count) + q->front) & q->mask] : 0; \
	} \
	SCOPE type *kdq_shift_##type(kdq_##type##_t *q) \
	{ \
		type *d = 0; \
		if (q->count == 0) return 0; \
		d = &q->a[q->front++]; \
		q->front &= q->mask; \
		--q->count; \
		return d; \
	}

#define KDQ_INIT2(type, SCOPE) \
	__KDQ_TYPE(type) \
	__KDQ_IMPL(type, SCOPE)

#ifndef klib_unused
#if (defined __clang__ && __clang_major__ >= 3) || (defined __GNUC__ && __GNUC__ >= 3)
#define klib_unused __attribute__ ((__unused__))
#else
#define klib_unused
#endif
#endif /* klib_unused */

#define KDQ_INIT(type) KDQ_INIT2(type, static inline klib_unused)

#define KDQ_DECLARE(type) \
	__KDQ_TYPE(type) \
	kdq_##type##_t *kdq_init_##type(); \
	void kdq_destroy_##type(kdq_##type##_t *q); \
	int kdq_resize_##type(kdq_##type##_t *q, int new_bits); \
	type *kdq_pushp_##type(kdq_##type##_t *q); \
	void kdq_push_##type(kdq_##type##_t *q, type v); \
	type *kdq_unshiftp_##type(kdq_##type##_t *q); \
	void kdq_unshift_##type(kdq_##type##_t *q, type v); \
	type *kdq_pop_##type(kdq_##type##_t *q); \
	type *kdq_shift_##type(kdq_##type##_t *q);

#define kdq_init(type) kdq_init_##type()
#define kdq_destroy(type, q) kdq_destroy_##type(q)
#define kdq_resize(type, q, new_bits) kdq_resize_##type(q, new_bits)
#define kdq_pushp(type, q) kdq_pushp_##type(q)
#define kdq_push(type, q, v) kdq_push_##type(q, v)
#define kdq_pop(type, q) kdq_pop_##type(q)
#define kdq_unshiftp(type, q) kdq_unshiftp_##type(q)
#define kdq_unshift(type, q, v) kdq_unshift_##type(q, v)
#define kdq_shift(type, q) kdq_shift_##type(q)

#endif
