// see skhash.h for commentary and the end of this file for license

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h> // debug

#include "skhash.h"

#define __ac_isempty(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&2)
#define __ac_isdel(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&1)
#define __ac_iseither(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&3)
#define __ac_set_isdel_false(flag, i) (flag[i>>4]&=~(1ul<<((i&0xfU)<<1)))
#define __ac_set_isempty_false(flag, i) (flag[i>>4]&=~(2ul<<((i&0xfU)<<1)))
#define __ac_set_isboth_false(flag, i) (flag[i>>4]&=~(3ul<<((i&0xfU)<<1)))
#define __ac_set_isdel_true(flag, i) (flag[i>>4]|=1ul<<((i&0xfU)<<1))

#define __ac_fsize(m) ((m) < 16? 1 : (m)>>4)

#ifndef kroundup32
#define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#endif

#define MAXSIZE 1024 // maximum size of key or value
static char tmp[MAXSIZE];   // buffer for temporary keys

static const double __ac_HASH_UPPER = 0.77;

// some hash and equalty functions

#define DEF_SKH_KEY(name, size) \
  uint32_t skh_hash_##name(struct skh *h, const char *key)		\
  {									\
    uint32_t hash = key[0];						\
    uint32_t loops = size - 1;						\
    while (--loops) { hash = (hash << 5) - hash + (uint32_t)*++key; }	\
    return hash;							\
  }									\
  bool skh_equal_##name(struct skh *h, const char *key1, const char *key2) \
  {									\
    return memcmp(key1, key2, size) == 0;				\
  }

// Generic for all key sizes.
DEF_SKH_KEY(generic, h->keysize)
// Specialized for individual key sizes.
// Expect GCC to unroll the constant-length loops.
DEF_SKH_KEY(16, 16)
DEF_SKH_KEY(32, 32)

// utilities
//
// note: these generic memcpy's could be replaced by specialized
// constant-size memcpy's called via function pointer. I don't know if
// that would be faster or more in the spirit of khash. -lukego
static inline void copy_key(struct skh *h, char *dst, const char *src) {
  memcpy(dst, src, h->keysize);
}
static inline void copy_val(struct skh *h, char *dst, const char *src) {
  memcpy(dst, src, h->valuesize);
}

static inline char *key(struct skh *h, int i) { 
  return &h->keys[i*h->keysize];
}

static inline char *val(struct skh *h, int i) {
  return &h->vals[i*h->valuesize];
}

struct skh *skh_init(uint32_t keysize,
		     uint32_t valuesize,
		     uint32_t (*hash)(struct skh *h, const char *key),
		     bool (*equal)(struct skh *h, const char *key1, const char *key2))
{
  assert(keysize <= MAXSIZE);
  assert(valuesize <= MAXSIZE);
  struct skh *h = (struct skh *)calloc(1, sizeof(struct skh));
  h->keysize = keysize;
  h->valuesize = valuesize;
  h->hash = (hash == NULL) ? skh_hash_generic : hash;
  h->equal = (equal == NULL) ? skh_equal_generic : equal;
  return h;
}

void skh_destroy(struct skh *h)
{
  if (h) {
    free(h->keys);
    free(h->flags);
    free(h->vals);
    free(h);
  }
}

void skh_clear(struct skh *h)
{
  if (h && h->flags) {
    memset(h->flags, 0xaa, __ac_fsize(h->n_buckets) * sizeof(uint32_t));
    h->size = h->n_occupied = 0;
  }
}

char* skh_get(struct skh *h, const char *key1)
{
  if (h->n_buckets) {
    uint32_t k, i, last, mask, step = 0;
    mask = h->n_buckets - 1;
    k = h->hash(h, key1);
    i = k & mask;
    last = i;
    while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || !h->equal(h, key(h,i), key1))) {
      i = (i + (++step)) & mask;
      if (i == last) return NULL;
    }
    return __ac_iseither(h->flags, i)? NULL : val(h,i);
  } else return NULL;
}

