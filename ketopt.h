#ifndef KETOPT_H
#define KETOPT_H

#define ko_no_argument       0
#define ko_required_argument 1
#define ko_optional_argument 2

typedef struct {
	int ind;   /* equivalent to optind */
	int opt;   /* equivalent to optopt */
	char *arg; /* equivalent to optarg */
	int longidx; /* index of a long option; or -1 if short */
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

/**
 * Parse command-line options and arguments
 *
 * This fuction has a similar interface to GNU's getopt_long(). Each call
 * parses one option and returns the option name.  s->arg points to the option
 * argument if present. The function returns -1 when all command-line arguments
 * are parsed. In this case, s->ind is the index of the first non-option
 * argument.
 *
 * @param s         status; shall be initialized to KETOPT_INIT on the first call
 * @param argc      length of argv[]
 * @param argv      list of command-line arguments; argv[0] is ignored
 * @param permute   non-zero to move options ahead of non-option arguments
 * @param ostr      option string
 * @param longopts  long options
 *
 * @return ASCII for a short option; ko_longopt_t::val for a long option; -1 if
 *         argv[] is fully processed; '?' for an unknown option or an ambiguous
 *         long option; ':' if an option argument is missing
 */
int ketopt(ketopt_t *s, int argc, char *argv[], int permute, const char *ostr, const ko_longopt_t *longopts);

#ifdef __cplusplus
}
#endif

#endif
