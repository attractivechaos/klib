#ifndef AC_KMEMPOOL_H
#define AC_KMEMPOOL_H

#ifdef __cplusplus
extern "C" {
#endif

void *kmp_init(unsigned sz);
void kmp_destroy(void *mp);
void *kmp_alloc(void *mp);
void kmp_free(void *mp, void *p);

#ifdef __cplusplus
}
#endif

#endif
