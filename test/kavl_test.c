#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "kavl.h"

#define CALLOC(type, num) ((type*)calloc(num, sizeof(type)))

struct my_node {
	int key;
	KAVL_HEAD(struct my_node) head;
};

#define my_cmp(p, q) (((p)->key > (q)->key) - ((p)->key < (q)->key))
KAVL_INIT(my, struct my_node, head, my_cmp)

int check(struct my_node *p, int *hh)
{
	int c = 1, h[2] = {0, 0};
	*hh = 0;
	if (p) {
		if (p->head.p[0]) c += check(p->head.p[0], &h[0]);
		if (p->head.p[1]) c += check(p->head.p[1], &h[1]);
		*hh = (h[0] > h[1]? h[0] : h[1]) + 1;
		if (h[1] - h[0] != (int)p->head.balance)
			fprintf(stderr, "%d - %d != %d at %c\n", h[1], h[0], p->head.balance, p->key);
		if (c != (int)p->head.size)
			fprintf(stderr, "%d != %d at %c\n", p->head.size, c, p->key);
		return c;
	} else return 0;
}
/*
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
	putchar(p->key);
	return c;
}

void check_and_print(struct my_node *root)
{
	int h;
	check(root, &h);
	print_tree(root);
	putchar('\n');
}
*/
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
	int i, n, h;
	struct my_node *root = 0;
	struct my_node *p, *q, t;
	kavl_itr_t(my) itr;
	unsigned cnt;

	for (i = 33, n = 0; i <= 126; ++i)
		if (i != '(' && i != ')' && i != '.' && i != ';')
			buf[n++] = i;
	shuffle(n, buf);
	for (i = 0; i < n; ++i) {
		p = CALLOC(struct my_node, 1);
		p->key = buf[i];
		q = kavl_insert(my, &root, p, &cnt);
		if (p != q) free(p);
		check(root, &h);
	}
	shuffle(n, buf);
	for (i = 0; i < n/2; ++i) {
		t.key = buf[i];
		q = kavl_erase(my, &root, &t, 0);
		if (q) free(q);
		check(root, &h);
	}

	kavl_itr_first(my, root, &itr);
	do {
		const struct my_node *r = kavl_at(&itr);
		putchar(r->key);
		free((void*)r);
	} while (kavl_itr_next(my, &itr));
	putchar('\n');
	return 0;
}
