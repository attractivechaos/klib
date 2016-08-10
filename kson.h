#ifndef KSON_H
#define KSON_H

#include <string.h>

#define KSON_TYPE_NO_QUOTE  1
#define KSON_TYPE_SGL_QUOTE 2
#define KSON_TYPE_DBL_QUOTE 3
#define KSON_TYPE_BRACKET   4
#define KSON_TYPE_BRACE     5

#define KSON_OK              0
#define KSON_ERR_EXTRA_LEFT  1
#define KSON_ERR_EXTRA_RIGHT 2
#define KSON_ERR_NO_KEY      3

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

typedef struct {
	long n_nodes;
	kson_node_t *root;
} kson_t;

#define kson_is_internal(p) ((p)->type == KSON_TYPE_BRACKET || (p)->type == KSON_TYPE_BRACE)

#ifdef __cplusplus
extern "C" {
#endif

	kson_t *kson_parse(const char *json);
	void kson_destroy(kson_t *kson);
    const kson_node_t *kson_by_key(const kson_node_t *p, const char *key);
    const kson_node_t *kson_by_index(const kson_node_t *p, long i)
	const kson_node_t *kson_by_path(const kson_node_t *root, int path_len, ...);
	void kson_format(const kson_node_t *root);
    char* kson_format_str(const kson_node_t *root, int mode);
    kson_node_t *kson_add_key(const kson_node_t *p, long type, const char *key);
    kson_node_t *kson_add_index(const kson_node_t *p, long type);

#ifdef __cplusplus
}
#endif

#endif
