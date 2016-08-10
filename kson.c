#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include "kson.h"

/*************
 *** Parse ***
 *************/

kson_node_t *kson_parse_core(const char *json, long *_n, int *error, long *parsed_len)
{
	long *stack = 0, top = 0, max = 0, n_a = 0, m_a = 0, i, j;
	kson_node_t *a = 0, *u;
	const char *p, *q;
	size_t *tmp;

#define __push_back(y) do { \
		if (top == max) { \
			max = max? max<<1 : 4; \
			stack = (long*)realloc(stack, sizeof(long) * max); \
		} \
		stack[top++] = (y); \
	} while (0)

#define __new_node(z) do { \
		if (n_a == m_a) { \
			long old_m = m_a; \
			m_a = m_a? m_a<<1 : 4; \
			a = (kson_node_t*)realloc(a, sizeof(kson_node_t) * m_a); \
			memset(a + old_m, 0, sizeof(kson_node_t) * (m_a - old_m)); \
		} \
		*(z) = &a[n_a++]; \
	} while (0)

	assert(sizeof(size_t) == sizeof(kson_node_t*));
	*error = KSON_OK;
	for (p = json; *p; ++p) {
		while (*p && isspace(*p)) ++p;
		if (*p == 0) break;
		if (*p == ',') { // comma is somewhat redundant
		} else if (*p == '[' || *p == '{') {
			int t = *p == '['? -1 : -2;
			if (top < 2 || stack[top-1] != -3) { // unnamed internal node
				__push_back(n_a);
				__new_node(&u);
				__push_back(t);
			} else stack[top-1] = t; // named internal node
		} else if (*p == ']' || *p == '}') {
			long i, start, t = *p == ']'? -1 : -2;
			for (i = top - 1; i >= 0 && stack[i] != t; --i);
			if (i < 0) { // error: an extra right bracket
				*error = KSON_ERR_EXTRA_RIGHT;
				break;
			}
			start = i;
			u = &a[stack[start-1]];
			u->key = u->v.str;
			u->n = top - 1 - start;
			u->v.child = (kson_node_t**)malloc(u->n * sizeof(kson_node_t*));
			tmp = (size_t*)u->v.child;
			for (i = start + 1; i < top; ++i)
				tmp[i - start - 1] = stack[i];
			u->type = *p == ']'? KSON_TYPE_BRACKET : KSON_TYPE_BRACE;
			if ((top = start) == 1) break; // completed one object; remaining characters discarded
		} else if (*p == ':') {
			if (top == 0 || stack[top-1] == -3) {
				*error = KSON_ERR_NO_KEY;
				break;
			}
			__push_back(-3);
		} else {
			int c = *p;
			// get the node to modify
			if (top >= 2 && stack[top-1] == -3) { // we have a key:value pair here
				--top;
				u = &a[stack[top-1]];
				u->key = u->v.str; // move old value to key
			} else { // don't know if this is a bare value or a key:value pair; keep it as a value for now
				__push_back(n_a);
				__new_node(&u);
			}
			// parse string
			if (c == '\'' || c == '"') {
				for (q = ++p; *q && *q != c; ++q)
					if (*q == '\\') ++q;
			} else {
				for (q = p; *q && *q != ']' && *q != '}' && *q != ',' && *q != ':' && *q != '\n'; ++q)
					if (*q == '\\') ++q;
			}
			u->v.str = (char*)malloc(q - p + 1); strncpy(u->v.str, p, q - p); u->v.str[q-p] = 0; // equivalent to u->v.str=strndup(p, q-p)
			u->type = c == '\''? KSON_TYPE_SGL_QUOTE : c == '"'? KSON_TYPE_DBL_QUOTE : KSON_TYPE_NO_QUOTE;
			p = c == '\'' || c == '"'? q : q - 1;
		}
	}
	while (*p && isspace(*p)) ++p; // skip trailing blanks
	if (parsed_len) *parsed_len = p - json;
	if (top != 1) *error = KSON_ERR_EXTRA_LEFT;

	for (i = 0; i < n_a; ++i)
		for (j = 0, u = &a[i], tmp = (size_t*)u->v.child; j < (long)u->n; ++j)
			u->v.child[j] = &a[tmp[j]];

	free(stack);
	*_n = n_a;
	return a;
}

