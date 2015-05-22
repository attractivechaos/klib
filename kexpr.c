#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "kexpr.h"

/***************
 * Definitions *
 ***************/

#define KEO_NULL  0
#define KEO_POS   1
#define KEO_NEG   2
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

#define KEF_NULL  0
#define KEF_REAL  1

struct ke1_s;

typedef struct ke1_s {
	uint32_t ttype:16, vtype:10, assigned:1, user_func:5; // ttype: token type; vtype: value type
	int32_t op:8, n_args:24; // op: operator, n_args: number of arguments
	char *name; // variable name or function name
	union {
		void (*builtin)(struct ke1_s *a, struct ke1_s *b); // execution function
		double (*real_func1)(double);
		double (*real_func2)(double, double);
	} f;
	double r;
	int64_t i;
	char *s;
} ke1_t;

static int ke_op[25] = {
	0,
	1<<1|1, 1<<1|1, 1<<1|1, 1<<1|1, // unary operators
	2<<1|1, // pow()
	3<<1, 3<<1, 3<<1, 3<<1, // * / // %
	4<<1, 4<<1, // + and -
	5<<1, 5<<1, // << and >>
	6<<1, 6<<1, 6<<1, 6<<1, // < > <= >=
	7<<1, 7<<1, // == !=
	8<<1, // &
	9<<1, // ^
	10<<1,// |
	11<<1,// &&
	12<<1 // ||
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

/**********************
 * Operator functions *
 **********************/

#define KE_GEN_CMP(_type, _op) \
	static void ke_op_##_type(ke1_t *p, ke1_t *q) { \
		if (p->vtype == KEV_STR && q->vtype == KEV_STR) p->i = (strcmp(p->s, q->s) _op 0); \
		else p->i = p->vtype == KEV_REAL || q->vtype == KEV_REAL? (p->r _op q->r) : (p->i _op q->i); \
		p->r = (double)p->i; \
		p->vtype = KEV_INT; \
	}

KE_GEN_CMP(KEO_LT, <)
KE_GEN_CMP(KEO_LE, <=)
KE_GEN_CMP(KEO_GT, >)
KE_GEN_CMP(KEO_GE, >=)
KE_GEN_CMP(KEO_EQ, ==)
KE_GEN_CMP(KEO_NE, !=)

#define KE_GEN_BIN_INT(_type, _op) \
	static void ke_op_##_type(ke1_t *p, ke1_t *q) { \
		p->i _op q->i; p->r = (double)p->i; \
		p->vtype = KEV_INT; \
	}

KE_GEN_BIN_INT(KEO_BAND, &=)
KE_GEN_BIN_INT(KEO_BOR, |=)
KE_GEN_BIN_INT(KEO_BXOR, ^=)
KE_GEN_BIN_INT(KEO_LSH, <<=)
KE_GEN_BIN_INT(KEO_RSH, >>=)
KE_GEN_BIN_INT(KEO_MOD, %=)
KE_GEN_BIN_INT(KEO_IDIV, /=)

#define KE_GEN_BIN_BOTH(_type, _op) \
	static void ke_op_##_type(ke1_t *p, ke1_t *q) { \
		p->i _op q->i; p->r _op q->r; \
		p->vtype = p->vtype == KEV_REAL || q->vtype == KEV_REAL? KEV_REAL : KEV_INT; \
	}

KE_GEN_BIN_BOTH(KEO_ADD, +=)
KE_GEN_BIN_BOTH(KEO_SUB, -=)
KE_GEN_BIN_BOTH(KEO_MUL, *=)

static void ke_op_KEO_DIV(ke1_t *p, ke1_t *q)  { p->r /= q->r, p->i = (int64_t)(p->r + .5); p->vtype = KEV_REAL; }
static void ke_op_KEO_LAND(ke1_t *p, ke1_t *q) { p->i = (p->i && q->i); p->r = p->i; p->vtype = KEV_INT; }
static void ke_op_KEO_LOR(ke1_t *p, ke1_t *q)  { p->i = (p->i || q->i); p->r = p->i; p->vtype = KEV_INT; }
static void ke_op_KEO_POW(ke1_t *p, ke1_t *q)  { p->r = pow(p->r, q->r), p->i = (int64_t)(p->r + .5); p->vtype = p->vtype == KEV_REAL || q->vtype == KEV_REAL? KEV_REAL : KEV_INT; }
static void ke_op_KEO_BNOT(ke1_t *p, ke1_t *q) { p->i = ~p->i; p->r = (double)p->i; p->vtype = KEV_INT; }
static void ke_op_KEO_LNOT(ke1_t *p, ke1_t *q) { p->i = !p->i; p->r = (double)p->i; p->vtype = KEV_INT; }
static void ke_op_KEO_POS(ke1_t *p, ke1_t *q)  { } // do nothing
static void ke_op_KEO_NEG(ke1_t *p, ke1_t *q)  { p->i = -p->i, p->r = -p->r; }

