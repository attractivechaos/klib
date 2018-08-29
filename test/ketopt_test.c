#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "ketopt.h"

static void test_opt(int c, int opt, const char *arg)
{
	if (c == 'x') fprintf(stderr, "-x\n");
	else if (c == 'y') fprintf(stderr, "-y %s\n", arg);
	else if (c == 301) fprintf(stderr, "--foo\n");
	else if (c == 302) fprintf(stderr, "--bar %s\n", arg? arg : "(null)");
	else if (c == 303) fprintf(stderr, "--opt %s\n", arg? arg : "(null)");
	else if (c == '?') fprintf(stderr, "unknown option -%c\n", opt? opt : ':');
	else if (c == ':') fprintf(stderr, "missing option argument: -%c\n", opt? opt : ':');
}

static void print_cmd(int argc, char *argv[], int ind)
{
	int i;
	fprintf(stderr, "CMD: %s", argv[0]);
	if (ind > 1) {
		fputs(" [", stderr);
		for (i = 1; i < ind; ++i) {
			if (i != 1) fputc(' ', stderr);
			fputs(argv[i], stderr);
		}
		fputc(']', stderr);
	}
	for (i = ind; i < argc; ++i)
		fprintf(stderr, " %s", argv[i]);
	fputc('\n', stderr);
}

static void test_ketopt(int argc, char *argv[])
{
	static ko_longopt_t longopts[] = {
		{ "foo", ko_no_argument,       301 },
		{ "bar", ko_required_argument, 302 },
		{ "opt", ko_optional_argument, 303 },
		{ NULL, 0, 0 }
	};
	ketopt_t opt = KETOPT_INIT;
	int c;
	fprintf(stderr, "===> ketopt() <===\n");
	while ((c = ketopt(&opt, argc, argv, 1, "xy:", longopts)) >= 0)
		test_opt(c, opt.opt, opt.arg);
	print_cmd(argc, argv, opt.ind);
}

static void test_getopt(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "foo", no_argument,       0, 301 },
		{ "bar", required_argument, 0, 302 },
		{ "opt", optional_argument, 0, 303 },
		{0, 0, 0, 0}
	};
	int c, option_index;
	fprintf(stderr, "===> getopt() <===\n");
	while ((c = getopt_long(argc, argv, ":xy:", long_options, &option_index)) >= 0)
		test_opt(c, optopt, optarg);
	print_cmd(argc, argv, optind);
}

int main(int argc, char *argv[])
{
	int i;
	char **argv2;
	if (argc == 1) {
		fprintf(stderr, "Usage: ketopt_test [options] <argument> [...]\n");
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "  -x           no argument\n");
		fprintf(stderr, "  -y STR       required argument\n");
		fprintf(stderr, "  --foo        no argument\n");
		fprintf(stderr, "  --bar=STR    required argument\n");
		fprintf(stderr, "  --opt[=STR]  optional argument\n");
		fprintf(stderr, "\nExamples:\n");
		fprintf(stderr, "  ketopt_test -xy1 -x arg1 -y -x -- arg2 -x\n");
		fprintf(stderr, "  ketopt_test --foo --bar=1 --bar 2 --opt arg1 --opt=3\n");
		fprintf(stderr, "  ketopt_test arg1 -y\n");
		return 1;
	}
	argv2 = (char**)malloc(sizeof(char*) * argc);
	for (i = 0; i < argc; ++i) argv2[i] = argv[i];
	test_ketopt(argc, argv);
	test_getopt(argc, argv2);
	free(argv2);
	return 0;
}
