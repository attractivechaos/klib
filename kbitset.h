/* The MIT License

   Copyright (C) 2015 Genome Research Ltd.

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

#ifndef KBITSET_H
#define KBITSET_H

/* Example of using kbitset_t, which represents a subset of {0,..., N-1},
   where N is the size specified in kbs_init().

	kbitset_t *bset = kbs_init(100);
	kbs_insert(bset, 5);
	kbs_insert(bset, 68);
	kbs_delete(bset, 37);
	// ...

	if (kbs_exists(bset, 68)) printf("68 present\n");

	kbitset_iter_t itr;
	int i;
	kbs_start(&itr);
	while ((i = kbs_next(bset, &itr)) >= 0)
		printf("%d present\n", i);

	kbs_destroy(bset);

   Example of declaring a kbitset_t-using function in a header file, so that
   only source files that actually use process() need to include <kbitset.h>:

	struct kbitset_t;
	void process(struct kbitset_t *bset);
*/

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define KBS_ELTBITS (CHAR_BIT * sizeof (unsigned long))
#define KBS_ELT(i)  ((i) / KBS_ELTBITS)
#define KBS_MASK(i) (1UL << ((i) % KBS_ELTBITS))

typedef struct kbitset_t {
	size_t n, n_max;
	unsigned long b[1];
} kbitset_t;

// (For internal use only.) Returns a mask (like 00011111) showing
// which bits are in use in the last slot (for the given ni) set.
static inline unsigned long kbs_last_mask(size_t ni)
{
	unsigned long mask = KBS_MASK(ni) - 1;
	return mask? mask : ~0UL;
}

// Initialise a bit set capable of holding ni integers, 0 <= i < ni.
// The set returned is empty if fill == 0, or all of [0,ni) otherwise.
static inline kbitset_t *kbs_init2(size_t ni, int fill)
{
	size_t n = (ni + KBS_ELTBITS-1) / KBS_ELTBITS;
	kbitset_t *bs =
		(kbitset_t *) malloc(sizeof(kbitset_t) + n * sizeof(unsigned long));
	if (bs == NULL) return NULL;
	bs->n = bs->n_max = n;
	memset(bs->b, fill? ~0 : 0, n * sizeof (unsigned long));
	// b[n] is always non-zero (a fact used by kbs_next()).
	bs->b[n] = kbs_last_mask(ni);
	if (fill) bs->b[n-1] &= bs->b[n];
	return bs;
}

// Initialise an empty bit set capable of holding ni integers, 0 <= i < ni.
static inline kbitset_t *kbs_init(size_t ni)
{
	return kbs_init2(ni, 0);
}

// Resize an existing bit set to be capable of holding ni_new integers.
// Elements in [ni_old,ni_new) are added to the set if fill != 0.
static inline int kbs_resize2(kbitset_t **bsp, size_t ni_new, int fill)
{
	kbitset_t *bs = *bsp;
	size_t n = bs? bs->n : 0;
	size_t n_new = (ni_new + KBS_ELTBITS-1) / KBS_ELTBITS;
	if (bs == NULL || n_new > bs->n_max) {
		bs = (kbitset_t *)
			realloc(bs, sizeof(kbitset_t) + n_new * sizeof(unsigned long));
		if (bs == NULL) return -1;

		bs->n_max = n_new;
		*bsp = bs;
	}

	bs->n = n_new;
	if (n_new >= n)
		memset(&bs->b[n], fill? ~0 : 0, (n_new - n) * sizeof (unsigned long));
	bs->b[n_new] = kbs_last_mask(ni_new);
	// Need to clear excess bits when fill!=0 or n_new<n; always is simpler.
	bs->b[n_new-1] &= bs->b[n_new];
	return 0;
}

// Resize an existing bit set to be capable of holding ni_new integers.
// Returns negative on error.
static inline int kbs_resize(kbitset_t **bsp, size_t ni_new)
{
	return kbs_resize2(bsp, ni_new, 0);
}

// Destroy a bit set.
static inline void kbs_destroy(kbitset_t *bs)
{
	free(bs);
}

// Reset the bit set to empty.
static inline void kbs_clear(kbitset_t *bs)
{
	memset(bs->b, 0, bs->n * sizeof (unsigned long));
}

// Reset the bit set to all of [0,ni).
static inline void kbs_insert_all(kbitset_t *bs)
{
	memset(bs->b, ~0, bs->n * sizeof (unsigned long));
	bs->b[bs->n-1] &= bs->b[bs->n];
}

// Insert an element into the bit set.
static inline void kbs_insert(kbitset_t *bs, int i)
{
	bs->b[KBS_ELT(i)] |= KBS_MASK(i);
}

// Remove an element from the bit set.
static inline void kbs_delete(kbitset_t *bs, int i)
{
	bs->b[KBS_ELT(i)] &= ~KBS_MASK(i);
}

// Test whether the bit set contains the element.
static inline int kbs_exists(const kbitset_t *bs, int i)
{
	return (bs->b[KBS_ELT(i)] & KBS_MASK(i)) != 0;
}

typedef struct kbitset_iter_t {
	unsigned long mask;
	size_t elt;
	int i;
} kbitset_iter_t;

// Initialise or reset a bit set iterator.
static inline void kbs_start(kbitset_iter_t *itr)
{
	itr->mask = 1;
	itr->elt = 0;
	itr->i = 0;
}

// Return the next element contained in the bit set, or -1 if there are no more.
static inline int kbs_next(const kbitset_t *bs, kbitset_iter_t *itr)
{
	unsigned long b = bs->b[itr->elt];

	for (;;) {
		if (itr->mask == 0) {
			while ((b = bs->b[++itr->elt]) == 0) itr->i += KBS_ELTBITS;
			if (itr->elt == bs->n) return -1;
			itr->mask = 1;
		}

		if (b & itr->mask) break;

		itr->i++;
		itr->mask <<= 1;
	}

	itr->mask <<= 1;
	return itr->i++;
}

#endif