kson_t *kson_parse(const char *json)
{
	kson_t *kson;
	int error;
	kson = (kson_t*)calloc(1, sizeof(kson_t));
	kson->root = kson_parse_core(json, &kson->n_nodes, &error, 0);
	if (error) {
		kson_destroy(kson);
		return 0;
	}
	return kson;
}

/******************
 *** Manipulate ***
 ******************/

kson_node_t *kson_create_node(const kson_node_t *p, long type, const char *key)
{
    kson_node_t *r = (kson_node_t *) malloc(sizeof(kson_node_t));
    if (key) {
        r->key = (char *) calloc(strlen(key) + 1, sizeof(char))
        strcpy(r->key, key);
    }
    else
        r->key = 0;
    r->type = type;
    r->n = 0;
    if (kson_is_internal(r)) r->v.child = 0;
    else r->v.str = 0;
	return r;
}

kson_node_t *kson_add_key(const kson_node_t *p, long type, const char *key)
{
	long i;
    kson_node_t *r = 0;
	if ((p)->type != KSON_TYPE_BRACE) return 0;
	for (i = 0; i < (long)p->n; ++i) {
		const kson_node_t *q = p->v.child[i];
		if (q->key && strcmp(q->key, key) == 0)
			r = q;
	}
	if (r == 0) r = kson_create_node(p, type, key);
	return r;
}

kson_node_t *kson_add_index(const kson_node_t *p, long type)
{
	if ((p)->type != KSON_TYPE_BRACKET) return 0;
    p->v.child = (kson_node_t *) realloc(p->v.child, p->n + 1);
    p->v.child[p->n] = kson_create_node(p, type, 0);
	return p->v.child[p->n];
}

void kson_set(const kson_node_t *p, const char* value)
{
    if (kson_is_internal(p)) return;
    if (value) {
        p->v.str = (char *) calloc(strlen(value) + 1, sizeof(char))
        strcpy(p->v.str, value);
    }
    else
        p->v.str = 0;
}

void kson_clear_node(const kson_node_t *p)
{
	long i;
	if (p == 0) return;
    if (kson_is_internal(p)) {
        //for (i = 0; i < (long)p->n; ++i) {
        //    kson_clear_node( p->v.child[i] );
        //    free( p->v.child[i] );
        //}
        if(p->v.child) free(p->v.child);
        p->v.child = 0;
        p->n = 0;
    }
    else {
        //if (p->v.str) free(p->v.str);
        p->v.str = 0;
    }
    
    free(p->key);
}

void kson_destroy(kson_t *kson)
{
	long i;
	if (kson == 0) return;
	for (i = 0; i < kson->n_nodes; ++i) {
		free(kson->root[i].key); free(kson->root[i].v.str);
	}
	free(kson->root); free(kson);
}

/*************
 *** Query ***
 *************/

const kson_node_t *kson_by_key(const kson_node_t *p, const char *key)
{
	long i;
	if (!kson_is_internal(p)) return 0;
	for (i = 0; i < (long)p->n; ++i) {
		const kson_node_t *q = p->v.child[i];
		if (q->key && strcmp(q->key, key) == 0)
			return q;
	}
	return 0;
}

const kson_node_t *kson_by_index(const kson_node_t *p, long i)
{
	if (!kson_is_internal(p)) return 0;
	return 0 <= i && i < (long)p->n? p->v.child[i] : 0;
}

