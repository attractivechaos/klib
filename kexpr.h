#ifndef KEXPR_H
#define KEXPR_H

#include <stdint.h>

struct kexpr_s;
typedef struct kexpr_s kexpr_t;

// Parse errors
#define KEE_UNQU    0x01 // unmatched quotation marks
#define KEE_UNLP    0x02 // unmatched left parentheses
#define KEE_UNRP    0x04 // unmatched right parentheses
#define KEE_UNOP    0x08 // unknown operators
#define KEE_FUNC    0x10 // wrong function syntax
#define KEE_ARG     0x20

// Evaluation errors
#define KEE_UNFUNC  0x40 // undefined function
#define KEE_UNVAR   0x80 // unassigned variable

#ifdef __cplusplus
extern "C" {
#endif

	// parse an expression and return errors in $err
	kexpr_t *ke_parse(const char *_s, int *err);

	// free memory allocated during parsing
	void ke_destroy(kexpr_t *ke);

	// set a variable to integer value and return the occurrence of the variable
	int ke_set_int(kexpr_t *ke, const char *var, int64_t x);

	// set a variable to real value and return the occurrence of the variable
	int ke_set_real(kexpr_t *ke, const char *var, double x);

	// set a variable to string value and return the occurrence of the variable
	int ke_set_str(kexpr_t *ke, const char *var, const char *x);

	// set a user-defined function
	int ke_set_real_func1(kexpr_t *ke, const char *name, double (*func)(double));
	int ke_set_real_func2(kexpr_t *ke, const char *name, double (*func)(double, double));

	// set default math functions
	int ke_set_default_func(kexpr_t *ke);

	// mark all variable as unset
	void ke_unset(kexpr_t *e);

	// evaluate expression; return error code; final value is returned via pointers
	int ke_eval(const kexpr_t *ke, int64_t *_i, double *_r, int *int_ret);

	// print the expression in Reverse Polish notation (RPN)
	void ke_print(const kexpr_t *ke);

#ifdef __cplusplus
}
#endif

#endif
