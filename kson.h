#ifndef KSON_H
#define KSON_H

#include <string.h>

#define KSON_TYPE_NULL      0
#define KSON_TYPE_BOOLEAN   1
#define KSON_TYPE_NUMBER    2
#define KSON_TYPE_STRING    3
#define KSON_TYPE_BRACKET   4
#define KSON_TYPE_BRACE     5

#define KSON_OK              0
#define KSON_ERR_UNEXPECTED  1
#define KSON_ERR_NO_KEY      2
#define KSON_ERR_NO_VALUE    3

#define KSON_FMT_SERIAL      -1
#define KSON_FMT_IDENT       0


typedef struct kson_node_s {
	unsigned long long type:3, n:61;
	char *key;
	union {
		struct kson_node_s **child;
		char *str;
	} v;
} kson_node_t;

#define kson_is_internal(p) ((p)->type == KSON_TYPE_BRACKET || (p)->type == KSON_TYPE_BRACE)

#ifdef __cplusplus
extern "C" {
#endif

	kson_node_t *kson_parse(const char *json);
	kson_node_t *kson_create(long type, const char *key);
	void kson_set(kson_node_t *root, const char *value);
	void kson_clear(kson_node_t *root);
	void kson_destroy(kson_node_t *root);
	kson_node_t *kson_by_key(const kson_node_t *root, const char *key);
	kson_node_t *kson_by_index(const kson_node_t *root, long i);
	kson_node_t *kson_by_path(const kson_node_t *root, int path_len, ...);
	void kson_format(const kson_node_t *root);
	char* kson_format_str(const kson_node_t *root, int mode);
	kson_node_t *kson_add_key(kson_node_t *root, long type, const char *key);
	kson_node_t *kson_add_index(kson_node_t *root, long type);

#ifdef __cplusplus
}
#endif

#endif
