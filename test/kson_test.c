#include <assert.h>
#include <stdio.h>
#include "kson.h"

static void test_colon_cannot_follow_container_marker(void)
{
	assert(kson_parse("[:0") == 0);
	assert(kson_parse("{:0") == 0);
	assert(kson_parse("[[:0]]") == 0);
}

static void test_valid_documents_still_parse(void)
{
	kson_t *kson;

	kson = kson_parse("a:1");
	assert(kson != 0);
	kson_destroy(kson);

	kson = kson_parse("{'a':{'b':1},'items':[1,2]}");
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
	test_colon_cannot_follow_container_marker();
	test_valid_documents_still_parse();
	printf("kson_test: all tests passed\n");
	return 0;
}