int skh_resize(struct skh *h, int32_t new_n_buckets)
{ /* This function uses 0.25*n_buckets bytes of working space instead of [sizeof(key_t+val_t)+.25]*n_buckets. */
  printf("resize\n");
  uint32_t *new_flags = 0;
  uint32_t j = 1;
  {
    kroundup32(new_n_buckets);
    if (new_n_buckets < 4) new_n_buckets = 4;
    if (h->size >= (uint32_t)(new_n_buckets * __ac_HASH_UPPER + 0.5)) j = 0;	/* requested size is too small */
    else { /* hash table size to be changed (shrink or expand); rehash */
      new_flags = (uint32_t*)malloc(__ac_fsize(new_n_buckets) * sizeof(uint32_t));
      if (!new_flags) return -1;
      memset(new_flags, 0xaa, __ac_fsize(new_n_buckets) * sizeof(uint32_t));
      if (h->n_buckets < new_n_buckets) {	/* expand */
	char *new_keys = (char *)realloc((void *)h->keys, new_n_buckets * h->keysize);
	if (!new_keys) { free(new_flags); return -1; }
	h->keys = new_keys;
	if (h->valuesize) {
	  char *new_vals = (char*)realloc((void *)h->vals, new_n_buckets * h->valuesize);
	  if (!new_vals) { free(new_flags); return -1; }
	  h->vals = new_vals;
	}
      } /* otherwise shrink */
    }
  }
  if (j) { /* rehashing is needed */
    for (j = 0; j != h->n_buckets; ++j) {
      if (__ac_iseither(h->flags, j) == 0) {
	char *key1 = key(h,j);
	char *val1;
	uint32_t new_mask;
	new_mask = new_n_buckets - 1;
	if (h->valuesize) val1 = val(h,j);
	__ac_set_isdel_true(h->flags, j);
	while (1) { /* kick-out process; sort of like in Cuckoo hashing */
	  uint32_t k, i, step = 0;
	  k = h->hash(h, key1);
	  i = k & new_mask;
	  while (!__ac_isempty(new_flags, i)) i = (i + (++step)) & new_mask;
	  __ac_set_isempty_false(new_flags, i);
	  if (i < h->n_buckets && __ac_iseither(h->flags, i) == 0) { /* kick out the existing element */
	    copy_key(h, tmp, key(h,i));
	    copy_key(h, key(h,i), key1);
	    copy_key(h, key1, tmp);
	    if (h->valuesize) { 
	      copy_val(h, tmp, val(h,i));
	      copy_val(h, val(h,i), val1);
	      copy_val(h, val1, tmp);
	    }
	    __ac_set_isdel_true(h->flags, i); /* mark it as deleted in the old hash table */
	  } else { /* write the element and jump out of the loop */
	    copy_key(h, key(h,i), key1);
	    if (h->valuesize) copy_val(h, val(h,i), val1);
	    break;
	  }
	}
      }
    }
    if (h->n_buckets > new_n_buckets) { /* shrink the hash table */
      h->keys = (char *)realloc((void *)h->keys, new_n_buckets * h->keysize);
      if (h->valuesize) h->vals = (char *)realloc((void *)h->vals, new_n_buckets * h->valuesize);
    }
    free(h->flags); /* free the working space */
    h->flags = new_flags;
    h->n_buckets = new_n_buckets;
    h->n_occupied = h->size;
    h->upper_bound = (uint32_t)(h->n_buckets * __ac_HASH_UPPER + 0.5);
  }
  return 0;
}

char* skh_put(struct skh *h, const char *key1, int *ret)
{
  uint32_t x;
  int default_ret;
  ret = (ret == NULL) ? &default_ret : ret;
  if (h->n_occupied >= h->upper_bound) { /* update the hash table */
    if (h->n_buckets > (h->size<<1)) {
      if (skh_resize(h, h->n_buckets - 1) < 0) { /* clear "deleted" elements */
	*ret = -1; return NULL;
      }
    } else if (skh_resize(h, h->n_buckets + 1) < 0) { /* expand the hash table */
      *ret = -1; return NULL;
    }
  } /* TODO: to implement automatically shrinking; resize() already support shrinking */
  {
    uint32_t k, i, site, last, mask = h->n_buckets - 1, step = 0;
    x = site = h->n_buckets; k = h->hash(h, key1); i = k & mask;
    if (__ac_isempty(h->flags, i)) x = i; /* for speed up */
    else {
      last = i;
      while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || !h->equal(h, &h->keys[i*h->keysize], key1))) {
	if (__ac_isdel(h->flags, i)) site = i;
	i = (i + (++step)) & mask;
	if (i == last) { x = site; break; }
      }
      if (x == h->n_buckets) {
	if (__ac_isempty(h->flags, i) && site != h->n_buckets) x = site;
	else x = i;
      }
    }
  }
  if (__ac_isempty(h->flags, x)) { /* not present at all */
    copy_key(h, &h->keys[x*h->keysize], key1);
    __ac_set_isboth_false(h->flags, x);
    ++h->size; ++h->n_occupied;
    *ret = 1;
  } else if (__ac_isdel(h->flags, x)) { /* deleted */
    copy_key(h, &h->keys[x*h->keysize], key1);
    __ac_set_isboth_false(h->flags, x);
    ++h->size;
    *ret = 2;
  } else *ret = 0; /* Don't touch h->keys[x] if present and not deleted */
  return &h->vals[x*h->valuesize];
}

void skh_del(struct skh *h, uint32_t x)
{
  if (x != h->n_buckets && !__ac_iseither(h->flags, x)) {
    __ac_set_isdel_true(h->flags, x);
    --h->size;
  }
}

/* The MIT License

   Copyright (c) 2008, 2009, 2011 by Attractive Chaos <attractor@live.co.uk>
   Copyright (c) 2015 Luke Gorrie <luke@snabb.co>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
