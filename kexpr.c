#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "kexpr.h"

#define KEO_NULL  0
#define KEO_PLUS  1
#define KEO_MINUS 2
#define KEO_BNOT  3
#define KEO_LNOT  4
#define KEO_POW   5
#define KEO_MUL   6
#define KEO_DIV   7
#define KEO_IDIV  8
#define KEO_MOD   9
#define KEO_ADD  10
#define KEO_SUB  11
#define KEO_LSH  12
#define KEO_RSH  13
#define KEO_LT   14
#define KEO_LE   15
#define KEO_GT   16
#define KEO_GE   17
#define KEO_EQ   18
#define KEO_NE   19
#define KEO_BAND 20
#define KEO_BXOR 21
#define KEO_BOR  22
#define KEO_LAND 23
#define KEO_LOR  24

#define KET_NULL  0
#define KET_VAL   1
#define KET_OP    2
#define KET_FUNC  3

#define KEV_REAL  1
#define KEV_INT   2
#define KEV_STR   3

typedef struct {
	uint32_t ttype:16, vtype:16;
	int32_t op:8, n_args:24;
	char *name, *s;
	double r;
	int64_t i;
} ke1_t;

static int ke_op[25] = {
	0,
	1<<1|1, 1<<1|1, 1<<1|1, 1<<1|1,
	2<<1|1,
	3<<1, 3<<1, 3<<1, 3<<1,
	4<<1, 4<<1,
	5<<1, 5<<1,
	6<<1, 6<<1, 6<<1, 6<<1,
	7<<1, 7<<1,
	8<<1,
	9<<1,
	10<<1,
	11<<1,
	12<<1
};

static char *ke_opstr[] = {
	"",
	"+(1)", "-(1)", "~", "!",
	"**",
	"*", "/", "//", "%",
	"+", "-",
	"<<", ">>",
	"<", "<=", ">", ">=",
	"==", "!=",
	"&",
	"^",
	"|",
	"&&",
	"||"
};

struct kexpr_s {
	int n;
	ke1_t *e;
};


static ke1_t ke_read_token(char *p, char **r, int *err, int last_is_val) // it doesn't parse parentheses
{
	char *q = p;
	ke1_t e;
	memset(&e, 0, sizeof(ke1_t));
	if (isalpha(*p) || *p == '_') { // a variable or a function
		for (; *p && (*p == '_' || isalnum(*p)); ++p);
		if (*p == '(') e.ttype = KET_FUNC, e.n_args = 1;
		else e.ttype = KET_VAL, e.vtype = KEV_REAL;
		e.name = strndup(q, p - q);
		e.i = 0, e.r = 0.;
		*r = p;
	} else if (isdigit(*p)) { // a number
		long x;
		double y;
		char *pp;
		e.ttype = KET_VAL;
		y = strtod(q, &p);
		x = strtol(q, &pp, 0);
		if (p > pp) {
			e.vtype = KEV_REAL;
			e.i = (int64_t)(y + .5), e.r = y;
			*r = p;
		} else {
			e.vtype = KEV_INT;
			e.i = x, e.r = x;
			*r = pp;
		}
	} else if (*p == '"') { // a string value
		for (++p; *p && *p != '"'; ++p); // TODO: support escaping
		if (*p == '"') {
			e.ttype = KET_VAL;
			e.vtype = KEV_STR;
			e.s = strndup(q + 1, p - q - 1);
			*r = p + 1;
		} else *err |= KEE_UNDQ, *r = p;
	} else {
		e.ttype = KET_OP;
		if (*p == '*' && p[1] == '*') e.op = KEO_POW, *r = q + 2;
		else if (*p == '*') e.op = KEO_MUL, *r = q + 1; // FIXME: NOT working for unary operators
		else if (*p == '/' && p[1] == '/') e.op = KEO_IDIV, *r = q + 2;
		else if (*p == '/') e.op = KEO_DIV, *r = q + 1;
		else if (*p == '%') e.op = KEO_MOD, *r = q + 1;
		else if (*p == '+') e.op = last_is_val? KEO_ADD : KEO_PLUS, *r = q + 1;
		else if (*p == '-') e.op = last_is_val? KEO_SUB : KEO_MINUS, *r = q + 1;
		else if (*p == '=' && p[1] == '=') e.op = KEO_EQ, *r = q + 2;
		else if (*p == '!' && p[1] == '=') e.op = KEO_NE, *r = q + 2;
		else if (*p == '>' && p[1] == '=') e.op = KEO_GE, *r = q + 2;
		else if (*p == '<' && p[1] == '=') e.op = KEO_LE, *r = q + 2;
		else if (*p == '>' && p[1] == '>') e.op = KEO_RSH, *r = q + 2;
		else if (*p == '<' && p[1] == '<') e.op = KEO_LSH, *r = q + 2;
		else if (*p == '>') e.op = KEO_GT, *r = q + 1;
		else if (*p == '<') e.op = KEO_LT, *r = q + 1;
		else if (*p == '|' && p[1] == '|') e.op = KEO_LOR, *r = q + 2;
		else if (*p == '&' && p[1] == '&') e.op = KEO_LAND, *r = q + 2;
		else if (*p == '|') e.op = KEO_BOR, *r = q + 1;
		else if (*p == '&') e.op = KEO_BAND, *r = q + 1;
		else if (*p == '^') e.op = KEO_BXOR, *r = q + 1;
		else if (*p == '~') e.op = KEO_BNOT, *r = q + 1;
		else if (*p == '!') e.op = KEO_LNOT, *r = q + 1;
		else e.ttype = KET_NULL, *err |= KEE_UNTO;
	}
	return e;
}

