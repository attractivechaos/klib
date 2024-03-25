#include <stdio.h>
#include "klist.h"

#define __int_free(x)
KLIST_INIT(32, int, __int_free)

int main()
{
	klist_t(32) *kl;
	kliter_t(32) *p;
	kl = kl_init(32);
	*kl_pushp(32, kl) = 10;
	*kl_pushp(32, kl) = 20;
	*kl_pushp(32, kl) = 30;
	*kl_pushp(32, kl) = 40;
	*kl_pushp(32, kl) = 50;
	//kl_shift(32, kl, 0);
	*kl_pushp(32, kl) = 60;

	p = kl_begin(kl);
	p = kl_next(p);
	p = kl_remove(32, kl, p);
	p = kl_next(p);
	p = kl_remove(32, kl, p);

	*kl_pushp(32, kl) = 70;

	for (p = kl_begin(kl); p != kl_end(kl); p = kl_next(p))
		printf("%d\n", kl_val(p));
	printf("size: %ld\n", kl->size);
	kl_destroy(32, kl);
	return 0;
}
