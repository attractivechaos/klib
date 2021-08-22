/* The MIT License

   Copyright (c) 2019 by Attractive Chaos <attractor@live.co.uk>

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

#ifndef __AC_KHASHL_H
#define __AC_KHASHL_H

#define AC_VERSION_KHASHL_H "0.1"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

/************************************
 * Compiler specific configurations *
 ************************************/

#if UINT_MAX == 0xffffffffu
typedef unsigned int khint32_t;
#elif ULONG_MAX == 0xffffffffu
typedef unsigned long khint32_t;
#endif

#if ULONG_MAX == ULLONG_MAX
typedef unsigned long khint64_t;
#else
typedef unsigned long long khint64_t;
#endif

#ifndef kh_inline
#ifdef _MSC_VER
#define kh_inline __inline
#else
#define kh_inline inline
#endif
#endif /* kh_inline */

#ifndef klib_unused
#if (defined __clang__ && __clang_major__ >= 3) || (defined __GNUC__ && __GNUC__ >= 3)
#define klib_unused __attribute__ ((__unused__))
#else
#define klib_unused
#endif
#endif /* klib_unused */

#define KH_LOCAL static kh_inline klib_unused

typedef khint32_t khint_t;

/******************
 * malloc aliases *
 ******************/

#ifndef kcalloc
#define kcalloc(N,Z) calloc(N,Z)
#endif
#ifndef kmalloc
#define kmalloc(Z) malloc(Z)
#endif
#ifndef krealloc
#define krealloc(P,Z) realloc(P,Z)
#endif
#ifndef kfree
#define kfree(P) free(P)
#endif

/****************************
 * Simple private functions *
 ****************************/

#define __kh_used(flag, i)       (flag[i>>5] >> (i&0x1fU) & 1U)
#define __kh_set_used(flag, i)   (flag[i>>5] |= 1U<<(i&0x1fU))
#define __kh_set_unused(flag, i) (flag[i>>5] &= ~(1U<<(i&0x1fU)))

#define __kh_fsize(m) ((m) < 32? 1 : (m)>>5)

static kh_inline khint_t __kh_h2b(khint_t hash, khint_t bits) { return hash * 2654435769U >> (32 - bits); }

/*******************
 * Hash table base *
 *******************/

#define __KHASHL_TYPE(HType, khkey_t) \
	typedef struct HType { \
		khint_t bits, count; \
		khint32_t *used; \
		khkey_t *keys; \
	} HType;

#define __KHASHL_PROTOTYPES(HType, prefix, khkey_t) \
	extern HType *prefix##_init(void); \
	extern void prefix##_destroy(HType *h); \
	extern void prefix##_clear(HType *h); \
	extern khint_t prefix##_getp(const HType *h, const khkey_t *key); \
	extern int prefix##_resize(HType *h, khint_t new_n_buckets); \
	extern khint_t prefix##_putp(HType *h, const khkey_t *key, int *absent); \
	extern void prefix##_del(HType *h, khint_t k);

#define __KHASHL_IMPL_BASIC(SCOPE, HType, prefix) \
	SCOPE HType *prefix##_init(void) { \
		return (HType*)kcalloc(1, sizeof(HType)); \
	} \
	SCOPE void prefix##_destroy(HType *h) { \
		if (!h) return; \
		kfree((void *)h->keys); kfree(h->used); \
		kfree(h); \
	} \
	SCOPE void prefix##_clear(HType *h) { \
		if (h && h->used) { \
			uint32_t n_buckets = 1U << h->bits; \
			memset(h->used, 0, __kh_fsize(n_buckets) * sizeof(khint32_t)); \
			h->count = 0; \
		} \
	}

#define __KHASHL_IMPL_GET(SCOPE, HType, prefix, khkey_t, __hash_fn, __hash_eq) \
	SCOPE khint_t prefix##_getp(const HType *h, const khkey_t *key) { \
		khint_t i, last, n_buckets, mask; \
		if (h->keys == 0) return 0; \
		n_buckets = 1U << h->bits; \
		mask = n_buckets - 1U; \
		i = last = __kh_h2b(__hash_fn(*key), h->bits); \
		while (__kh_used(h->used, i) && !__hash_eq(h->keys[i], *key)) { \
			i = (i + 1U) & mask; \
			if (i == last) return n_buckets; \
		} \
		return !__kh_used(h->used, i)? n_buckets : i; \
	} \
	SCOPE khint_t prefix##_get(const HType *h, khkey_t key) { return prefix##_getp(h, &key); }

