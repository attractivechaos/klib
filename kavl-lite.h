/* The MIT License

   Copyright (c) 2021 by Attractive Chaos <attractor@live.co.uk>

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

/* An example:

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kavl-lite.h"

struct my_node {
  char key;
  KAVLL_HEAD(struct my_node) head;
};
#define my_cmp(p, q) (((q)->key < (p)->key) - ((p)->key < (q)->key))
KAVLL_INIT(my, struct my_node, head, my_cmp)

int main(void) {
  const char *str = "MNOLKQOPHIA"; // from wiki, except a duplicate
  struct my_node *root = 0;
  int i, l = strlen(str);
  for (i = 0; i < l; ++i) {        // insert in the input order
    struct my_node *q, *p = malloc(sizeof(*p));
    p->key = str[i];
    q = my_insert(&root, p);
    if (p != q) free(p);           // if already present, free
  }
  my_itr_t itr;
  my_itr_first(root, &itr);  // place at first
  do {                             // traverse
    const struct my_node *p = kavll_at(&itr);
    putchar(p->key);
    free((void*)p);                // free node
  } while (my_itr_next(&itr));
  putchar('\n');
  return 0;
}
*/

#ifndef KAVL_LITE_H
#define KAVL_LITE_H

#ifdef __STRICT_ANSI__
#define inline __inline__
#endif

#define KAVLL_MAX_DEPTH 64

#define KAVLL_HEAD(__type) \
	struct { \
		__type *p[2]; \
		signed char balance; /* balance factor */ \
	}

#define __KAVLL_FIND(pre, __scope, __type, __head,  __cmp) \
	__scope __type *pre##_find(const __type *root, const __type *x) { \
		const __type *p = root; \
		while (p != 0) { \
			int cmp; \
			cmp = __cmp(x, p); \
			if (cmp < 0) p = p->__head.p[0]; \
			else if (cmp > 0) p = p->__head.p[1]; \
			else break; \
		} \
		return (__type*)p; \
	}

#define __KAVLL_ROTATE(pre, __type, __head) \
	/* one rotation: (a,(b,c)q)p => ((a,b)p,c)q */ \
	static inline __type *pre##_rotate1(__type *p, int dir) { /* dir=0 to left; dir=1 to right */ \
		int opp = 1 - dir; /* opposite direction */ \
		__type *q = p->__head.p[opp]; \
		p->__head.p[opp] = q->__head.p[dir]; \
		q->__head.p[dir] = p; \
		return q; \
	} \
	/* two consecutive rotations: (a,((b,c)r,d)q)p => ((a,b)p,(c,d)q)r */ \
	static inline __type *pre##_rotate2(__type *p, int dir) { \
		int b1, opp = 1 - dir; \
		__type *q = p->__head.p[opp], *r = q->__head.p[dir]; \
		p->__head.p[opp] = r->__head.p[dir]; \
		r->__head.p[dir] = p; \
		q->__head.p[dir] = r->__head.p[opp]; \
		r->__head.p[opp] = q; \
		b1 = dir == 0? +1 : -1; \
		if (r->__head.balance == b1) q->__head.balance = 0, p->__head.balance = -b1; \
		else if (r->__head.balance == 0) q->__head.balance = p->__head.balance = 0; \
		else q->__head.balance = b1, p->__head.balance = 0; \
		r->__head.balance = 0; \
		return r; \
	}