static void ke_func1_abs(ke1_t *p, ke1_t *q) { if (p->vtype == KEV_INT) p->i = abs(p->i), p->r = (double)p->i; else p->r = fabs(p->r), p->i = (int64_t)(p->r + .5); }

/**********
 * Parser *
 **********/

// parse a token except "(", ")" and ","
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
		x = strtol(q, &pp, 0); // FIXME: check int/double parsing errors
		if (p > pp) { // has "." or "[eE]"; then it is a real number
			e.vtype = KEV_REAL;
			e.i = (int64_t)(y + .5), e.r = y;
			*r = p;
		} else {
			e.vtype = KEV_INT;
			e.i = x, e.r = y;
			*r = pp;
		}
	} else if (*p == '"' || *p == '\'') { // a string value
		int c = *p;
		for (++p; *p && *p != c; ++p)
			if (*p == '\\') ++p; // escaping
		if (*p == c) {
			e.ttype = KET_VAL, e.vtype = KEV_STR;
			e.s = strndup(q + 1, p - q - 1);
			*r = p + 1;
		} else *err |= KEE_UNQU, *r = p;
	} else { // an operator
		e.ttype = KET_OP;
		if (*p == '*' && p[1] == '*') e.op = KEO_POW, e.f.builtin = ke_op_KEO_POW, e.n_args = 2, *r = q + 2;
		else if (*p == '*') e.op = KEO_MUL, e.f.builtin = ke_op_KEO_MUL, e.n_args = 2, *r = q + 1; // FIXME: NOT working for unary operators
		else if (*p == '/' && p[1] == '/') e.op = KEO_IDIV, e.f.builtin = ke_op_KEO_IDIV, e.n_args = 2, *r = q + 2;
		else if (*p == '/') e.op = KEO_DIV, e.f.builtin = ke_op_KEO_DIV, e.n_args = 2, *r = q + 1;
		else if (*p == '%') e.op = KEO_MOD, e.f.builtin = ke_op_KEO_MOD, e.n_args = 2, *r = q + 1;
		else if (*p == '+') {
			if (last_is_val) e.op = KEO_ADD, e.f.builtin = ke_op_KEO_ADD, e.n_args = 2;
			else e.op = KEO_POS, e.f.builtin = ke_op_KEO_POS, e.n_args = 1;
			*r = q + 1;
		} else if (*p == '-') {
			if (last_is_val) e.op = KEO_SUB, e.f.builtin = ke_op_KEO_SUB, e.n_args = 2;
			else e.op = KEO_NEG, e.f.builtin = ke_op_KEO_NEG, e.n_args = 1;
			*r = q + 1;
		} else if (*p == '=' && p[1] == '=')e.op = KEO_EQ,e.f.builtin = ke_op_KEO_EQ, e.n_args = 2, *r = q + 2;
		else if (*p == '!' && p[1] == '=') e.op = KEO_NE, e.f.builtin = ke_op_KEO_NE, e.n_args = 2, *r = q + 2;
		else if (*p == '>' && p[1] == '=') e.op = KEO_GE, e.f.builtin = ke_op_KEO_GE, e.n_args = 2, *r = q + 2;
		else if (*p == '<' && p[1] == '=') e.op = KEO_LE, e.f.builtin = ke_op_KEO_LE, e.n_args = 2, *r = q + 2;
		else if (*p == '>' && p[1] == '>') e.op = KEO_RSH, e.f.builtin = ke_op_KEO_RSH, e.n_args = 2, *r = q + 2;
		else if (*p == '<' && p[1] == '<') e.op = KEO_LSH, e.f.builtin = ke_op_KEO_LSH, e.n_args = 2, *r = q + 2;
		else if (*p == '>') e.op = KEO_GT, e.f.builtin = ke_op_KEO_GT, e.n_args = 2, *r = q + 1;
		else if (*p == '<') e.op = KEO_LT, e.f.builtin = ke_op_KEO_LT, e.n_args = 2, *r = q + 1;
		else if (*p == '|' && p[1] == '|') e.op = KEO_LOR, e.f.builtin = ke_op_KEO_LOR, e.n_args = 2, *r = q + 2;
		else if (*p == '&' && p[1] == '&') e.op = KEO_LAND, e.f.builtin = ke_op_KEO_LAND, e.n_args = 2, *r = q + 2;
		else if (*p == '|') e.op = KEO_BOR, e.f.builtin = ke_op_KEO_BOR, e.n_args = 2, *r = q + 1;
		else if (*p == '&') e.op = KEO_BAND, e.f.builtin = ke_op_KEO_BAND, e.n_args = 2, *r = q + 1;
		else if (*p == '^') e.op = KEO_BXOR, e.f.builtin = ke_op_KEO_BXOR, e.n_args = 2, *r = q + 1;
		else if (*p == '~') e.op = KEO_BNOT, e.f.builtin = ke_op_KEO_BNOT, e.n_args = 1, *r = q + 1;
		else if (*p == '!') e.op = KEO_LNOT, e.f.builtin = ke_op_KEO_LNOT, e.n_args = 1, *r = q + 1;
		else e.ttype = KET_NULL, *err |= KEE_UNOP;
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
			t = push_back(&op, &n_op, &m_op); // push to the operator stack
			t->op = -1, t->ttype = KET_NULL; // ->op < 0 for a left parenthsis
			++p;
		} else if (*p == ')') {
			while (n_op > 0 && op[n_op-1].op >= 0) { // move operators to the output until we see a left parenthesis
				u = push_back(&out, &n_out, &m_out);
				*u = op[--n_op];
			}
			if (n_op < 0) { // error: extra right parenthesis
				*err |= KEE_UNRP;
				break;
			} else --n_op; // pop out '('
			if (n_op > 0 && op[n_op-1].ttype == KET_FUNC) { // the top of the operator stack is a function
				u = push_back(&out, &n_out, &m_out); // move it to the output
				*u = op[--n_op];
				if (u->n_args == 1 && strcmp(u->name, "abs") == 0) u->f.builtin = ke_func1_abs;
			}
			++p;
		} else if (*p == ',') { // function arguments separator
			while (n_op > 0 && op[n_op-1].op >= 0) {
				u = push_back(&out, &n_out, &m_out);
				*u = op[--n_op];
			}
			if (n_op < 2 || op[n_op-2].ttype != KET_FUNC) { // we should at least see a function and a left parenthesis
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
	
	if (*err == 0) { // then check if the number of args is correct
		int i, n;
		for (i = n = 0; i < n_out; ++i) {
			ke1_t *e = &out[i];
			if (e->ttype == KET_VAL) ++n;
			else n -= e->n_args - 1;
		}
		if (n != 1) *err |= KEE_ARG;
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
	ke1_t *e;
	kexpr_t *ke;
	e = ke_parse_core(_s, &n, err);
	if (*err) return 0;
	ke = (kexpr_t*)calloc(1, sizeof(kexpr_t));
	ke->n = n, ke->e = e;
	return ke;
}

int ke_eval(const kexpr_t *ke, int64_t *_i, double *_r, int *int_ret)
{
	ke1_t *stack, *p, *q;
	int i, top = 0, err = 0;
	*_i = 0, *_r = 0., *int_ret = 0;
	for (i = 0; i < ke->n; ++i) {
		ke1_t *e = &ke->e[i];
		if ((e->ttype == KET_OP || e->ttype == KET_FUNC) && e->f.builtin == 0) err |= KEE_UNFUNC;
		else if (e->ttype == KET_VAL && e->name && e->assigned == 0) err |= KEE_UNVAR;
	}
	stack = (ke1_t*)malloc(ke->n * sizeof(ke1_t));
	for (i = 0; i < ke->n; ++i) {
		ke1_t *e = &ke->e[i];
		if (e->ttype == KET_OP || e->ttype == KET_FUNC) {
			if (e->n_args == 2 && e->f.builtin) {
				q = &stack[--top], p = &stack[top-1];
				if (e->user_func) {
					if (e->user_func == KEF_REAL)
						p->r = e->f.real_func2(p->r, q->r), p->i = (int64_t)(p->r + .5), p->vtype = KEV_REAL;
				} else e->f.builtin(p, q);
			} else if (e->n_args == 1 && e->f.builtin) {
				p = &stack[top-1];
				if (e->user_func) {
					if (e->user_func == KEF_REAL)
						p->r = e->f.real_func1(p->r), p->i = (int64_t)(p->r + .5), p->vtype = KEV_REAL;
				} else e->f.builtin(&stack[top-1], 0);
			} else top -= e->n_args - 1;
		} else stack[top++] = *e;
	}
	*_i = stack->i, *_r = stack->r, *int_ret = (stack->vtype == KEV_INT);
	free(stack);
	return err;
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
			e->i = x, e->r = xx, e->ttype = KEV_INT, e->assigned = 1, ++n;
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
			e->r = x, e->i = xx, e->ttype = KEV_REAL, e->assigned = 1, ++n;
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
			e->i = 0, e->r = 0., e->assigned = 1;
			e->vtype = KEV_STR;
			++n;
		}
	}
	return n;
}

