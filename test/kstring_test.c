#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kstring.h"

int nfail = 0;

void check(const char *what, const kstring_t *ks, const char *correct)
{
	if (ks->l != strlen(correct) || strcmp(ks->s, correct) != 0) {
		fprintf(stderr, "%s produced \"%.*s\" (\"%s\" is correct)\tFAIL\n", what, (int)(ks->l), ks->s, correct);
		nfail++;
	}
}

void test_kputw(kstring_t *ks, int n)
{
	char buf[16];

	ks->l = 0;
	kputw(n, ks);

	sprintf(buf, "%d", n);
	check("kputw()", ks, buf);
}

void test_kputl(kstring_t *ks, long n)
{
	char buf[24];

	ks->l = 0;
	kputl(n, ks);

	sprintf(buf, "%ld", n);
	check("kputl()", ks, buf);
}

static char *mem_gets(char *buf, int buflen, void *vtextp)
{
	const char **textp = (const char **) vtextp;

	const char *nl = strchr(*textp, '\n');
	size_t n = nl? nl - *textp + 1 : strlen(*textp);

	if (n == 0) return NULL;

	if (n > buflen-1) n = buflen-1;
	memcpy(buf, *textp, n);
	buf[n] = '\0';
	*textp += n;
	return buf;
}

void test_kgetline(kstring_t *ks, const char *text, ...)
{
	const char *exp;
	va_list arg;

	va_start(arg, text);
	while ((exp = va_arg(arg, const char *)) != NULL) {
		ks->l = 0;
		if (kgetline(ks, mem_gets, &text) != 0) kputs("EOF", ks);
		check("kgetline()", ks, exp);
	}
	va_end(arg);

	ks->l = 0;
	if (kgetline(ks, mem_gets, &text) == 0) check("kgetline()", ks, "EOF");
}

int main(int argc, char **argv)
{
	kstring_t ks;

	ks.l = ks.m = 0;
	ks.s = NULL;

	test_kputw(&ks, 0);
	test_kputw(&ks, 1);
	test_kputw(&ks, 37);
	test_kputw(&ks, 12345);
	test_kputw(&ks, -12345);
	test_kputw(&ks, INT_MAX);
	test_kputw(&ks, -INT_MAX);
	test_kputw(&ks, INT_MIN);

	test_kputl(&ks, 0);
	test_kputl(&ks, 1);
	test_kputl(&ks, 37);
	test_kputl(&ks, 12345);
	test_kputl(&ks, -12345);
	test_kputl(&ks, INT_MAX);
	test_kputl(&ks, -INT_MAX);
	test_kputl(&ks, INT_MIN);
	test_kputl(&ks, LONG_MAX);
	test_kputl(&ks, -LONG_MAX);
	test_kputl(&ks, LONG_MIN);

	test_kgetline(&ks, "", NULL);
	test_kgetline(&ks, "apple", "apple", NULL);
	test_kgetline(&ks, "banana\n", "banana", NULL);
	test_kgetline(&ks, "carrot\r\n", "carrot", NULL);
	test_kgetline(&ks, "\n", "", NULL);
	test_kgetline(&ks, "\n\n", "", "", NULL);
	test_kgetline(&ks, "foo\nbar", "foo", "bar", NULL);
	test_kgetline(&ks, "foo\nbar\n", "foo", "bar", NULL);
	test_kgetline(&ks,
		"abcdefghijklmnopqrstuvwxyz0123456789\nABCDEFGHIJKLMNOPQRSTUVWXYZ\n",
		"abcdefghijklmnopqrstuvwxyz0123456789",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ", NULL);

	if (argc > 1) {
		FILE *f = fopen(argv[1], "r");
		if (f) {
			for (ks.l = 0; kgetline(&ks, (kgets_func *)fgets, f) == 0; ks.l = 0)
				puts(ks.s);
			fclose(f);
		}
	}

	free(ks.s);

	if (nfail > 0) {
		fprintf(stderr, "Total failures: %d\n", nfail);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