static inline ke1_t *push_back(ke1_t **a, int *n, int *m)
{
	if (*n == *m) {
		int old_m = *m;
		*m = *m? *m<<1 : 8;
		*a = (ke1_t*)realloc(*a, *m * sizeof(ke1_t));
		memset(*a + old_m, 0, (*m - old_m) * sizeof(ke1_t));
	}
	return &(*a)[(*n)++];
}

static ke1_t *ke_parse_core(const char *_s, int *_n, int *err)
{
	char *s, *p, *q;
	int n_out, m_out, n_op, m_op, last_is_val = 0;
	ke1_t *out, *op, *t, *u;

	*err = 0; *_n = 0;
	s = strdup(_s); // make a copy
	for (p = q = s; *p; ++p) // squeeze out spaces
		if (!isspace(*p)) *q++ = *p;
	*q++ = 0;

	out = op = 0;
	n_out = m_out = n_op = m_op = 0;
	p = s;
	while (*p) {
		if (*p == '(') {
			t = push_back(&op, &n_op, &m_op);
			t->op = -1, t->ttype = KET_NULL;
			++p;
		} else if (*p == ')') {
			while (n_op > 0 && op[n_op-1].op >= 0) {
				u = push_back(&out, &n_out, &m_out);
				*u = op[--n_op];
			}
			if (n_op < 0) {
				*err |= KEE_UNRP;
				break;
			} else --n_op; // pop out '('
			if (n_op > 0 && op[n_op-1].ttype == KET_FUNC) {
				u = push_back(&out, &n_out, &m_out);
				*u = op[--n_op];
			}
			++p;
		} else if (*p == ',') {
			while (n_op > 0 && op[n_op-1].op >= 0) {
				u = push_back(&out, &n_out, &m_out);
				*u = op[--n_op];
			}
			if (n_op < 2 || op[n_op-2].ttype != KET_FUNC) {
				*err |= KEE_FUNC;
				break;
			}
			++op[n_op-2].n_args;
			++p;
		} else { // output-able token
			ke1_t v;
			v = ke_read_token(p, &p, err, last_is_val);
			if (*err) break;
			if (v.ttype == KET_VAL) {
				u = push_back(&out, &n_out, &m_out);
				*u = v;
				last_is_val = 1;
			} else if (v.ttype == KET_FUNC) {
				t = push_back(&op, &n_op, &m_op);
				*t = v;
				last_is_val = 0;
			} else if (v.ttype == KET_OP) {
				int oi = ke_op[v.op];
				while (n_op > 0 && op[n_op-1].ttype == KET_OP) {
					int pre = ke_op[op[n_op-1].op]>>1;
					if (((oi&1) && oi>>1 <= pre) || (!(oi&1) && oi>>1 < pre)) break;
					u = push_back(&out, &n_out, &m_out);
					*u = op[--n_op];
				}
				t = push_back(&op, &n_op, &m_op);
				*t = v;
				last_is_val = 0;
			}
		}
	}

	if (*err == 0) {
		while (n_op > 0 && op[n_op-1].op >= 0) {
			u = push_back(&out, &n_out, &m_out);
			*u = op[--n_op];
		}
		if (n_op > 0) *err |= KEE_UNLP;
	}

	free(op); free(s);
	if (*err) {
		free(out);
		return 0;
	}
	*_n = n_out;
	return out;
}