#define __KHASHL_IMPL_RESIZE(SCOPE, HType, prefix, khkey_t, __hash_fn, __hash_eq) \
	SCOPE int prefix##_resize(HType *h, khint_t new_n_buckets) { \
		khint32_t *new_used = 0; \
		khint_t j = 0, x = new_n_buckets, n_buckets, new_bits, new_mask; \
		while ((x >>= 1) != 0) ++j; \
		if (new_n_buckets & (new_n_buckets - 1)) ++j; \
		new_bits = j > 2? j : 2; \
		new_n_buckets = 1U << new_bits; \
		if (h->count > (new_n_buckets>>1) + (new_n_buckets>>2)) return 0; /* requested size is too small */ \
		new_used = (khint32_t*)kmalloc(__kh_fsize(new_n_buckets) * sizeof(khint32_t)); \
		memset(new_used, 0, __kh_fsize(new_n_buckets) * sizeof(khint32_t)); \
		if (!new_used) return -1; /* not enough memory */ \
		n_buckets = h->keys? 1U<<h->bits : 0U; \
		if (n_buckets < new_n_buckets) { /* expand */ \
			khkey_t *new_keys = (khkey_t*)krealloc((void*)h->keys, new_n_buckets * sizeof(khkey_t)); \
			if (!new_keys) { kfree(new_used); return -1; } \
			h->keys = new_keys; \
		} /* otherwise shrink */ \
		new_mask = new_n_buckets - 1; \
		for (j = 0; j != n_buckets; ++j) { \
			khkey_t key; \
			if (!__kh_used(h->used, j)) continue; \
			key = h->keys[j]; \
			__kh_set_unused(h->used, j); \
			while (1) { /* kick-out process; sort of like in Cuckoo hashing */ \
				khint_t i; \
				i = __kh_h2b(__hash_fn(key), new_bits); \
				while (__kh_used(new_used, i)) i = (i + 1) & new_mask; \
				__kh_set_used(new_used, i); \
				if (i < n_buckets && __kh_used(h->used, i)) { /* kick out the existing element */ \
					{ khkey_t tmp = h->keys[i]; h->keys[i] = key; key = tmp; } \
					__kh_set_unused(h->used, i); /* mark it as deleted in the old hash table */ \
				} else { /* write the element and jump out of the loop */ \
					h->keys[i] = key; \
					break; \
				} \
			} \
		} \
		if (n_buckets > new_n_buckets) /* shrink the hash table */ \
			h->keys = (khkey_t*)krealloc((void *)h->keys, new_n_buckets * sizeof(khkey_t)); \
		kfree(h->used); /* free the working space */ \
		h->used = new_used, h->bits = new_bits; \
		return 0; \
	}

#define __KHASHL_IMPL_PUT(SCOPE, HType, prefix, khkey_t, __hash_fn, __hash_eq) \
	SCOPE khint_t prefix##_putp(HType *h, const khkey_t *key, int *absent) { \
		khint_t n_buckets, i, last, mask; \
		n_buckets = h->keys? 1U<<h->bits : 0U; \
		*absent = -1; \
		if (h->count >= (n_buckets>>1) + (n_buckets>>2)) { /* rehashing */ \
			if (prefix##_resize(h, n_buckets + 1U) < 0) \
				return n_buckets; \
			n_buckets = 1U<<h->bits; \
		} /* TODO: to implement automatically shrinking; resize() already support shrinking */ \
		mask = n_buckets - 1; \
		i = last = __kh_h2b(__hash_fn(*key), h->bits); \
		while (__kh_used(h->used, i) && !__hash_eq(h->keys[i], *key)) { \
			i = (i + 1U) & mask; \
			if (i == last) break; \
		} \
		if (!__kh_used(h->used, i)) { /* not present at all */ \
			h->keys[i] = *key; \
			__kh_set_used(h->used, i); \
			++h->count; \
			*absent = 1; \
		} else *absent = 0; /* Don't touch h->keys[i] if present */ \
		return i; \
	} \
	SCOPE khint_t prefix##_put(HType *h, khkey_t key, int *absent) { return prefix##_putp(h, &key, absent); }