int ke_set_real_func1(kexpr_t *ke, const char *name, double (*func)(double))
{
	int i, n = 0;
	for (i = 0; i < ke->n; ++i) {
		ke1_t *e = &ke->e[i];
		if (e->ttype == KET_FUNC && e->n_args == 1 && strcmp(e->name, name) == 0)
			e->f.real_func1 = func, e->user_func = KEF_REAL, ++n;
	}
	return n;
}

int ke_set_real_func2(kexpr_t *ke, const char *name, double (*func)(double, double))
{
	int i, n = 0;
	for (i = 0; i < ke->n; ++i) {
		ke1_t *e = &ke->e[i];
		if (e->ttype == KET_FUNC && e->n_args == 2 && strcmp(e->name, name) == 0)
			e->f.real_func2 = func, e->user_func = KEF_REAL, ++n;
	}
	return n;
}

int ke_set_default_func(kexpr_t *ke)
{
	int n = 0;
	n += ke_set_real_func1(ke, "exp", exp);
	n += ke_set_real_func1(ke, "exp2", exp2);
	n += ke_set_real_func1(ke, "log", log);
	n += ke_set_real_func1(ke, "log2", log2);
	n += ke_set_real_func1(ke, "log10", log10);
	n += ke_set_real_func1(ke, "sqrt", sqrt);
	n += ke_set_real_func1(ke, "sin", sin);
	n += ke_set_real_func1(ke, "cos", cos);
	n += ke_set_real_func1(ke, "tan", tan);
	n += ke_set_real_func2(ke, "pow", pow);
	return n;
}