kexpr_t *ke_parse(const char *_s, int *err)
{
	int n;
	ke1_t *a;
	kexpr_t *ke;
	a = ke_parse_core(_s, &n, err);
	if (*err) return 0;
	ke = (kexpr_t*)calloc(1, sizeof(kexpr_t));
	ke->n = n;
	ke->e = a;
	return ke;
}

int ke_eval(const kexpr_t *ke, int64_t *_i, double *_r)
{
#define _do_cmp(_op) do { \
		q = &stack[--top], p = &stack[top-1]; \
		if (p->vtype == KEV_STR && q->vtype == KEV_STR) p->i = (strcmp(p->s, q->s) _op 0); \
		else p->i = p->vtype == KEV_REAL || q->vtype == KEV_REAL? (p->r _op q->r) : (p->i _op q->i); \
		p->r = (double)p->i; \
		p->vtype = KEV_INT; \
	} while (0)
#define _do_bin_both(_op) do { \
		q = &stack[--top], p = &stack[top-1]; \
		p->i _op q->i, p->r _op q->r; \
		p->vtype = p->vtype == KEV_REAL || q->vtype == KEV_REAL? KEV_REAL : KEV_INT; \
	} while (0)
#define _do_bin_int(_op) do { \
		q = &stack[--top], p = &stack[top-1]; \
		p->i _op q->i; p->r = (double)p->i; \
		p->vtype = KEV_INT; \
	} while (0)
#define _do_func1(_func) do { \
		p = &stack[top-1]; \
		p->r = _func(p->r); p->i = (int64_t)(p->r + .5); \
		p->vtype = KEV_REAL; \
	} while (0)

	ke1_t *stack, *p, *q;
	int i, top = 0, err = 0;
	stack = (ke1_t*)malloc(ke->n * sizeof(ke1_t));
	for (i = 0; i < ke->n; ++i) {
		ke1_t *e = &ke->e[i];
		if (e->ttype == KET_VAL) {
			stack[top++] = *e;
		} else if (e->ttype == KET_OP) {
			if (top == 0) break;
			if (e->op == KEO_ADD) {
				_do_bin_both(+=);
			} else if (e->op == KEO_SUB) {
				_do_bin_both(-=);
			} else if (e->op == KEO_MUL) {
				_do_bin_both(*=);
			} else if (e->op == KEO_DIV) {
				q = &stack[--top], p = &stack[top-1];
				p->r /= q->r; p->i = (int64_t)(p->r + .5);
				p->vtype = KEV_REAL;
			} else if (e->op == KEO_MINUS) {
				p = &stack[top-1];
				p->i = -p->i, p->r = -p->r;
			} else if (e->op == KEO_PLUS) {
			} else if (e->op == KEO_LAND) {
				q = &stack[--top], p = &stack[top-1];
				p->i = (p->i && q->i); p->r = p->i;
				p->vtype = KEV_INT;
			} else if (e->op == KEO_LOR) {
				q = &stack[--top], p = &stack[top-1];
				p->i = (p->i || q->i); p->r = p->i;
				p->vtype = KEV_INT;
			} else if (e->op == KEO_POW) {
				q = &stack[--top], p = &stack[top-1];
				p->r = pow(p->r, q->r), p->i = (int64_t)(p->r + .5);
				p->vtype = p->vtype == KEV_REAL || q->vtype == KEV_REAL? KEV_REAL : KEV_INT;
			} else if (e->op == KEO_LT) {
				_do_cmp(<);
			} else if (e->op == KEO_LE) {
				_do_cmp(<=);
			} else if (e->op == KEO_GT) {
				_do_cmp(>);
			} else if (e->op == KEO_GE) {
				_do_cmp(>=);
			} else if (e->op == KEO_EQ) {
				_do_cmp(==);
			} else if (e->op == KEO_NE) {
				_do_cmp(!=);
			} else if (e->op == KEO_BAND) {
				_do_bin_int(&=);
			} else if (e->op == KEO_BOR) {
				_do_bin_int(|=);
			} else if (e->op == KEO_BXOR) {
				_do_bin_int(^=);
			} else if (e->op == KEO_LSH) {
				_do_bin_int(<<=);
			} else if (e->op == KEO_RSH) {
				_do_bin_int(>>=);
			} else if (e->op == KEO_MOD) {
				_do_bin_int(%=);
			} else if (e->op == KEO_IDIV) {
				_do_bin_int(/=);
			} else if (e->op == KEO_BNOT) {
				p = &stack[top-1];
				p->i = ~p->i; p->r = (double)p->i;
				p->vtype = KEV_INT;
			} else if (e->op == KEO_LNOT) {
				p = &stack[top-1];
				p->i = !p->i; p->r = (double)p->i;
				p->vtype = KEV_INT;
			}
		} else if (e->ttype == KET_FUNC) {
			if (strcmp(e->name, "abs") == 0) {
				p = &stack[top-1];
				if (p->vtype == KEV_INT) p->i = abs(p->i), p->r = (double)p->i;
				else p->r = fabs(p->r), p->i = (int64_t)(p->r + .5);
			} else if (strcmp(e->name, "log") == 0) _do_func1(log);
			else if (strcmp(e->name, "exp") == 0) _do_func1(exp);
			else if (strcmp(e->name, "log2") == 0) _do_func1(log2);
			else if (strcmp(e->name, "exp2") == 0) _do_func1(exp2);
			else if (strcmp(e->name, "log10") == 0) _do_func1(log10);
			else if (strcmp(e->name, "sqrt") == 0) _do_func1(sqrt);
		}
	}
	if (top != 1) err |= KEE_ARG;
	free(stack);
	*_i = stack->i, *_r = stack->r;
	return err;