#define __KAVLL_INSERT(pre, __scope, __type, __head, __cmp) \
	__scope __type *pre##_insert(__type **root_, __type *x) { \
		unsigned char stack[KAVLL_MAX_DEPTH]; \
		__type *path[KAVLL_MAX_DEPTH]; \
		__type *bp, *bq; \
		__type *p, *q, *r = 0; /* _r_ is potentially the new root */ \
		int which = 0, top, b1, path_len; \
		bp = *root_, bq = 0; \
		/* find the insertion location */ \
		for (p = bp, q = bq, top = path_len = 0; p; q = p, p = p->__head.p[which]) { \
			int cmp; \
			cmp = __cmp(x, p); \
			if (cmp == 0) return p; \
			if (p->__head.balance != 0) \
				bq = q, bp = p, top = 0; \
			stack[top++] = which = (cmp > 0); \
			path[path_len++] = p; \
		} \
		x->__head.balance = 0, x->__head.p[0] = x->__head.p[1] = 0; \
		if (q == 0) *root_ = x; \
		else q->__head.p[which] = x; \
		if (bp == 0) return x; \
		for (p = bp, top = 0; p != x; p = p->__head.p[stack[top]], ++top) /* update balance factors */ \
			if (stack[top] == 0) --p->__head.balance; \
			else ++p->__head.balance; \
		if (bp->__head.balance > -2 && bp->__head.balance < 2) return x; /* no re-balance needed */ \
		/* re-balance */ \
		which = (bp->__head.balance < 0); \
		b1 = which == 0? +1 : -1; \
		q = bp->__head.p[1 - which]; \
		if (q->__head.balance == b1) { \
			r = pre##_rotate1(bp, which); \
			q->__head.balance = bp->__head.balance = 0; \
		} else r = pre##_rotate2(bp, which); \
		if (bq == 0) *root_ = r; \
		else bq->__head.p[bp != bq->__head.p[0]] = r; \
		return x; \
	}

#define __KAVLL_ERASE(pre, __scope, __type, __head, __cmp) \
	__scope __type *pre##_erase(__type **root_, const __type *x) { \
		__type *p, *path[KAVLL_MAX_DEPTH], fake; \
		unsigned char dir[KAVLL_MAX_DEPTH]; \
		int d = 0, cmp; \
		fake.__head.p[0] = *root_, fake.__head.p[1] = 0; \
		if (x) { \
			for (cmp = -1, p = &fake; cmp; cmp = __cmp(x, p)) { \
				int which = (cmp > 0); \
				dir[d] = which; \
				path[d++] = p; \
				p = p->__head.p[which]; \
				if (p == 0) return 0; \
			} \
		} else { \
			for (p = &fake; p; p = p->__head.p[0]) \
				dir[d] = 0, path[d++] = p; \
			p = path[--d]; \
		} \
		if (p->__head.p[1] == 0) { /* ((1,.)2,3)4 => (1,3)4; p=2 */ \
			path[d-1]->__head.p[dir[d-1]] = p->__head.p[0]; \
		} else { \
			__type *q = p->__head.p[1]; \
			if (q->__head.p[0] == 0) { /* ((1,2)3,4)5 => ((1)2,4)5; p=3 */ \
				q->__head.p[0] = p->__head.p[0]; \
				q->__head.balance = p->__head.balance; \
				path[d-1]->__head.p[dir[d-1]] = q; \
				path[d] = q, dir[d++] = 1; \
			} else { /* ((1,((.,2)3,4)5)6,7)8 => ((1,(2,4)5)3,7)8; p=6 */ \
				__type *r; \
				int e = d++; /* backup _d_ */\
				for (;;) { \
					dir[d] = 0; \
					path[d++] = q; \
					r = q->__head.p[0]; \
					if (r->__head.p[0] == 0) break; \
					q = r; \
				} \
				r->__head.p[0] = p->__head.p[0]; \
				q->__head.p[0] = r->__head.p[1]; \
				r->__head.p[1] = p->__head.p[1]; \
				r->__head.balance = p->__head.balance; \
				path[e-1]->__head.p[dir[e-1]] = r; \
				path[e] = r, dir[e] = 1; \
			} \
		} \
		while (--d > 0) { \
			__type *q = path[d]; \
			int which, other, b1 = 1, b2 = 2; \
			which = dir[d], other = 1 - which; \
			if (which) b1 = -b1, b2 = -b2; \
			q->__head.balance += b1; \
			if (q->__head.balance == b1) break; \
			else if (q->__head.balance == b2) { \
				__type *r = q->__head.p[other]; \
				if (r->__head.balance == -b1) { \
					path[d-1]->__head.p[dir[d-1]] = pre##_rotate2(q, which); \
				} else { \
					path[d-1]->__head.p[dir[d-1]] = pre##_rotate1(q, which); \
					if (r->__head.balance == 0) { \
						r->__head.balance = -b1; \
						q->__head.balance = b1; \
						break; \
					} else r->__head.balance = q->__head.balance = 0; \
				} \
			} \
		} \
		*root_ = fake.__head.p[0]; \
		return p; \
	}

