#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void kt_for(int n_threads, void (*func)(void*,long,int), void *data, long n);
void kt_pipeline(int n_threads, void *(*func)(void*, int, void*), void *shared_data, int n_steps);

typedef struct {
	FILE *fp;
	int max_lines, buf_size, n_threads;
	char *buf;
} pipeline_t;

typedef struct {
	int n_lines;
	char **lines;
} step_t;

static void worker_for(void *_data, long i, int tid)
{
	step_t *step = (step_t*)_data;
	char *s = step->lines[i];
	int t, l, j;
	l = strlen(s) - 1;
	assert(s[l] == '\n'); // not supporting long lines
	for (j = 0; j < l>>1; ++j)
		t = s[j], s[j] = s[l - 1 - j], s[l - 1 - j] = t;
}

static void *worker_pipeline(void *shared, int step, void *in)
{
	pipeline_t *p = (pipeline_t*)shared;
	if (step == 0) {
		step_t *s;
		s = calloc(1, sizeof(step_t));
		s->lines = calloc(p->max_lines, sizeof(char*));
		while (fgets(p->buf, p->buf_size, p->fp) != 0) {
			s->lines[s->n_lines] = strdup(p->buf);
			if (++s->n_lines >= p->max_lines)
				break;
		}
		if (s->n_lines) return s;
	} else if (step == 1) {
		kt_for(p->n_threads, worker_for, in, ((step_t*)in)->n_lines);
		return in;
	} else if (step == 2) {
		step_t *s = (step_t*)in;
		while (s->n_lines > 0) {
			fputs(s->lines[--s->n_lines], stdout);
			free(s->lines[s->n_lines]);
		}
		free(s->lines); free(s);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	pipeline_t pl;
	if (argc == 1) {
		fprintf(stderr, "Usage: reverse <in.txt> [n_threads]\n");
		return 1;
	}
	pl.fp = strcmp(argv[1], "-")? fopen(argv[1], "r") : stdin;
	if (pl.fp == 0) {
		fprintf(stderr, "ERROR: failed to open the input file.\n");
		return 1;
	}
	pl.max_lines = 4096;
	pl.buf_size = 0x10000;
	pl.n_threads = argc > 2? atoi(argv[2]) : 1;
	pl.buf = calloc(pl.buf_size, 1);
	kt_pipeline(3, worker_pipeline, &pl, 3);
	free(pl.buf);
	if (pl.fp != stdin) fclose(pl.fp);
	return 0;
}