const kson_node_t *kson_by_path(const kson_node_t *p, int depth, ...)
{
	va_list ap;
	va_start(ap, depth);
	while (p && depth > 0) {
		if (p->type == KSON_TYPE_BRACE) {
			p = kson_by_key(p, va_arg(ap, const char*));
		} else if (p->type == KSON_TYPE_BRACKET) {
			p = kson_by_index(p, va_arg(ap, long));
		} else break;
		--depth;
	}
	va_end(ap);
	return p;
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
	for (i = 0; i < depth; ++i) strcat(str, "  ");
	if (p->key) {
		size_t key_str_len = strlen(p->key) + 2;                        // Allocate memory for key string + 2 quotation marks
		str = (char*) realloc(str, str_len + key_str_len + 1);
		sprintf(str + str_len, "\"%s\":", p->key);                      // Copy "<key>" to the end of the string
		str_len += key_str_len;                                         // Update total JSON string length
	}
	if (p->type == KSON_TYPE_BRACKET || p->type == KSON_TYPE_BRACE) {
		str_len += 2;
		str = (char*) realloc(str, str_len + 1);                        // Allocate memory for 2 brackets/braces
		strcat(str, p->type == KSON_TYPE_BRACKET? "[" : "{");           // Add only the first bracket/brace before the child nodes                                                     
		if (p->n) {
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
		if (p->type != KSON_TYPE_NO_QUOTE) str_len += 2;                // Allocate memory for 2 quotation marks if needed
		str = (char*) realloc(str, str_len + 1);
		if (p->type != KSON_TYPE_NO_QUOTE)
			strcat(str, p->type == KSON_TYPE_SGL_QUOTE? "\'" : "\"");
		strcat(str, p->v.str);                                          // Append value string between quotes (if needed)
		if (p->type != KSON_TYPE_NO_QUOTE)
			strcat(str, p->type == KSON_TYPE_SGL_QUOTE? "\'" : "\"");
	}
	s[str_len] = '\0'
	return s;
}

void kson_format_recur(const kson_node_t *p, int depth)
{
	long i;
	if (p->key) printf("\"%s\":", p->key);
	if (p->type == KSON_TYPE_BRACKET || p->type == KSON_TYPE_BRACE) {
		putchar(p->type == KSON_TYPE_BRACKET? '[' : '{');
		if (p->n) {
			putchar('\n'); for (i = 0; i <= depth; ++i) fputs("  ", stdout);
			for (i = 0; i < (long)p->n; ++i) {
				if (i) {
					int i;
					putchar(',');
					putchar('\n'); for (i = 0; i <= depth; ++i) fputs("  ", stdout);
				}
				kson_format_recur(p->v.child[i], depth + 1);
			}
			putchar('\n'); for (i = 0; i < depth; ++i) fputs("  ", stdout);
		}
		putchar(p->type == KSON_TYPE_BRACKET? ']' : '}');
	} else {
		if (p->type != KSON_TYPE_NO_QUOTE)
			putchar(p->type == KSON_TYPE_SGL_QUOTE? '\'' : '"');
		fputs(p->v.str, stdout);
		if (p->type != KSON_TYPE_NO_QUOTE)
			putchar(p->type == KSON_TYPE_SGL_QUOTE? '\'' : '"');
	}
}

void kson_format(const kson_node_t *root)
{
	kson_format_recur(root, 0);
	putchar('\n');
}

/*********************
 *** Main function ***
 *********************/

#ifdef KSON_MAIN
#define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
int main(int argc, char *argv[])
{
	kson_t *kson = 0;
	if (argc > 1) {
		FILE *fp;
		int len = 0, max = 0, tmp, i;
		char *json = 0, buf[0x10000];
		if ((fp = fopen(argv[1], "rb")) != 0) {
			// read the entire file into a string
			while ((tmp = fread(buf, 1, 0x10000, fp)) != 0) {
				if (len + tmp + 1 > max) {
					max = len + tmp + 1;
					kroundup32(max);
					json = (char*)realloc(json, max);
				}
				memcpy(json + len, buf, tmp);
				len += tmp;
			}
			fclose(fp);
			// parse
			kson = kson_parse(json);
			free(json);
			if (kson) {
				kson_format(kson->root);
				if (argc > 2) {
					// path finding
					const kson_node_t *p = kson->root;
					for (i = 2; i < argc && p; ++i) {
						if (p->type == KSON_TYPE_BRACKET)
							p = kson_by_index(p, atoi(argv[i]));
						else if (p->type == KSON_TYPE_BRACE)
							p = kson_by_key(p, argv[i]);
						else p = 0;
					}
					if (p) {
						if (kson_is_internal(p)) printf("Reached an internal node\n");
						else printf("Value: %s\n", p->v.str);
					} else printf("Failed to find the slot\n");
				}
			} else printf("Failed to parse\n");
		}
	} else {
		kson = kson_parse("{'a' : 1,'b':[0,'isn\\'t',true],'d':[{\n\n\n}]}");
		if (kson) {
			const kson_node_t *p = kson_by_path(kson->root, 2, "b", 1);
			if (p) printf("*** %s\n", p->v.str);
			else printf("!!! not found\n");
			kson_format(kson->root);
		} else {
			printf("Failed to parse\n");
		}
	}
	kson_destroy(kson);
	return 0;
}
#endif
