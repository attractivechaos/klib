#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

struct kbitset_t;
void check(struct kbitset_t *bset, const int present[], const char *title);

#include "kbitset.h"

int nfail = 0;

void fail(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	nfail++;
}

void check(kbitset_t *bset, const int present[], const char *title)
{
	kbitset_iter_t itr;
	int i, j, n, nn;

	for (i = 0; present[i] >= 0; i++) kbs_insert(bset, present[i]);
	nn = i;

	for (i = j = n = 0; i < 600; i++)
		if (kbs_exists(bset, i)) {
			n++;
			if (i == present[j]) j++;
			else fail("%s: %d should not be in the set\n", title, i);
		}
		else {
			if (i == present[j]) {
				fail("%s: %d should be in the set\n", title, i);
				j++;
			}
		}

	if (n != nn)
		fail("%s: expected %d elements; found %d\n", title, nn, n);

	j = n = 0;
	kbs_start(&itr);
	while ((i = kbs_next(bset, &itr)) >= 0) {
		n++;
		if (i == present[j]) j++;
		else fail("%s: %d should not be returned by iterator\n", title, i);
	}

	if (n != nn)
		fail("%s: expected %d elements; iterator found %d\n", title, nn, n);
}

// Element boundaries
#define B KBS_ELTBITS

const int test1[] = {
	0, 1, 6, 10, 20, 22, 24,
	B-1, B, 2*B-1, 3*B,
	4*B-2, 4*B-1, 4*B, 4*B+1,
	512, 513,
	-1
};

const int test2[] = {
	3*B+5, 4*B-10, 500, 501, 502, 503, 504, 505, 506, 599,
	-1
};

int main(int argc, char **argv)
{
	kbitset_t *bset = kbs_init(600);

	check(bset, test1, "test1");
	kbs_delete(bset, 0);
	kbs_delete(bset, 1);
	kbs_delete(bset, 6);
	check(bset, &test1[3], "test1a");

	kbs_clear(bset);
	check(bset, test2, "test2");

	kbs_destroy(bset);

	if (nfail > 0) {
		fprintf(stderr, "Total failures: %d\n", nfail);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
