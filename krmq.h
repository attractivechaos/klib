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

/* An example:

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "krmq.h"

struct my_node {
  char key;
  KRMQ_HEAD(struct my_node) head;
};
#define my_cmp(p, q) (((q)->key < (p)->key) - ((p)->key < (q)->key))
KRMQ_INIT(my, struct my_node, head, my_cmp)

int main(void) {
  const char *str = "MNOLKQOPHIA"; // from wiki, except a duplicate
  struct my_node *root = 0;
  int i, l = strlen(str);
  for (i = 0; i < l; ++i) {        // insert in the input order
    struct my_node *q, *p = malloc(sizeof(*p));
    p->key = str[i];
    q = krmq_insert(my, &root, p, 0);
    if (p != q) free(p);           // if already present, free
  }
  krmq_itr_t(my) itr;
  krmq_itr_first(my, root, &itr);  // place at first
  do {                             // traverse
    const struct my_node *p = krmq_at(&itr);
    putchar(p->key);
    free((void*)p);                // free node
  } while (krmq_itr_next(my, &itr));
  putchar('\n');
  return 0;
}
*/

#ifndef KRMQ_H
#define KRMQ_H

#ifdef __STRICT_ANSI__
#define inline __inline__
#endif

#define KRMQ_MAX_DEPTH 64

#define krmq_size(head, p) ((p)? (p)->head.size : 0)
#define krmq_size_child(head, q, i) ((q)->head.p[(i)]? (q)->head.p[(i)]->head.size : 0)

#define KRMQ_HEAD(__type) \
	struct { \
		__type *p[2], *s; \
		signed char balance; /* balance factor */ \
		unsigned size; /* #elements in subtree */ \
	}

#define __KRMQ_FIND(suf, __scope, __type, __head,  __cmp) \
	__scope __type *krmq_find_##suf(const __type *root, const __type *x, unsigned *cnt_) { \
		const __type *p = root; \
		unsigned cnt = 0; \
		while (p != 0) { \
			int cmp; \
			cmp = __cmp(x, p); \
			if (cmp >= 0) cnt += krmq_size_child(__head, p, 0) + 1; \
			if (cmp < 0) p = p->__head.p[0]; \
			else if (cmp > 0) p = p->__head.p[1]; \
			else break; \
		} \
		if (cnt_) *cnt_ = cnt; \
		return (__type*)p; \
	} \
	__scope __type *krmq_interval_##suf(const __type *root, const __type *x, __type **lower, __type **upper) { \
		const __type *p = root, *l = 0, *u = 0; \
		while (p != 0) { \
			int cmp; \
			cmp = __cmp(x, p); \
			if (cmp < 0) u = p, p = p->__head.p[0]; \
			else if (cmp > 0) l = p, p = p->__head.p[1]; \
			else { l = u = p; break; } \
		} \
		if (lower) *lower = (__type*)l; \
		if (upper) *upper = (__type*)u; \
		return (__type*)p; \
	}

#define __KRMQ_RMQ(suf, __scope, __type, __head,  __cmp, __lt2) \
	__scope __type *krmq_rmq_##suf(const __type *root, const __type *lo, const __type *up) { /* CLOSED interval */ \
		const __type *p = root, *path[2][KRMQ_MAX_DEPTH], *min; \
		int plen[2] = {0, 0}, pcmp[2][KRMQ_MAX_DEPTH], i, cmp, lca; \
		if (root == 0) return 0; \
		while (p) { \
			cmp = __cmp(lo, p); \
			path[0][plen[0]] = p, pcmp[0][plen[0]++] = cmp; \
			if (cmp < 0) p = p->__head.p[0]; \
			else if (cmp > 0) p = p->__head.p[1]; \
			else break; \
		} \
		p = root; \
		while (p) { \
			cmp = __cmp(up, p); \
			path[1][plen[1]] = p, pcmp[1][plen[1]++] = cmp; \
			if (cmp < 0) p = p->__head.p[0]; \
			else if (cmp > 0) p = p->__head.p[1]; \
			else break; \
		} \
		for (i = 0; i < plen[0] && i < plen[1]; ++i) /* find the LCA */ \
			if (path[0][i] == path[1][i] && pcmp[0][i] <= 0 && pcmp[1][i] >= 0) \
				break; \
		if (i == plen[0] || i == plen[1]) return 0; /* no elements in the closed interval */ \
		lca = i, min = path[0][lca]; \
		for (i = lca + 1; i < plen[0]; ++i) { \
			if (pcmp[0][i] <= 0) { \
				if (__lt2(path[0][i], min)) min = path[0][i]; \
				if (path[0][i]->__head.p[1] && __lt2(path[0][i]->__head.p[1]->__head.s, min)) \
					min = path[0][i]->__head.p[1]->__head.s; \
			} \
		} \
		for (i = lca + 1; i < plen[1]; ++i) { \
			if (pcmp[1][i] >= 0) { \
				if (__lt2(path[1][i], min)) min = path[1][i]; \
				if (path[1][i]->__head.p[0] && __lt2(path[1][i]->__head.p[0]->__head.s, min)) \
					min = path[1][i]->__head.p[0]->__head.s; \
			} \
		} \
		return (__type*)min; \
	}

