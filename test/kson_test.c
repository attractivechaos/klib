#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "kson.h"

/*
 * "[:0" is not valid JSON: a colon can only follow a key, and an array
 * cannot start with one. Before this bug was fixed, kson_parse_core()
 * failed to reject this input and went on to treat the array-opening
 * marker on the stack as if it were the index of a key node, reading
 * and writing through a[-1] one slot before the start of the node
 * array. Under AddressSanitizer this aborts with a heap-buffer-overflow;
 * the correct behavior is simply to reject the input.
 */
static void test_colon_after_bracket_is_rejected(void)
{
	kson_t *kson = kson_parse("[:0");
	assert(kson == 0);
}

static void test_colon_after_brace_is_rejected(void)
{
	kson_t *kson = kson_parse("{:0");
	assert(kson == 0);
}

static void test_colon_after_nested_bracket_is_rejected(void)
{
	kson_t *kson = kson_parse("[[:0]]");
	assert(kson == 0);
}

/* make sure the fix does not reject any of the JSON kson already supports */
static void test_valid_documents_still_parse(void)
{
	kson_t *kson;

	kson = kson_parse("{'a' : 1,'b':[0,'isn\\'t',true],'d':[{\n\n\n}]}");
	assert(kson != 0);
	{
		const kson_node_t *p = kson_by_path(kson->root, 2, "b", 1);
		/* kson does not interpret backslash escapes, it just skips over
		 * them, so the stored string still contains the literal '\' */
		assert(p != 0 && p->v.str != 0 && strcmp(p->v.str, "isn\\'t") == 0);
	}
	kson_destroy(kson);

	kson = kson_parse("a:1"); /* bare, unbracketed key:value pair */
	assert(kson != 0);
	kson_destroy(kson);

	kson = kson_parse("{'a':{'b':1}}"); /* nested object with a real key */
	assert(kson != 0);
	kson_destroy(kson);

	kson = kson_parse("[1,2,3]");
	assert(kson != 0);
	kson_destroy(kson);

	kson = kson_parse("{}");
	assert(kson != 0);
	kson_destroy(kson);

	kson = kson_parse("[]");
	assert(kson != 0);
	kson_destroy(kson);
}

int main(void)
{
	test_colon_after_bracket_is_rejected();
	test_colon_after_brace_is_rejected();
	test_colon_after_nested_bracket_is_rejected();
	test_valid_documents_still_parse();
	printf("kson_test: all tests passed\n");
	return 0;
}
