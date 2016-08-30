#include "kson.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
int main(int argc, char *argv[])
{
	kson_node_t *kson = 0;
	if (argc > 1) {
		printf("Parsing file:\n");
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
				kson_format(kson);
				if (argc > 2) {
					// path finding
					const kson_node_t *p = kson;
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
		printf("Parsing string:\n");
		kson = kson_parse("{'a' : 1,'b':[0,'isn\\'t',true],'d':[{\n\n\n}]}");
		if (kson) {
			kson_node_t *p = kson_by_path(kson, 2, "b", 1);
			if (p) printf("*** %s\n", p->v.str);
			else printf("!!! not found\n");
			kson_format(kson);
		} else {
			printf("Failed to parse\n");
		}
		printf("New object:\n");
		kson_destroy(kson);
		kson = kson_create(KSON_TYPE_BRACE, 0);
		kson_node_t *b = kson_add_key(kson, KSON_TYPE_BOOLEAN, "e");
		kson_set(b, 0);
		b = kson_add_key(kson, KSON_TYPE_STRING, "f");
		kson_set(b, "hey");
		b = kson_add_key(kson, KSON_TYPE_NUMBER, "g");
		kson_set(b, "0.0");
		b = kson_add_key(kson, KSON_TYPE_BRACKET, "h");
		b = kson_add_index(b, KSON_TYPE_NULL);
		kson_format(kson);
	}
	kson_destroy(kson);
	return 0;
}
