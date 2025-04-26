#ifndef AC_KMEMPOOL_H
#define AC_KMEMPOOL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kmempool_s kmempool_t;

kmempool_t *kmp_init(unsigned sz);
void kmp_destroy(kmempool_t *mp);
void *kmp_alloc(kmempool_t *mp);
void kmp_free(kmempool_t *mp, void *p);

#ifdef __cplusplus
}
#endif

#endif
