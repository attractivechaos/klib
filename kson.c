#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include "kson.h"

const char *NULL_STR = "null";
const char *TRUE_STR = "true";
const char *FALSE_STR = "false";

/*************
 *** Parse ***
 *************/

kson_node_t *kson_parse_recur(const char **json, int *error)
{
	const char *p;
	kson_node_t *u = kson_create(KSON_TYPE_NULL, 0);
	assert(sizeof(size_t) == sizeof(kson_node_t*));
	*error = KSON_OK;
	for (p = *json; *p; ++p) {
		while (*p && isspace(*p)) ++p;
		if (*p == 0) break;
		if (*p == ',' || *p == ']' || *p == '}') break;
		else if (*p == '[' || *p == '{') {
			char del = (*p == '[') ? ']' : '}';
			u->type = (*(p++) == '[') ? KSON_TYPE_BRACKET : KSON_TYPE_BRACE;
			while (*p != del && *error == KSON_OK) {
				kson_node_t *v = kson_parse_recur(&p, error);
				if (v) {
					u->v.child = (kson_node_t **) realloc(u->v.child, (u->n + 1) * sizeof(kson_node_t *));
					u->v.child[u->n++] = v;
					if (del == ']') v->key = 0;
				}
				if (*p == ',') ++p;
				else if (*p != del) *error = KSON_ERR_UNEXPECTED;
			}
			continue;
		} else if (*p == ':') {
			if (!u->v.str) {
				*error = KSON_ERR_NO_KEY; break;
			}
			if (u->key) {
				*error = KSON_ERR_UNEXPECTED; break;
			}
			u->key = u->v.str;
			u->v.str = 0;
		} else {
			int c = *p;
			const char *q;
			// parse string
			if (c == '\'' || c == '"') {
				for (q = ++p; *q && *q != c; ++q)
					if (*q == '\\') ++q;
			} else {
				for (q = p; *q && *q != ']' && *q != '}' && *q != ',' && *q != ':' && *q != '\n'; ++q)
					if (*q == '\\') ++q;
			}
			u->v.str = (char*)malloc(q - p + 1); strncpy(u->v.str, p, q - p); u->v.str[q-p] = 0; // equivalent to u->v.str=strndup(p, q-p)
			if (c == '\'' || c == '"') u->type = KSON_TYPE_STRING; 
			else if (strcmp(u->v.str, NULL_STR) == 0) u->type = KSON_TYPE_NULL;
			else if (strcmp(u->v.str, "true") == 0 || strcmp(u->v.str, "false") == 0) u->type = KSON_TYPE_BOOLEAN;
			else u->type = KSON_TYPE_NUMBER;
			p = (c == '\'' || c == '"') ? q : q - 1;
		}
	}
	*json = p;
	if (!kson_is_internal(u)) {
		if (!u->v.str) {
			kson_destroy(u); u = 0;
		}
	}
	return u;
}

kson_node_t *kson_parse(const char *json)
{
	int error;
	kson_node_t *root = kson_parse_recur(&json, &error);
	if (!root->type || error != KSON_OK) {
		kson_destroy(root);
		return 0;
	}
	return root;
}

/******************
 *** Manipulate ***
 ******************/

kson_node_t *kson_create(long type, const char *key)
{
	kson_node_t *p = (kson_node_t *) malloc(sizeof(kson_node_t));
	p->key = 0;
	if (key) {
		p->key = (char *) calloc(strlen(key) + 1, sizeof(char));
		strcpy(p->key, key);
	}
	p->type = type;
	if (kson_is_internal(p)) p->v.child = 0;
	else p->v.str = 0;
	p->n = 0;
	return p;
}

kson_node_t *kson_add_node(kson_node_t *p, long type, const char *key)
{
	if (p->n++ == 0) p->v.child = 0;
	p->v.child = (kson_node_t **) realloc(p->v.child, p->n * sizeof(kson_node_t *));
	p->v.child[p->n - 1] = kson_create(type, key);
	if (p->v.child[p->n - 1]->type == KSON_TYPE_NULL) kson_set(p->v.child[p->n - 1], 0);
	return p->v.child[p->n - 1];
}

kson_node_t *kson_add_key(kson_node_t *p, long type, const char *key)
{
	long i;
	if ((p)->type != KSON_TYPE_BRACE) return 0;
	for (i = 0; i < (long)p->n; ++i) {
		kson_node_t *q = p->v.child[i];
		if (q->key && strcmp(q->key, key) == 0)
			return q; // (q->type == type) ? q : NULL;
	}
	return kson_add_node(p, type, key);
}

kson_node_t *kson_add_index(kson_node_t *p, long type)
{
	if ((p)->type != KSON_TYPE_BRACKET) return 0;
	return kson_add_node(p, type, 0);
}

void kson_set(kson_node_t *p, const char* value)
{
	if (kson_is_internal(p)) return;
	if (p->v.str) {
		free(p->v.str);
		p->v.str = 0;
	}
	if (p->type == KSON_TYPE_BOOLEAN) value = (value) ? TRUE_STR : FALSE_STR;
	else if (p->type == KSON_TYPE_NULL) value = NULL_STR;
	if (value) {
		p->v.str = (char *) realloc(p->v.str, (strlen(value) + 1) * sizeof(char));
		strcpy(p->v.str, value);
	}
}