#define __KRMQ_ROTATE(suf, __type, __head, __lt2) \
	/* */ \
	static inline void krmq_update_min_##suf(__type *p, const __type *q, const __type *r) { \
		p->__head.s = !q || __lt2(p, q->__head.s)? p : q->__head.s; \
		p->__head.s = !r || __lt2(p->__head.s, r->__head.s)? p->__head.s : r->__head.s; \
	} \
	/* one rotation: (a,(b,c)q)p => ((a,b)p,c)q */ \
	static inline __type *krmq_rotate1_##suf(__type *p, int dir) { /* dir=0 to left; dir=1 to right */ \
		int opp = 1 - dir; /* opposite direction */ \
		__type *q = p->__head.p[opp], *s = p->__head.s; \
		unsigned size_p = p->__head.size; \
		p->__head.size -= q->__head.size - krmq_size_child(__head, q, dir); \
		q->__head.size = size_p; \
		krmq_update_min_##suf(p, p->__head.p[dir], q->__head.p[dir]); \
		q->__head.s = s; \
		p->__head.p[opp] = q->__head.p[dir]; \
		q->__head.p[dir] = p; \
		return q; \
	} \
	/* two consecutive rotations: (a,((b,c)r,d)q)p => ((a,b)p,(c,d)q)r */ \
	static inline __type *krmq_rotate2_##suf(__type *p, int dir) { \
		int b1, opp = 1 - dir; \
		__type *q = p->__head.p[opp], *r = q->__head.p[dir], *s = p->__head.s; \
		unsigned size_x_dir = krmq_size_child(__head, r, dir); \
		r->__head.size = p->__head.size; \
		p->__head.size -= q->__head.size - size_x_dir; \
		q->__head.size -= size_x_dir + 1; \
		krmq_update_min_##suf(p, p->__head.p[dir], r->__head.p[dir]); \
		krmq_update_min_##suf(q, q->__head.p[opp], r->__head.p[opp]); \
		r->__head.s = s; \
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

#define __KRMQ_INSERT(suf, __scope, __type, __head, __cmp, __lt2) \
	__scope __type *krmq_insert_##suf(__type **root_, __type *x, unsigned *cnt_) { \
		unsigned char stack[KRMQ_MAX_DEPTH]; \
		__type *path[KRMQ_MAX_DEPTH]; \
		__type *bp, *bq; \
		__type *p, *q, *r = 0; /* _r_ is potentially the new root */ \
		int i, which = 0, top, b1, path_len; \
		unsigned cnt = 0; \
		bp = *root_, bq = 0; \
		/* find the insertion location */ \
		for (p = bp, q = bq, top = path_len = 0; p; q = p, p = p->__head.p[which]) { \
			int cmp; \
			cmp = __cmp(x, p); \
			if (cmp >= 0) cnt += krmq_size_child(__head, p, 0) + 1; \
			if (cmp == 0) { \
				if (cnt_) *cnt_ = cnt; \
				return p; \
			} \
			if (p->__head.balance != 0) \
				bq = q, bp = p, top = 0; \
			stack[top++] = which = (cmp > 0); \
			path[path_len++] = p; \
		} \
		if (cnt_) *cnt_ = cnt; \
		x->__head.balance = 0, x->__head.size = 1, x->__head.p[0] = x->__head.p[1] = 0, x->__head.s = x; \
		if (q == 0) *root_ = x; \
		else q->__head.p[which] = x; \
		if (bp == 0) return x; \
		for (i = 0; i < path_len; ++i) ++path[i]->__head.size; \
		for (i = path_len - 1; i >= 0; --i) { \
			krmq_update_min_##suf(path[i], path[i]->__head.p[0], path[i]->__head.p[1]); \
			if (path[i]->__head.s != x) break; \
		} \
		for (p = bp, top = 0; p != x; p = p->__head.p[stack[top]], ++top) /* update balance factors */ \
			if (stack[top] == 0) --p->__head.balance; \
			else ++p->__head.balance; \
		if (bp->__head.balance > -2 && bp->__head.balance < 2) return x; /* no re-balance needed */ \
		/* re-balance */ \
		which = (bp->__head.balance < 0); \
		b1 = which == 0? +1 : -1; \
		q = bp->__head.p[1 - which]; \
		if (q->__head.balance == b1) { \
			r = krmq_rotate1_##suf(bp, which); \
			q->__head.balance = bp->__head.balance = 0; \
		} else r = krmq_rotate2_##suf(bp, which); \
		if (bq == 0) *root_ = r; \
		else bq->__head.p[bp != bq->__head.p[0]] = r; \
		return x; \
	}

