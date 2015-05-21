#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "kexpr.h"

static int ke_op[23] = {
	0,
	2<<1|1, 2<<1|1, 2<<1|1, 2<<1|1,
	3<<1, 3<<1, 3<<1,
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

static ke1_t ke_read_token(char *p, char **r, int *err) // it doesn't parse parentheses
{
	char *q = p;
	ke1_t e;
	memset(&e, 0, sizeof(ke1_t));
	if (isalpha(*p)) { // a variable or a function
		for (; *p && (*p == '_' || isalnum(*p)); ++p);
		if (*p == '(') e.ttype = KET_FUNC;
		else e.ttype = KET_VAL, e.vtype = KEV_VAR;
		e.s = strndup(q, p - q);
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
		if (*p == '*') e.op = KEO_MUL, *r = q + 1; // FIXME: NOT working for unary operators
		else if (*p == '/') e.op = KEO_DIV, *r = q + 1;
		else if (*p == '%') e.op = KEO_QUO, *r = q + 1;
		else if (*p == '+') e.op = KEO_ADD, *r = q + 1;
		else if (*p == '-') e.op = KEO_SUB, *r = q + 1;
		else if (*p == '=' && *p == '=') e.op = KEO_EQ, *r = q + 2;
		else if (*p == '!' && *p == '=') e.op = KEO_NE, *r = q + 2;
		else if (*p == '>' && *p == '=') e.op = KEO_GE, *r = q + 2;
		else if (*p == '<' && *p == '=') e.op = KEO_LE, *r = q + 2;
		else if (*p == '>') e.op = KEO_GT, *r = q + 1;
		else if (*p == '<') e.op = KEO_LT, *r = q + 1;
		else if (*p == '|' && *p == '|') e.op = KEO_LOR, *r = q + 2;
		else if (*p == '&' && *p == '&') e.op = KEO_LAND, *r = q + 2;
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

ke1_t *ke_parse(const char *_s, int *_n, int *err)
{
	char *s, *p, *q;
	int n_out, m_out, n_op, m_op;
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
		} else if (*p == ')') {
			while (n_op > 0 && op[n_op-1].op >= 0) {
				u = push_back(&out, &n_out, &m_out);
				*u = op[--n_op];
			}
			if (t < op) {
				*err |= KEE_UNRP;
				break;
			} else --n_op; // pop out '('
		} else if (*p == ',') { // FIXME: not implemented yet
		} else { // output-able token
			ke1_t v;
			v = ke_read_token(p, &p, err);
			if (*err) break;
			if (v.ttype == KET_VAL) {
				u = push_back(&out, &n_out, &m_out);
				*u = v;
			} else if (v.ttype == KET_FUNC) {
				t = push_back(&op, &n_op, &m_op);
				*t = v;
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

#ifdef KE_MAIN
#  define KE_PRINT
#endif

#ifdef KE_PRINT
#include <stdio.h>

static char *ke_opstr[] = {
	"",
	"+", "-", "~", "!",
	"*", "/", "%",
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

void ke_print(int n, const ke1_t *a)
{
	int i;
	for (i = 0; i < n; ++i) {
		const ke1_t *u = &a[i];
		if (i) putchar(' ');
		if (u->ttype == KET_VAL) {
			if (u->vtype == KEV_REAL) printf("%g", u->r);
			else if (u->vtype == KEV_INT) printf("%lld", (long long)u->i);
			else if (u->vtype == KEV_STR) printf("\"%s\"", u->s);
			else if (u->vtype == KEV_VAR) printf("%s", u->s);
		} else if (u->ttype == KET_OP) {
			printf("%s", ke_opstr[u->op]);
		} else if (u->ttype == KET_FUNC) {
			printf("%s()", u->s);
		}
	}
	putchar('\n');
}
#endif

#ifdef KE_MAIN
#include <unistd.h>

int main()
{
	int n, err;
	ke1_t *a;
	a = ke_parse("3+2*7", &n, &err);
	ke_print(n, a);
	return 0;
}
#endif
