#ifndef KAVL_HPP
#define KAVL_HPP

#include <functional>

namespace klib {

template<class T, typename Less = std::less<T> >
class Avl {
	static const int MAX_DEPTH = 64;
	struct Node {
		T data;
		signed char balance;
		unsigned size;
		Node *p[2];
	};
	Node *root;
	inline int cmp_func(const T &x, const T &y) {
		return Less()(y, x) - Less()(x, y);
	}
	inline unsigned child_size(Node *p, int dir) {
		return p->p[dir]? p->p[dir]->size : 0;
	};
	// one rotation: (a,(b,c)q)p => ((a,b)p,c)q
	inline Node *rotate1(Node *p, int dir) { // dir=0 to left; dir=1 to right
		int opp = 1 - dir; // opposite direction
		Node *q = p->p[opp];
		unsigned size_p = p->size;
		p->size -= q->size - child_size(q, dir);
		q->size = size_p;
		p->p[opp] = q->p[dir];
		q->p[dir] = p;
		return q;
	};
	// two consecutive rotations: (a,((b,c)r,d)q)p => ((a,b)p,(c,d)q)r
	inline Node *rotate2(Node *p, int dir) {
		int b1, opp = 1 - dir;
		Node *q = p->p[opp], *r = q->p[dir];
		unsigned size_x_dir = child_size(r, dir);
		r->size = p->size;
		p->size -= q->size - size_x_dir;
		q->size -= size_x_dir + 1;
		p->p[opp] = r->p[dir];
		r->p[dir] = p;
		q->p[dir] = r->p[opp];
		r->p[opp] = q;
		b1 = dir == 0? +1 : -1;
		if (r->balance == b1) q->balance = 0, p->balance = -b1;
		else if (r->balance == 0) q->balance = p->balance = 0;
		else q->balance = b1, p->balance = 0;
		r->balance = 0;
		return r;
	};
	void destroy(Node *r) {
		Node *p, *q;
		for (p = r; p; p = q) {
			if (p->p[0] == 0) {
				q = p->p[1];
				delete p;
			} else {
				q = p->p[0];
				p->p[0] = q->p[1];
				q->p[1] = p;
			}
		}
	};
public:
	Avl() : root(NULL) {};
	~Avl() { destroy(root); };
	unsigned size() const { return root? root->size : 0; }
	T *find(const T &data, unsigned *cnt_ = NULL) {
		Node *p = root;
		unsigned cnt = 0;
		while (p != 0) {
			int cmp = cmp_func(data, p->data);
			if (cmp >= 0) cnt += child_size(p, 0) + 1;
			if (cmp < 0) p = p->p[0];
			else if (cmp > 0) p = p->p[1];
			else break;
		}
		if (cnt_) *cnt_ = cnt;
		return p? &p->data : NULL;
	};
	T *insert(const T &data, bool *is_new = NULL, unsigned *cnt_ = NULL) {
		unsigned char stack[MAX_DEPTH];
		Node *path[MAX_DEPTH];
		Node *bp, *bq;
		Node *x, *p, *q, *r = 0; // _r_ is potentially the new root
		int i, which = 0, top, b1, path_len;
		unsigned cnt = 0;
		bp = root, bq = 0;
		if (is_new) *is_new = false;
		// find the insertion location
		for (p = bp, q = bq, top = path_len = 0; p; q = p, p = p->p[which]) {
			int cmp = cmp_func(data, p->data);
			if (cmp >= 0) cnt += child_size(p, 0) + 1;
			if (cmp == 0) {
				if (cnt_) *cnt_ = cnt;
				return &p->data;
			}
			if (p->balance != 0)
				bq = q, bp = p, top = 0;
			stack[top++] = which = (cmp > 0);
			path[path_len++] = p;
		}
		if (cnt_) *cnt_ = cnt;
		x = new Node;
		x->data = data, x->balance = 0, x->size = 1, x->p[0] = x->p[1] = 0;
		if (is_new) *is_new = true;
		if (q == 0) root = x;
		else q->p[which] = x;
		if (bp == 0) return &x->data;
		for (i = 0; i < path_len; ++i) ++path[i]->size;
		for (p = bp, top = 0; p != x; p = p->p[stack[top]], ++top) /* update balance factors */
			if (stack[top] == 0) --p->balance;
			else ++p->balance;
		if (bp->balance > -2 && bp->balance < 2) return &x->data; /* no re-balance needed */
		// re-balance
		which = (bp->balance < 0);
		b1 = which == 0? +1 : -1;
		q = bp->p[1 - which];
		if (q->balance == b1) {
			r = rotate1(bp, which);
			q->balance = bp->balance = 0;
		} else r = rotate2(bp, which);
		if (bq == 0) root = r;
		else bq->p[bp != bq->p[0]] = r;
		return &x->data;
	};
	bool erase(const T &data) {
		Node *p, *path[MAX_DEPTH], fake;
		unsigned char dir[MAX_DEPTH];
		int i, d = 0, cmp;
		fake.p[0] = root, fake.p[1] = 0;
		for (cmp = -1, p = &fake; cmp; cmp = cmp_func(data, p->data)) {
			int which = (cmp > 0);
			dir[d] = which;
			path[d++] = p;
			p = p->p[which];
			if (p == 0) return false;
		}
		for (i = 1; i < d; ++i) --path[i]->size;
		if (p->p[1] == 0) { // ((1,.)2,3)4 => (1,3)4; p=2
			path[d-1]->p[dir[d-1]] = p->p[0];
		} else {
			Node *q = p->p[1];
			if (q->p[0] == 0) { // ((1,2)3,4)5 => ((1)2,4)5; p=3
				q->p[0] = p->p[0];
				q->balance = p->balance;
				path[d-1]->p[dir[d-1]] = q;
				path[d] = q, dir[d++] = 1;
				q->size = p->size - 1;
			} else { // ((1,((.,2)3,4)5)6,7)8 => ((1,(2,4)5)3,7)8; p=6
				Node *r;
				int e = d++; // backup _d_
				for (;;) {
					dir[d] = 0;
					path[d++] = q;
					r = q->p[0];
					if (r->p[0] == 0) break;
					q = r;
				}
				r->p[0] = p->p[0];
				q->p[0] = r->p[1];
				r->p[1] = p->p[1];
				r->balance = p->balance;
				path[e-1]->p[dir[e-1]] = r;
				path[e] = r, dir[e] = 1;
				for (i = e + 1; i < d; ++i) --path[i]->size;
				r->size = p->size - 1;
			}
		}
		while (--d > 0) {
			Node *q = path[d];
			int which, other, b1 = 1, b2 = 2;
			which = dir[d], other = 1 - which;
			if (which) b1 = -b1, b2 = -b2;
			q->balance += b1;
			if (q->balance == b1) break;
			else if (q->balance == b2) {
				Node *r = q->p[other];
				if (r->balance == -b1) {
					path[d-1]->p[dir[d-1]] = rotate2(q, which);
				} else {
					path[d-1]->p[dir[d-1]] = rotate1(q, which);
					if (r->balance == 0) {
						r->balance = -b1;
						q->balance = b1;
						break;
					} else r->balance = q->balance = 0;
				}
			}
		}
		root = fake.p[0];
		delete p;
		return true;
	};
};

} // end of namespace klib

#endif
