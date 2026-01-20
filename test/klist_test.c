#include <stdio.h>
#include "klist.h"

#define __int_free(x)
KLIST_INIT(32, int, __int_free)

int main()
{
	klist_t(32) *kl;
	kl = kl_init(32);
	printf("Initial size of list is %d\n", kl_size(32, kl));
	*kl_pushp(32, kl) = 1;
	*kl_pushp(32, kl) = 2;
	*kl_pushp(32, kl) = 3;
	*kl_pushp(32, kl) = 4;
	*kl_pushp(32, kl) = 5;
	*kl_pushp(32, kl) = 6;
	*kl_pushp(32, kl) = 7;
	*kl_pushp(32, kl) = 10;
	printf("Size of list after several pushp is %d\n", kl_size(32, kl));
	kl_shift(32, kl, 0);
	printf("Size of list after shift is %d\n", kl_size(32, kl));
	kliter_t(32) *p;
	for (p = kl_begin(kl); p != kl_end(kl); p = kl_next(p))
		printf("%d\n", kl_val(p));
	kl_destroy(32, kl);
	return 0;
}