#define __KRMQ_ERASE(suf, __scope, __type, __head, __cmp, __lt2) \
	__scope __type *krmq_erase_##suf(__type **root_, const __type *x, unsigned *cnt_) { \
		__type *p, *path[KRMQ_MAX_DEPTH], fake; \
		unsigned char dir[KRMQ_MAX_DEPTH]; \
		int i, d = 0, cmp; \
		unsigned cnt = 0; \
		fake.__head.p[0] = *root_, fake.__head.p[1] = 0; \
		if (cnt_) *cnt_ = 0; \
		if (x) { \
			for (cmp = -1, p = &fake; cmp; cmp = __cmp(x, p)) { \
				int which = (cmp > 0); \
				if (cmp > 0) cnt += krmq_size_child(__head, p, 0) + 1; \
				dir[d] = which; \
				path[d++] = p; \
				p = p->__head.p[which]; \
				if (p == 0) { \
					if (cnt_) *cnt_ = 0; \
					return 0; \
				} \
			} \
			cnt += krmq_size_child(__head, p, 0) + 1; /* because p==x is not counted */ \
		} else { \
			for (p = &fake, cnt = 1; p; p = p->__head.p[0]) \
				dir[d] = 0, path[d++] = p; \
			p = path[--d]; \
		} \
		if (cnt_) *cnt_ = cnt; \
		for (i = 1; i < d; ++i) --path[i]->__head.size; \
		if (p->__head.p[1] == 0) { /* ((1,.)2,3)4 => (1,3)4; p=2 */ \
			path[d-1]->__head.p[dir[d-1]] = p->__head.p[0]; \
		} else { \
			__type *q = p->__head.p[1]; \
			if (q->__head.p[0] == 0) { /* ((1,2)3,4)5 => ((1)2,4)5; p=3,q=2 */ \
				q->__head.p[0] = p->__head.p[0]; \
				q->__head.balance = p->__head.balance; \
				path[d-1]->__head.p[dir[d-1]] = q; \
				path[d] = q, dir[d++] = 1; \
				q->__head.size = p->__head.size - 1; \
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
				for (i = e + 1; i < d; ++i) --path[i]->__head.size; \
				r->__head.size = p->__head.size - 1; \
			} \
		} \
		for (i = d - 1; i >= 0; --i) /* not sure why adding condition "path[i]->__head.s==p" doesn't work */ \
			krmq_update_min_##suf(path[i], path[i]->__head.p[0], path[i]->__head.p[1]); \
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
					path[d-1]->__head.p[dir[d-1]] = krmq_rotate2_##suf(q, which); \
				} else { \
					path[d-1]->__head.p[dir[d-1]] = krmq_rotate1_##suf(q, which); \
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

#define krmq_free(__type, __head, __root, __free) do { \
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

#define __KRMQ_ITR(suf, __scope, __type, __head, __cmp) \
	struct krmq_itr_##suf { \
		const __type *stack[KRMQ_MAX_DEPTH], **top; \
	}; \
	__scope void krmq_itr_first_##suf(const __type *root, struct krmq_itr_##suf *itr) { \
		const __type *p; \
		for (itr->top = itr->stack - 1, p = root; p; p = p->__head.p[0]) \
			*++itr->top = p; \
	} \
	__scope int krmq_itr_find_##suf(const __type *root, const __type *x, struct krmq_itr_##suf *itr) { \
		const __type *p = root; \
		itr->top = itr->stack - 1; \
		while (p != 0) { \
			int cmp; \
			*++itr->top = p; \
			cmp = __cmp(x, p); \
			if (cmp < 0) p = p->__head.p[0]; \
			else if (cmp > 0) p = p->__head.p[1]; \
			else break; \
		} \
		return p? 1 : 0; \
	} \
	__scope int krmq_itr_next_bidir_##suf(struct krmq_itr_##suf *itr, int dir) { \
		const __type *p; \
		if (itr->top < itr->stack) return 0; \
		dir = !!dir; \
		p = (*itr->top)->__head.p[dir]; \
		if (p) { /* go down */ \
			for (; p; p = p->__head.p[!dir]) \
				*++itr->top = p; \
			return 1; \
		} else { /* go up */ \
			do { \
				p = *itr->top--; \
			} while (itr->top >= itr->stack && p == (*itr->top)->__head.p[dir]); \
			return itr->top < itr->stack? 0 : 1; \
		} \
	} \