#define __KHASHL_IMPL_DEL(SCOPE, HType, prefix, khkey_t, __hash_fn) \
	SCOPE int prefix##_del(HType *h, khint_t i) { \
		khint_t j = i, k, mask, n_buckets; \
		if (h->keys == 0) return 0; \
		n_buckets = 1U<<h->bits; \
		mask = n_buckets - 1U; \
		while (1) { \
			j = (j + 1U) & mask; \
			if (j == i || !__kh_used(h->used, j)) break; /* j==i only when the table is completely full */ \
			k = __kh_h2b(__hash_fn(h->keys[j]), h->bits); \
			if ((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j))) \
				h->keys[i] = h->keys[j], i = j; \
		} \
		__kh_set_unused(h->used, i); \
		--h->count; \
		return 1; \
	}

#define KHASHL_DECLARE(HType, prefix, khkey_t) \
	__KHASHL_TYPE(HType, khkey_t) \
	__KHASHL_PROTOTYPES(HType, prefix, khkey_t)

#define KHASHL_INIT(SCOPE, HType, prefix, khkey_t, __hash_fn, __hash_eq) \
	__KHASHL_TYPE(HType, khkey_t) \
	__KHASHL_IMPL_BASIC(SCOPE, HType, prefix) \
	__KHASHL_IMPL_GET(SCOPE, HType, prefix, khkey_t, __hash_fn, __hash_eq) \
	__KHASHL_IMPL_RESIZE(SCOPE, HType, prefix, khkey_t, __hash_fn, __hash_eq) \
	__KHASHL_IMPL_PUT(SCOPE, HType, prefix, khkey_t, __hash_fn, __hash_eq) \
	__KHASHL_IMPL_DEL(SCOPE, HType, prefix, khkey_t, __hash_fn)

/*****************************
 * More convenient interface *
 *****************************/

#define __kh_packed __attribute__ ((__packed__))
#define __kh_cached_hash(x) ((x).hash)