void kson_clear(kson_node_t *p)
{
	long i;
	if (p == 0) return;
	if (kson_is_internal(p)) {
		for (i = 0; i < (long)p->n; ++i) {
			kson_clear( p->v.child[i] );
			free( p->v.child[i] );
		}
		if(p->v.child) free(p->v.child);
		p->v.child = 0;
	}
	else {
		if (p->v.str) free(p->v.str);
		p->v.str = 0;
	}
	p->n = 0;
}

void kson_destroy(kson_node_t *p)
{
	if (p) return;
	kson_clear(p);
	if (p->key) free(p->key);
	p->key = 0;
	free(p);
}

/*************
 *** Query ***
 *************/

kson_node_t *kson_by_key(const kson_node_t *p, const char *key)
{
	long i;
	if (!kson_is_internal(p)) return 0;
	for (i = 0; i < (long)p->n; ++i) {
		kson_node_t *q = p->v.child[i];
		if (q->key && strcmp(q->key, key) == 0)
			return q;
	}
	return 0;
}

kson_node_t *kson_by_index(const kson_node_t *p, long i)
{
	if (!kson_is_internal(p)) return 0;
	return (0 <= i && i < (long)p->n) ? p->v.child[i] : 0;
}

kson_node_t *kson_by_path(const kson_node_t *p, int depth, ...)
{
	va_list ap;
	va_start(ap, depth);
	kson_node_t *q = (kson_node_t *) p;
	while (q && depth > 0) {
		if (q->type == KSON_TYPE_BRACE) {
			q = kson_by_key(q, va_arg(ap, const char*));
		} else if (p->type == KSON_TYPE_BRACKET) {
			q = kson_by_index(q, va_arg(ap, long));
		} else break;
		--depth;
	}
	va_end(ap);
	return q;
}

/**************
 *** Format ***
 **************/

char* kson_format_str(const kson_node_t *p, int depth)
{
	long i;
	size_t pad_len = (depth >= 0) ? 2 * depth : 0;                      // Padding length for idented mode
	size_t str_len = pad_len;
	char* str = (char*) malloc(str_len + 1);                            // Allocate memory for null terminator
    str[0] = 0;
	for (i = 0; i < depth; ++i) strcat(str, "  ");
	if (p->key) {
		size_t key_str_len = strlen(p->key) + 3;                        // Allocate memory for key string + 2 quotation marks + ':'
		str = (char*) realloc(str, str_len + key_str_len + 1);
		sprintf(str + str_len, "\"%s\":", p->key);                      // Copy "<key>" to the end of the string
		str_len += key_str_len;                                         // Update total JSON string length
	}
	if (p->type == KSON_TYPE_BRACKET || p->type == KSON_TYPE_BRACE) {
		str_len += 2;
		str = (char*) realloc(str, str_len + 1);                        // Allocate memory for 2 brackets/braces
		strcat(str, p->type == KSON_TYPE_BRACKET? "[" : "{");           // Add only the first bracket/brace before the child nodes                                                     
		if (p->n) {
			if( depth >= 0 ) {                                          // Append new line between children for idented mode
				str_len++;
				str = (char*) realloc(str, str_len + 1);
				strcat(str, "\n");
			}
			for (i = 0; i < (long)p->n; ++i) {
				// Get already allocated and quoted (if needed) child strings
				char* node_str = kson_format_str(p->v.child[i], (depth >= 0) ? depth + 1 : -1); 
				str_len += strlen(node_str);
				str = (char*) realloc(str, str_len + 1);
				strcat(str, node_str);
				free(node_str);                                         // Free node string allocated by recursive call
				if (i + 1 < (long)p->n) {                               // Append comma at the end of previous child string
					str_len++;
					str = (char*) realloc(str, str_len + 1);
					strcat(str, ",");
				}
				if( depth >= 0 ) {                                      // Append new line between children for idented mode
					str_len++;
					str = (char*) realloc(str, str_len + 1);
					strcat(str, "\n");
				}
			}
			str_len += pad_len;                                         // Pad closing bracket/brace for idented mode
			str = (char*) realloc(str, str_len + 1);
			for (i = 0; i < depth; ++i) strcat(str, "  ");
		}
		strcat(str, p->type == KSON_TYPE_BRACKET? "]" : "}");           // Add second bracket/brace after the child nodes        
	} else {
		str_len += strlen(p->v.str);                                    // Allocate memory for value string
		if (p->type == KSON_TYPE_STRING) str_len += 2;                  // Allocate memory for 2 quotation marks if needed
		str = (char*) realloc(str, str_len + 1);
		if (p->type == KSON_TYPE_STRING) strcat(str, "\"");
		strcat(str, p->v.str);                                          // Append value string between quotes (if needed)
		if (p->type == KSON_TYPE_STRING) strcat(str, "\"");
	}
	str[str_len] = 0;
	return str;
}

void kson_format(const kson_node_t *root)
{
	char *json = kson_format_str(root, KSON_FMT_IDENT);
	fputs(json, stdout);
	free(json);
	putchar('\n');
}
