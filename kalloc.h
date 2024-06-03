#ifndef _KALLOC_H_
#define _KALLOC_H_

#include <stddef.h> /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	size_t capacity, available, n_blocks, n_cores, largest;
} km_stat_t;

void *kmalloc(void *km, size_t size);
void *krealloc(void *km, void *ptr, size_t size);
void *krelocate(void *km, void *ap, size_t n_bytes);
void *kcalloc(void *km, size_t count, size_t size);
void kfree(void *km, void *ptr);

void *km_init(void);
void *km_init2(void *km_par, size_t min_core_size);
void km_destroy(void *km);
void km_stat(const void *_km, km_stat_t *s);
void km_stat_print(const void *km);

#ifdef __cplusplus
}
#endif

#define Kmalloc(km, type, cnt)       ((type*)kmalloc((km), (cnt) * sizeof(type)))
#define Kcalloc(km, type, cnt)       ((type*)kcalloc((km), (cnt), sizeof(type)))
#define Krealloc(km, type, ptr, cnt) ((type*)krealloc((km), (ptr), (cnt) * sizeof(type)))

#define Kexpand(km, type, a, m) do { \
		(m) = (m) >= 4? (m) + ((m)>>1) : 16; \
		(a) = Krealloc(km, type, (a), (m)); \
	} while (0)

#define KMALLOC(km, ptr, len) ((ptr) = (__typeof__(ptr))kmalloc((km), (len) * sizeof(*(ptr))))
#define KCALLOC(km, ptr, len) ((ptr) = (__typeof__(ptr))kcalloc((km), (len), sizeof(*(ptr))))
#define KREALLOC(km, ptr, len) ((ptr) = (__typeof__(ptr))krealloc((km), (ptr), (len) * sizeof(*(ptr))))

#define KEXPAND(km, a, m) do { \
		(m) = (m) >= 4? (m) + ((m)>>1) : 16; \
		KREALLOC((km), (a), (m)); \
	} while (0)

#ifndef klib_unused
#if (defined __clang__ && __clang_major__ >= 3) || (defined __GNUC__ && __GNUC__ >= 3)
#define klib_unused __attribute__ ((__unused__))
#else
#define klib_unused
#endif
#endif /* klib_unused */

#define KALLOC_POOL_INIT2(SCOPE, name, kmptype_t) \
	typedef struct { \
		size_t cnt, n, max; \
		kmptype_t **buf; \
		void *km; \
	} kmp_##name##_t; \
	SCOPE kmp_##name##_t *kmp_init_##name(void *km) { \
		kmp_##name##_t *mp; \
		mp = Kcalloc(km, kmp_##name##_t, 1); \
		mp->km = km; \
		return mp; \
	} \
	SCOPE void kmp_destroy_##name(kmp_##name##_t *mp) { \
		size_t k; \
		for (k = 0; k < mp->n; ++k) kfree(mp->km, mp->buf[k]); \
		kfree(mp->km, mp->buf); kfree(mp->km, mp); \
	} \
	SCOPE kmptype_t *kmp_alloc_##name(kmp_##name##_t *mp) { \
		++mp->cnt; \
		if (mp->n == 0) return (kmptype_t*)kcalloc(mp->km, 1, sizeof(kmptype_t)); \
		return mp->buf[--mp->n]; \
	} \
	SCOPE void kmp_free_##name(kmp_##name##_t *mp, kmptype_t *p) { \
		--mp->cnt; \
		if (mp->n == mp->max) Kexpand(mp->km, kmptype_t*, mp->buf, mp->max); \
		mp->buf[mp->n++] = p; \
	}

#define KALLOC_POOL_INIT(name, kmptype_t) \
	KALLOC_POOL_INIT2(static inline klib_unused, name, kmptype_t)

#endif