#undef _do_bin_int
#undef _do_bin_both
#undef _do_cmp
}

void ke_destroy(kexpr_t *ke)
{
	int i;
	if (ke == 0) return;
	for (i = 0; i < ke->n; ++i) {
		free(ke->e[i].name);
		free(ke->e[i].s);
	}
	free(ke->e); free(ke);
}

int ke_set_int(kexpr_t *ke, const char *var, int64_t x)
{
	int i, n = 0;
	double xx = (double)x;
	for (i = 0; i < ke->n; ++i) {
		ke1_t *e = &ke->e[i];
		if (e->ttype == KET_VAL && e->name && strcmp(e->name, var) == 0)
			e->i = x, e->r = xx, e->ttype = KEV_INT, ++n;
	}
	return n;
}

int ke_set_real(kexpr_t *ke, const char *var, double x)
{
	int i, n = 0;
	int64_t xx = (int64_t)(x + .5);
	for (i = 0; i < ke->n; ++i) {
		ke1_t *e = &ke->e[i];
		if (e->ttype == KET_VAL && e->name && strcmp(e->name, var) == 0)
			e->r = x, e->i = xx, e->ttype = KEV_REAL, ++n;
	}
	return n;
}

int ke_set_str(kexpr_t *ke, const char *var, const char *x)
{
	int i, n = 0;
	for (i = 0; i < ke->n; ++i) {
		ke1_t *e = &ke->e[i];
		if (e->ttype == KET_VAL && e->name && strcmp(e->name, var) == 0) {
			if (e->vtype == KEV_STR) free(e->s);
			e->s = strdup(x);
			e->i = 0, e->r = 0.;
			e->vtype = KEV_STR;
		}
	}
	return n;
}

void ke_print(const kexpr_t *ke)
{
	int i;
	if (ke == 0) return;
	for (i = 0; i < ke->n; ++i) {
		const ke1_t *u = &ke->e[i];
		if (i) putchar(' ');
		if (u->ttype == KET_VAL) {
			if (u->name) printf("%s", u->name);
			else if (u->vtype == KEV_REAL) printf("%g", u->r);
			else if (u->vtype == KEV_INT) printf("%lld", (long long)u->i);
			else if (u->vtype == KEV_STR) printf("\"%s\"", u->s);
		} else if (u->ttype == KET_OP) {
			printf("%s", ke_opstr[u->op]);
		} else if (u->ttype == KET_FUNC) {
			printf("%s(%d)", u->name, u->n_args);
		}
	}
	putchar('\n');
}


#ifdef KE_MAIN
#include <unistd.h>

int main(int argc, char *argv[])
{
	int c, err, to_print = 0, is_int = 0;
	kexpr_t *ke;

	while ((c = getopt(argc, argv, "pi")) >= 0) {
		if (c == 'p') to_print = 1;
		else if (c == 'i') is_int = 1;
	}
	if (optind == argc) {
		fprintf(stderr, "Usage: %s [-pi] <expr>\n", argv[0]);
		return 1;
	}
	ke = ke_parse(argv[optind], &err);
	if (err) {
		fprintf(stderr, "ERROR: 0x%x\n", err);
		return 1;
	}
	if (!to_print) {
		int64_t vi;
		double vr;
		err = ke_eval(ke, &vi, &vr);
		if (err) {
			fprintf(stderr, "ERROR: 0x%x\n", err);
			return 1;
		}
		if (is_int) printf("%lld\n", (long long)vi);
		else printf("%g\n", vr);
	} else ke_print(ke);
	ke_destroy(ke);
	return 0;
}
#endif