#define kavll_free(__type, __head, __root, __free) do { \
		__type *_p, *_q; \
		for (_p = __root; _p; _p = _q) { \
			if (_p->__head.p[0] == 0) { \
				_q = _p->__head.p[1]; \
				__free(_p); \
			} else { \
				_q = _p->__head.p[0]; \
				_p->__head.p[0] = _q->__head.p[1]; \
				_q->__head.p[1] = _p; \
			} \
		} \
	} while (0)

#define kavll_size(__type, __head, __root, __cnt) do { \
		__type *_p, *_q; \
		*(__cnt) = 0; \
		for (_p = __root; _p; _p = _q) { \
			if (_p->__head.p[0] == 0) { \
				_q = _p->__head.p[1]; \
				++*(__cnt); \
			} else { \
				_q = _p->__head.p[0]; \
				_p->__head.p[0] = _q->__head.p[1]; \
				_q->__head.p[1] = _p; \
			} \
		} \
	} while (0)

#define __KAVLL_ITR(pre, __scope, __type, __head, __cmp) \
	typedef struct pre##_itr_t { \
		const __type *stack[KAVLL_MAX_DEPTH], **top, *right; /* _right_ points to the right child of *top */ \
	} pre##_itr_t; \
	__scope void pre##_itr_first(const __type *root, struct pre##_itr_t *itr) { \
		const __type *p; \
		for (itr->top = itr->stack - 1, p = root; p; p = p->__head.p[0]) \
			*++itr->top = p; \
		itr->right = (*itr->top)->__head.p[1]; \
	} \
	__scope int pre##_itr_find(const __type *root, const __type *x, struct pre##_itr_t *itr) { \
		const __type *p = root; \
		itr->top = itr->stack - 1; \
		while (p != 0) { \
			int cmp; \
			cmp = __cmp(x, p); \
			if (cmp < 0) *++itr->top = p, p = p->__head.p[0]; \
			else if (cmp > 0) p = p->__head.p[1]; \
			else break; \
		} \
		if (p) { \
			*++itr->top = p; \
			itr->right = p->__head.p[1]; \
			return 1; \
		} else if (itr->top >= itr->stack) { \
			itr->right = (*itr->top)->__head.p[1]; \
			return 0; \
		} else return 0; \
	} \
	__scope int pre##_itr_next(struct pre##_itr_t *itr) { \
		for (;;) { \
			const __type *p; \
			for (p = itr->right, --itr->top; p; p = p->__head.p[0]) \
				*++itr->top = p; \
			if (itr->top < itr->stack) return 0; \
			itr->right = (*itr->top)->__head.p[1]; \
			return 1; \
		} \
	}

#define kavll_at(itr) ((itr)->top < (itr)->stack? 0 : *(itr)->top)

#define KAVLL_INIT2(pre, __scope, __type, __head, __cmp) \
	__KAVLL_FIND(pre, __scope, __type, __head,  __cmp) \
	__KAVLL_ROTATE(pre, __type, __head) \
	__KAVLL_INSERT(pre, __scope, __type, __head, __cmp) \
	__KAVLL_ERASE(pre, __scope, __type, __head, __cmp) \
	__KAVLL_ITR(pre, __scope, __type, __head, __cmp)

#define KAVLL_INIT(pre, __type, __head, __cmp) \
	KAVLL_INIT2(pre,, __type, __head, __cmp)

#endif
