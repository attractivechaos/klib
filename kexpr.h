#ifndef KEXPR_H
#define KEXPR_H

#include <stdint.h>

#define KEO_NULL  0
#define KEO_PLUS  1
#define KEO_MINUS 2
#define KEO_BNOT  3
#define KEO_LNOT  4
#define KEO_MUL   5
#define KEO_DIV   6
#define KEO_QUO   7
#define KEO_ADD   8
#define KEO_SUB   9
#define KEO_LSH  10
#define KEO_RSH  11
#define KEO_LT   12
#define KEO_LE   13
#define KEO_GT   14
#define KEO_GE   15
#define KEO_EQ   16
#define KEO_NE   17
#define KEO_BAND 18
#define KEO_BXOR 19
#define KEO_BOR  20
#define KEO_LAND 21
#define KEO_LOR  22

#define KET_NULL  0
#define KET_VAL   1 // constant
#define KET_OP    2
#define KET_FUNC  3

#define KEV_REAL  1
#define KEV_INT   2
#define KEV_STR   3
#define KEV_VAR   4

#define KEE_UNDQ  1 // unmatched double quotation marks
#define KEE_UNLP  2 // unmatched left parentheses
#define KEE_UNRP  4 // unmatched right parentheses
#define KEE_UNTO  8 // unknown tokens

typedef struct {
	uint32_t ttype:16, vtype:16;
	int32_t op:8, n_vals:24;
	char *s;
	double r;
	int64_t i;
} ke1_t;

#endif