#define KHASHL_SET_INIT(SCOPE, HType, prefix, khkey_t, __hash_fn, __hash_eq) \
	typedef struct { khkey_t key; } __kh_packed HType##_s_bucket_t; \
	static kh_inline khint_t prefix##_s_hash(HType##_s_bucket_t x) { return __hash_fn(x.key); } \
	static kh_inline int prefix##_s_eq(HType##_s_bucket_t x, HType##_s_bucket_t y) { return __hash_eq(x.key, y.key); } \
	KHASHL_INIT(KH_LOCAL, HType, prefix##_s, HType##_s_bucket_t, prefix##_s_hash, prefix##_s_eq) \
	SCOPE HType *prefix##_init(void) { return prefix##_s_init(); } \
	SCOPE void prefix##_destroy(HType *h) { prefix##_s_destroy(h); } \
	SCOPE void prefix##_resize(HType *h, khint_t new_n_buckets) { prefix##_s_resize(h, new_n_buckets); } \
	SCOPE khint_t prefix##_get(const HType *h, khkey_t key) { HType##_s_bucket_t t; t.key = key; return prefix##_s_getp(h, &t); } \
	SCOPE int prefix##_del(HType *h, khint_t k) { return prefix##_s_del(h, k); } \
	SCOPE khint_t prefix##_put(HType *h, khkey_t key, int *absent) { HType##_s_bucket_t t; t.key = key; return prefix##_s_putp(h, &t, absent); }

#define KHASHL_MAP_INIT(SCOPE, HType, prefix, khkey_t, kh_val_t, __hash_fn, __hash_eq) \
	typedef struct { khkey_t key; kh_val_t val; } __kh_packed HType##_m_bucket_t; \
	static kh_inline khint_t prefix##_m_hash(HType##_m_bucket_t x) { return __hash_fn(x.key); } \
	static kh_inline int prefix##_m_eq(HType##_m_bucket_t x, HType##_m_bucket_t y) { return __hash_eq(x.key, y.key); } \
	KHASHL_INIT(KH_LOCAL, HType, prefix##_m, HType##_m_bucket_t, prefix##_m_hash, prefix##_m_eq) \
	SCOPE HType *prefix##_init(void) { return prefix##_m_init(); } \
	SCOPE void prefix##_destroy(HType *h) { prefix##_m_destroy(h); } \
	SCOPE khint_t prefix##_get(const HType *h, khkey_t key) { HType##_m_bucket_t t; t.key = key; return prefix##_m_getp(h, &t); } \
	SCOPE int prefix##_del(HType *h, khint_t k) { return prefix##_m_del(h, k); } \
	SCOPE khint_t prefix##_put(HType *h, khkey_t key, int *absent) { HType##_m_bucket_t t; t.key = key; return prefix##_m_putp(h, &t, absent); }

#define KHASHL_CSET_INIT(SCOPE, HType, prefix, khkey_t, __hash_fn, __hash_eq) \
	typedef struct { khkey_t key; khint_t hash; } __kh_packed HType##_cs_bucket_t; \
	static kh_inline int prefix##_cs_eq(HType##_cs_bucket_t x, HType##_cs_bucket_t y) { return x.hash == y.hash && __hash_eq(x.key, y.key); } \
	KHASHL_INIT(KH_LOCAL, HType, prefix##_cs, HType##_cs_bucket_t, __kh_cached_hash, prefix##_cs_eq) \
	SCOPE HType *prefix##_init(void) { return prefix##_cs_init(); } \
	SCOPE void prefix##_destroy(HType *h) { prefix##_cs_destroy(h); } \
	SCOPE khint_t prefix##_get(const HType *h, khkey_t key) { HType##_cs_bucket_t t; t.key = key; t.hash = __hash_fn(key); return prefix##_cs_getp(h, &t); } \
	SCOPE int prefix##_del(HType *h, khint_t k) { return prefix##_cs_del(h, k); } \
	SCOPE khint_t prefix##_put(HType *h, khkey_t key, int *absent) { HType##_cs_bucket_t t; t.key = key, t.hash = __hash_fn(key); return prefix##_cs_putp(h, &t, absent); }

#define KHASHL_CMAP_INIT(SCOPE, HType, prefix, khkey_t, kh_val_t, __hash_fn, __hash_eq) \
	typedef struct { khkey_t key; kh_val_t val; khint_t hash; } __kh_packed HType##_cm_bucket_t; \
	static kh_inline int prefix##_cm_eq(HType##_cm_bucket_t x, HType##_cm_bucket_t y) { return x.hash == y.hash && __hash_eq(x.key, y.key); } \
	KHASHL_INIT(KH_LOCAL, HType, prefix##_cm, HType##_cm_bucket_t, __kh_cached_hash, prefix##_cm_eq) \
	SCOPE HType *prefix##_init(void) { return prefix##_cm_init(); } \
	SCOPE void prefix##_destroy(HType *h) { prefix##_cm_destroy(h); } \
	SCOPE khint_t prefix##_get(const HType *h, khkey_t key) { HType##_cm_bucket_t t; t.key = key; t.hash = __hash_fn(key); return prefix##_cm_getp(h, &t); } \
	SCOPE int prefix##_del(HType *h, khint_t k) { return prefix##_cm_del(h, k); } \
	SCOPE khint_t prefix##_put(HType *h, khkey_t key, int *absent) { HType##_cm_bucket_t t; t.key = key, t.hash = __hash_fn(key); return prefix##_cm_putp(h, &t, absent); }

/**************************
 * Public macro functions *
 **************************/

#define kh_bucket(h, x) ((h)->keys[x])
#define kh_size(h) ((h)->count)
#define kh_capacity(h) ((h)->keys? 1U<<(h)->bits : 0U)
#define kh_end(h) kh_capacity(h)

#define kh_key(h, x) ((h)->keys[x].key)
#define kh_val(h, x) ((h)->keys[x].val)
#define kh_exist(h, x) __kh_used((h)->used, (x))

/**************************************
 * Common hash and equality functions *
 **************************************/

#define kh_eq_generic(a, b) ((a) == (b))
#define kh_eq_str(a, b) (strcmp((a), (b)) == 0)
#define kh_hash_dummy(x) ((khint_t)(x))

static kh_inline khint_t kh_hash_uint32(khint_t key) {
	key += ~(key << 15);
	key ^=  (key >> 10);
	key +=  (key << 3);
	key ^=  (key >> 6);
	key += ~(key << 11);
	key ^=  (key >> 16);
	return key;
}

static kh_inline khint_t kh_hash_uint64(khint64_t key) {
	key = ~key + (key << 21);
	key = key ^ key >> 24;
	key = (key + (key << 3)) + (key << 8);
	key = key ^ key >> 14;
	key = (key + (key << 2)) + (key << 4);
	key = key ^ key >> 28;
	key = key + (key << 31);
	return (khint_t)key;
}

static kh_inline khint_t kh_hash_str(const char *s) {
	khint_t h = (khint_t)*s;
	if (h) for (++s ; *s; ++s) h = (h << 5) - h + (khint_t)*s;
	return h;
}

#endif /* __AC_KHASHL_H */
