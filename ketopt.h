#ifndef KETOPT_H
#define KETOPT_H

#define ko_no_argument       0
#define ko_required_argument 1
#define ko_optional_argument 2

typedef struct {
	int ind;   /* equivalent to optind */
	int opt;   /* equivalent to optopt */
	char *arg; /* equivalent to optarg */
	/* private variables not intended for external uses */
	int i, pos, n_args;
} ketopt_t;

typedef struct {
	char *name;
	int has_arg;
	int val;
} ko_longopt_t;

#ifdef __cplusplus
extern "C" {
#endif

extern ketopt_t KETOPT_INIT;

int ketopt(ketopt_t *s, int argc, char *argv[], int permute, const char *ostr, const ko_longopt_t *longopts);

#ifdef __cplusplus
}
#endif

#endif
