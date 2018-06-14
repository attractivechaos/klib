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


/** JSON data tree node structure/object */
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

/**
 * Generate JSON tree data structure from a serialized JSON string
 * 
 * @param json serialized JSON string
 * 
 * @return reference/pointer to root node of generated JSON tree data structure
 */
kson_node_t *kson_parse(const char *json);

/**
 * Create root/base JSON node of given type
 * 
 * @param type enum value defining node type (KSON_TYPE_{NULL,BOOLEAN,NUMBER,STRING,BRACKET,BRACE})
 * @param key string key to index the node. NULL for node without key
 * 
 * @return reference/pointer to created node. NULL on errors
 */
kson_node_t *kson_create(long type, const char *key);

/**
 * Set value of given JSON node
 * 
 * @param root node for which the value will be set
 * @param value node value in string form
 */
void kson_set(kson_node_t *root, const char *value);
    
/**
 * Clear value of given node or destroy its children, if internal
 * 
 * @param root node from which the clearing starts
 */
void kson_clear(kson_node_t *root);
    
/**
 * Destroy given node and its children, if any
 * 
 * @param root node to be destroyed
 */
void kson_destroy(kson_node_t *root);
    
/**
 * Find node (inside a BRACE type node) by its key
 * 
 * @param root pointer to the node (BRACE type) where search will be performed
 * @param key key of searched node
 * 
 * @return reference/pointer to found node. NULL if nothing is found
 */
kson_node_t *kson_by_key(const kson_node_t *root, const char *key);
    
/**
 * Find node (inside a BRACKET type node) by its index
 * 
 * @param root pointer to the node (BRACKET type) where search will be performed
 * @param i index of searched node
 * 
 * @return reference/pointer to found node. NULL if nothing is found
 */
kson_node_t *kson_by_index(const kson_node_t *root, long i);
    
/**
 * Find node following a sequence of keys and/or indexes
 * 
 * @param root pointer to the node from where search will be performed
 * @param path_len number of parameters for the subsequent variable lenght arguments list
 * 
 * @return reference/pointer to found node. NULL if nothing is found
 */
kson_node_t *kson_by_path(const kson_node_t *root, int path_len, ...);
    
/**
 * Display JSON data tree as a formatted string
 * 
 * @param root root/base node of the tree to be displayed
 */
void kson_format(const kson_node_t *root);
    
/**
 * Write JSON data tree to a string
 * 
 * @param root root/base node of the tree to be written
 * @param mode format of the string representation. Serialized (KSON_FMT_SERIAL) or idented (KSON_FMT_IDENT)
 * 
 * @return reference/pointer to allocated string containing JSON data. Must be freed manually
 */
char* kson_format_str(const kson_node_t *root, int mode);
    
/**
 * Add JSON node to BRACE type node
 * 
 * @param root parent BRACE type node to which new node will be added
 * @param type type of new child node to be added
 * @param key key of new child node to be added
 * 
 * @return reference/pointer to newly created child node. NULL on errors
 */
kson_node_t *kson_add_key(kson_node_t *root, long type, const char *key);
    
/**
 * Append JSON node to BRACKET type node
 * 
 * @param root parent BRACKET type node to which new node will be appended
 * @param type type of new child node to be appended
 * 
 * @return reference/pointer to newly created child node. NULL on errors
 */
kson_node_t *kson_add_index(kson_node_t *root, long type);

#ifdef __cplusplus
}
#endif

#endif
