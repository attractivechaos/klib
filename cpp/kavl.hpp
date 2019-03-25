#ifndef KAVL_HPP
#define KAVL_HPP

#include <cstdlib> // for malloc() etc
#include <functional>

namespace klib {

template<class T, class Less = std::less<T> >
class Kavl {
	static const int MAX_DEPTH = 64;
	struct Node {
		T data;
		signed char balance;
		Kavl<T, Less>::Node *p[2];
	};
	typedef Kavl<T, Less>::Node *NodePtr;

	// member variable
	NodePtr root;

	// one rotation: (a,(b,c)q)p => ((a,b)p,c)q
	static inline NodePtr rotate1(NodePtr p, int dir) { // dir=0 to left; dir=1 to right
		int opp = 1 - dir; // opposite direction
		NodePtr q = p->p[opp];
		p->p[opp] = q->p[dir];
		q->p[dir] = p;
		return q;
	};
	// two consecutive rotations: (a,((b,c)r,d)q)p => ((a,b)p,(c,d)q)r
	static inline NodePtr rotate2(NodePtr p, int dir) {
		int b1, opp = 1 - dir;
		NodePtr q = p->p[opp], r = q->p[dir];
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
	}
	// deallocate
	void destroy(void) {
		NodePtr p, q;
		for (p = root; p; p = q) {
			if (p->p[0] == 0) {
				q = p->p[1];
				std::free(p);
			} else {
				q = p->p[0];
				p->p[0] = q->p[1];
				q->p[1] = p;
			}
		}
		root = 0;
	};
public:
	Kavl() : root(0) {};
	~Kavl() { destroy(); };
	bool find(const T &data) const {
		NodePtr p = root;
		while (p) {
			if (Less()(data, p->data)) p = p->p[0];
			else if (Less()(p->data, data)) p = p->p[1];
			else break;
		}
		return (p != 0);
	};
	bool insert(const T &data) {
		unsigned char stack[MAX_DEPTH];
		NodePtr path[MAX_DEPTH];
		NodePtr bp, bq, x;
		NodePtr p, q, r = 0; // _r_ is potentially the new root
		int which = 0, top, b1, path_len;
		bp = root, bq = 0;
		// find the insertion location
		for (p = bp, q = bq, top = path_len = 0; p; q = p, p = p->p[which]) {
			int cmp = (int)Less()(data, p->data) - (int)Less()(p->data, data);
			if (cmp == 0) return false;
			if (p->balance != 0)
				bq = q, bp = p, top = 0;
			stack[top++] = which = (cmp > 0);
			path[path_len++] = p;
		}
		x = (NodePtr)std::calloc(1, sizeof(*x));
		x->data = data;
		if (q == 0) root = x;
		else q->p[which] = x;
		if (bp == 0) return true;
		for (p = bp, top = 0; p != x; p = p->p[stack[top]], ++top) // update balance factors
			if (stack[top] == 0) --p->balance;
			else ++p->balance;
		if (bp->balance > -2 && bp->balance < 2) return true; // no re-balance needed
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
		return true;
	}
	bool erase(const T &data, bool erase_first = false) {
		NodePtr p, path[MAX_DEPTH];
		Node fake;
		unsigned char dir[MAX_DEPTH];
		int d = 0;
		fake.p[0] = root, fake.p[1] = 0;
		if (!erase_first) {
			int cmp = -1;
			for (p = &fake; cmp;) {
				int which = (cmp > 0);
				dir[d] = which;
				path[d++] = p;
				p = p->p[which];
				if (p == 0) return false;
				cmp = (int)Less()(p->data, data) - (int)Less()(data, p->data);
			}
		} else {
			for (p = &fake; p; p = p->p[0])
				dir[d] = 0, path[d++] = p;
			p = path[--d];
		}
		if (p->p[1] == 0) { // ((1,.)2,3)4 => (1,3)4; p=2
			path[d-1]->p[dir[d-1]] = p->p[0];
		} else {
			NodePtr q = p->p[1];
			if (q->p[0] == 0) { // ((1,2)3,4)5 => ((1)2,4)5; p=3
				q->p[0] = p->p[0];
				q->balance = p->balance;
				path[d-1]->p[dir[d-1]] = q;
				path[d] = q, dir[d++] = 1;
			} else { // ((1,((.,2)3,4)5)6,7)8 => ((1,(2,4)5)3,7)8; p=6
				NodePtr r;
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
			}
		}
		while (--d > 0) {
			NodePtr q = path[d];
			int which, other, b1 = 1, b2 = 2;
			which = dir[d], other = 1 - which;
			if (which) b1 = -b1, b2 = -b2;
			q->balance += b1;
			if (q->balance == b1) break;
			else if (q->balance == b2) {
				NodePtr r = q->p[other];
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
		return true;
	};
};

} // end of namespace klib

#endif