/**
 * Insert a node to the tree
 *
 * @param suf     name suffix used in KRMQ_INIT()
 * @param proot   pointer to the root of the tree (in/out: root may change)
 * @param x       node to insert (in)
 * @param cnt     number of nodes smaller than or equal to _x_; can be NULL (out)
 *
 * @return _x_ if not present in the tree, or the node equal to x.
 */
#define krmq_insert(suf, proot, x, cnt) krmq_insert_##suf(proot, x, cnt)

/**
 * Find a node in the tree
 *
 * @param suf     name suffix used in KRMQ_INIT()
 * @param root    root of the tree
 * @param x       node value to find (in)
 * @param cnt     number of nodes smaller than or equal to _x_; can be NULL (out)
 *
 * @return node equal to _x_ if present, or NULL if absent
 */
#define krmq_find(suf, root, x, cnt) krmq_find_##suf(root, x, cnt)
#define krmq_interval(suf, root, x, lower, upper) krmq_interval_##suf(root, x, lower, upper)
#define krmq_rmq(suf, root, lo, up) krmq_rmq_##suf(root, lo, up)

/**
 * Delete a node from the tree
 *
 * @param suf     name suffix used in KRMQ_INIT()
 * @param proot   pointer to the root of the tree (in/out: root may change)
 * @param x       node value to delete; if NULL, delete the first node (in)
 *
 * @return node removed from the tree if present, or NULL if absent
 */
#define krmq_erase(suf, proot, x, cnt) krmq_erase_##suf(proot, x, cnt)
#define krmq_erase_first(suf, proot) krmq_erase_##suf(proot, 0, 0)

#define krmq_itr_t(suf) struct krmq_itr_##suf

/**
 * Place the iterator at the smallest object
 *
 * @param suf     name suffix used in KRMQ_INIT()
 * @param root    root of the tree
 * @param itr     iterator
 */
#define krmq_itr_first(suf, root, itr) krmq_itr_first_##suf(root, itr)

/**
 * Place the iterator at the object equal to or greater than the query
 *
 * @param suf     name suffix used in KRMQ_INIT()
 * @param root    root of the tree
 * @param x       query (in)
 * @param itr     iterator (out)
 *
 * @return 1 if find; 0 otherwise. krmq_at(itr) is NULL if and only if query is
 *         larger than all objects in the tree
 */
#define krmq_itr_find(suf, root, x, itr) krmq_itr_find_##suf(root, x, itr)

/**
 * Move to the next object in order
 *
 * @param itr     iterator (modified)
 *
 * @return 1 if there is a next object; 0 otherwise
 */
#define krmq_itr_next(suf, itr) krmq_itr_next_bidir_##suf(itr, 1)
#define krmq_itr_prev(suf, itr) krmq_itr_next_bidir_##suf(itr, 0)

/**
 * Return the pointer at the iterator
 *
 * @param itr     iterator
 *
 * @return pointer if present; NULL otherwise
 */
#define krmq_at(itr) ((itr)->top < (itr)->stack? 0 : *(itr)->top)

#define KRMQ_INIT2(suf, __scope, __type, __head, __cmp, __lt2) \
	__KRMQ_FIND(suf, __scope, __type, __head,  __cmp) \
	__KRMQ_RMQ(suf, __scope, __type, __head,  __cmp, __lt2) \
	__KRMQ_ROTATE(suf, __type, __head, __lt2) \
	__KRMQ_INSERT(suf, __scope, __type, __head, __cmp, __lt2) \
	__KRMQ_ERASE(suf, __scope, __type, __head, __cmp, __lt2) \
	__KRMQ_ITR(suf, __scope, __type, __head, __cmp)

#define KRMQ_INIT(suf, __type, __head, __cmp, __lt2) \
	KRMQ_INIT2(suf,, __type, __head, __cmp, __lt2)

#endif
