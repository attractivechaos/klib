#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "kavl-lite.h"

#define CALLOC(type, num) ((type*)calloc(num, sizeof(type)))

struct my_node {
	int key;
	KAVLL_HEAD(struct my_node) head;
};

#define my_cmp(p, q) (((p)->key > (q)->key) - ((p)->key < (q)->key))
KAVLL_INIT(my, struct my_node, head, my_cmp)

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
	int i, n;
	struct my_node *root = 0;
	struct my_node *p, *q, t;
	my_itr_t itr;

	for (i = 33, n = 0; i <= 126; ++i)
		if (i != '(' && i != ')' && i != '.' && i != ';')
			buf[n++] = i;
	shuffle(n, buf);
	for (i = 0; i < n; ++i) {
		p = CALLOC(struct my_node, 1);
		p->key = buf[i];
		q = my_insert(&root, p);
		if (p != q) free(p);
	}
	shuffle(n, buf);
	for (i = 0; i < n/2; ++i) {
		t.key = buf[i];
		q = my_erase(&root, &t);
		if (q) free(q);
	}

	my_itr_first(root, &itr);
	do {
		const struct my_node *r = kavll_at(&itr);
		putchar(r->key);
		free((void*)r);
	} while (my_itr_next(&itr));
	putchar('\n');
	return 0;
}
