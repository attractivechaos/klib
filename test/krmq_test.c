#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "krmq.h"

#define CALLOC(type, num) ((type*)calloc(num, sizeof(type)))

struct my_node {
	int key;
	int val;
	KRMQ_HEAD(struct my_node) head;
};

#define my_cmp(p, q) (((p)->key > (q)->key) - ((p)->key < (q)->key))
#define my_lt2(p, q) ((p)->val < (q)->val)
KRMQ_INIT(my, struct my_node, head, my_cmp, my_lt2)

int check(struct my_node *p, int *hh, int *min)
{
	*hh = 0, *min = INT_MAX;
	if (p) {
		int c = 1, h[2] = {0, 0}, m[2] = {INT_MAX, INT_MAX};
		*min = p->val;
		if (p->head.p[0]) c += check(p->head.p[0], &h[0], &m[0]);
		if (p->head.p[1]) c += check(p->head.p[1], &h[1], &m[1]);
		*min = *min < m[0]? *min : m[0];
		*min = *min < m[1]? *min : m[1];
		*hh = (h[0] > h[1]? h[0] : h[1]) + 1;
		if (*min != p->head.s->val)
			fprintf(stderr, "min %d != %d at %c\n", *min, p->head.s->val, p->key);
		if (h[1] - h[0] != (int)p->head.balance)
			fprintf(stderr, "%d - %d != %d at %c\n", h[1], h[0], p->head.balance, p->key);
		if (c != (int)p->head.size)
			fprintf(stderr, "%d != %d at %c\n", p->head.size, c, p->key);
		return c;
	} else return 0;
}

int check_rmq(const struct my_node *root, int lo, int hi)
{
	struct my_node s, t, *p, *q;
	krmq_itr_t(my) itr;
	int min = INT_MAX;
	s.key = lo, t.key = hi;
	p = krmq_rmq(my, root, &s, &t);
	krmq_interval(my, root, &s, 0, &q);
	if (p == 0) return -1;
	krmq_itr_find(my, root, q, &itr);
	do {
		const struct my_node *r = krmq_at(&itr);
		if (r->key > hi) break;
		//fprintf(stderr, "%c\t%d\n", r->key, r->val);
		if (r->val < min) min = r->val;
	} while (krmq_itr_next(my, &itr));
	assert((min == INT_MAX && p == 0) || (min < INT_MAX && p));
	if (min != p->val) fprintf(stderr, "rmq_min %d != %d\n", p->val, min);
	return min;
}

int print_tree(const struct my_node *p)
{
	int c = 1;
	if (p == 0) return 0;
	if (p->head.p[0] || p->head.p[1]) {
		putchar('(');
		if (p->head.p[0]) c += print_tree(p->head.p[0]);
		else putchar('.');
		putchar(',');
		if (p->head.p[1]) c += print_tree(p->head.p[1]);
		else putchar('.');
		putchar(')');
	}
	printf("%c:%d/%d", p->key, p->val, p->head.s->val);
	return c;
}

void check_and_print(struct my_node *root)
{
	int h, min;
	check(root, &h, &min);
	print_tree(root);
	putchar('\n');
}

void shuffle(int n, char a[])
{
	int i, j;
	for (i = n; i > 1; --i) {
		char tmp;
		j = (int)(drand48() * i);
		tmp = a[j]; a[j] = a[i-1]; a[i-1] = tmp;
	}
}

int main(void)
{
	char buf[256];
	int i, n, h, min;
	struct my_node *root = 0;
	struct my_node *p, *q, t;
	krmq_itr_t(my) itr;
	unsigned cnt;

	srand48(123);
	for (i = 33, n = 0; i <= 126; ++i)
		if (i != '(' && i != ')' && i != '.' && i != ';')
			buf[n++] = i;
	shuffle(n, buf);
	for (i = 0; i < n; ++i) {
		p = CALLOC(struct my_node, 1);
		p->key = buf[i];
		p->val = i;
		q = krmq_insert(my, &root, p, &cnt);
		if (p != q) free(p);
		check(root, &h, &min);
	}

	shuffle(n, buf);
	for (i = 0; i < n/2; ++i) {
		t.key = buf[i];
		//fprintf(stderr, "i=%d, key=%c, n/2=%d\n", i, t.key, n/2);
		q = krmq_erase(my, &root, &t, 0);
		if (q) free(q);
		check(root, &h, &min);
	}
	check_and_print(root);

	check_rmq(root, '0', '9');
	check_rmq(root, '!', '~');
	check_rmq(root, 'A', 'Z');
	check_rmq(root, 'F', 'G');
	check_rmq(root, 'a', 'z');
	for (i = 0; i < n; ++i) {
		int lo, hi;
		lo = (int)(drand48() * n);
		hi = (int)(drand48() * n);
		check_rmq(root, lo, hi);
	}

	krmq_itr_first(my, root, &itr);
	do {
		const struct my_node *r = krmq_at(&itr);
		putchar(r->key);
	} while (krmq_itr_next(my, &itr));
	putchar('\n');
	krmq_free(struct my_node, head, root, free);
	return 0;
}
