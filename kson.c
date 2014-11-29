#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "kson.h"

kson_node_t *kson_parse(const char *json, int *_n, int *error, int *parsed_len)
{
	int *stack = 0, top = 0, max = 0, n_a = 0, m_a = 0;
	kson_node_t *a = 0, *u;
	const char *p, *q;

#define __push_back(y) do { \
		if (top == max) { \
			max = max? max<<1 : 4; \
			stack = (int*)realloc(stack, sizeof(int) * max); \
		} \
		stack[top++] = (y); \
	} while (0)

#define __new_node(z) do { \
		if (n_a == m_a) { \
			int old_m = m_a; \
			m_a = m_a? m_a<<1 : 4; \
			a = (kson_node_t*)realloc(a, sizeof(kson_node_t) * m_a); \
			memset(a + old_m, 0, sizeof(kson_node_t) * (m_a - old_m)); \
		} \
		*(z) = &a[n_a++]; \
	} while (0)

	for (p = json; *p; ++p) {
		while (*p && isblank(*p)) ++p;
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
			int i, start, t = *p == ']'? -1 : -2;
			for (i = top - 1; i >= 0 && stack[i] != t; --i);
			if (i < 0) { // error: an extra right bracket
				*error = KSON_ERR_EXTRA_RIGHT;
				break;
			}
			start = i;
			u = &a[stack[start-1]];
			u->key = u->v.str;
			u->n = top - 1 - start;
			u->v.child = (int*)malloc(u->n * sizeof(int));
			for (i = start + 1; i < top; ++i)
				u->v.child[i - start - 1] = stack[i];
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
				for (q = p; *q && *q != ']' && *q != '}' && *q != ',' && *q != ':'; ++q)
					if (*q == '\\') ++q;
			}
			u->v.str = (char*)malloc(q - p + 1); strncpy(u->v.str, p, q - p); u->v.str[q-p] = 0; // equivalent to u->v.str=strndup(p, q-p)
			u->type = c == '\''? KSON_TYPE_SGL_QUOTE : c == '"'? KSON_TYPE_DBL_QUOTE : KSON_TYPE_NO_QUOTE;
			p = c == '\'' || c == '"'? q : q - 1;
		}
	}
	while (*p && isblank(*p)) ++p; // skip trailing blanks
	if (parsed_len) *parsed_len = p - json;
	if (top != 1) *error = KSON_ERR_EXTRA_LEFT;

	free(stack);
	*_n = n_a;
	return a;
}

void kson_print_recur(kson_node_t *nodes, kson_node_t *p)
{
	if (p->key) {
		printf("\"%s\"", p->key);
		if (p->v.str) putchar(':');
	}
	if (p->type == KSON_TYPE_BRACKET || p->type == KSON_TYPE_BRACE) {
		int i;
		putchar(p->type == KSON_TYPE_BRACKET? '[' : '{');
		for (i = 0; i < p->n; ++i) {
			if (i) putchar(',');
			kson_print_recur(nodes, &nodes[p->v.child[i]]);
		}
		putchar(p->type == KSON_TYPE_BRACKET? ']' : '}');
	} else {
		if (p->type != KSON_TYPE_NO_QUOTE)
			putchar(p->type == KSON_TYPE_SGL_QUOTE? '\'' : '"');
		printf("%s", p->v.str);
		if (p->type != KSON_TYPE_NO_QUOTE)
			putchar(p->type == KSON_TYPE_SGL_QUOTE? '\'' : '"');
	}
}

#ifdef KSON_MAIN
int main(int argc, char *argv[])
{
	kson_node_t *nodes;
	int n_nodes, error, parsed_len;
	nodes = kson_parse("{'a' : 1, 'b':[0,'isn\\'t',true],'d':[{}]}", &n_nodes, &error, &parsed_len);
	if (error == 0) {
		kson_print_recur(nodes, &nodes[0]);
		putchar('\n');
	} else {
		printf("Error code: %d\n", error);
	}
	return 0;
}
#endif