void ke_unset(kexpr_t *ke)
{
	int i;
	for (i = 0; i < ke->n; ++i) {
		ke1_t *e = &ke->e[i];
		if (e->ttype == KET_VAL && e->name) e->assigned = 0;
	}
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
	ke_set_default_func(ke);
	if (err) {
		fprintf(stderr, "Parse error: 0x%x\n", err);
		return 1;
	}
	if (!to_print) {
		int64_t vi;
		double vr;
		int i, int_ret;
		if (argc - optind > 1) {
			for (i = optind + 1; i < argc; ++i) {
				char *p, *s = argv[i];
				for (p = s; *p && *p != '='; ++p);
				if (*p == 0) continue; // not an assignment
				*p = 0;
				ke_set_real(ke, s, strtod(p+1, &p));
			}
		}
		err |= ke_eval(ke, &vi, &vr, &int_ret);
		if (err & KEE_UNFUNC)
			fprintf(stderr, "Evaluation warning: an undefined function returns the first function argument.\n");
		if (err & KEE_UNVAR) fprintf(stderr, "Evaluation warning: unassigned variables are set to 0.\n");
		if (is_int) printf("%lld\n", (long long)vi);
		else printf("%g\n", vr);
	} else ke_print(ke);
	ke_destroy(ke);
	return 0;
}
#endif
