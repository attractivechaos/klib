#include <stdint.h>
#include <stdlib.h>
#include "kmempool.h"

#define Malloc(type, cnt)       ((type*)malloc((cnt) * sizeof(type)))
#define Calloc(type, cnt)       ((type*)calloc((cnt), sizeof(type)))
#define Realloc(type, ptr, cnt) ((type*)realloc((ptr), (cnt) * sizeof(type)))

typedef struct {
	uint32_t sz; // size of an entry
	uint32_t chunk_size; // number of entries in a chunk
	uint32_t n_chunk; // number of chunks
	uint8_t *p; // pointing to an entry that can be allocated from a chunk
	uint8_t *p_end; // end of the current chunk
	uint8_t **chunk; // list of chunks
	uint32_t n_buf, m_buf; // number/max number of buffered free entries
	void **buf; // list of pointers to free entries
} kmempool_t;

static void kmp_add_chunk(kmempool_t *mp) // add a new chunk
{
	uint64_t x = (uint64_t)mp->sz * mp->chunk_size;
	mp->chunk = Realloc(uint8_t*, mp->chunk, mp->n_chunk + 1);
	mp->p = mp->chunk[mp->n_chunk++] = Calloc(uint8_t, x);
	mp->p_end = mp->p + x;
}

void *kmp_init2(unsigned sz, unsigned chunk_size)
{
	kmempool_t *mp;
	mp = Calloc(kmempool_t, 1);
	mp->sz = sz, mp->chunk_size = chunk_size;
	kmp_add_chunk(mp);
	return mp;
}

void *kmp_init(unsigned sz) // fixed chunk size
{
	return kmp_init2(sz, 0x10000);
}

void kmp_destroy(void *mp_)
{
	kmempool_t *mp = (kmempool_t*)mp_;
	uint32_t i;
	for (i = 0; i < mp->n_chunk; ++i) // free all chunks
		free(mp->chunk[i]);
	free(mp->chunk); free(mp->buf); free(mp);
}

void *kmp_alloc(void *mp_)
{
	kmempool_t *mp = (kmempool_t*)mp_;
	void *ret;
	if (mp->n_buf > 0) { // there are buffered free entries
		ret = mp->buf[--mp->n_buf];
	} else { // need to "allocate" from chunks
		if (mp->p == mp->p_end) kmp_add_chunk(mp); // chunk full; add a new one
		ret = (void*)mp->p;
		mp->p += mp->sz;
	}
	return ret;
}

void kmp_free(void *mp_, void *p)
{
	kmempool_t *mp = (kmempool_t*)mp_;
	if (mp->n_buf == mp->m_buf) { // then enlarge the buffer
		mp->m_buf += (mp->m_buf>>1) + 16;
		mp->buf = Realloc(void*, mp->buf, mp->m_buf);
	}
	mp->buf[mp->n_buf++] = p;
}
