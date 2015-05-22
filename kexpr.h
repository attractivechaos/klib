#ifndef KEXPR_H
#define KEXPR_H

#include <stdint.h>

struct kexpr_s;
typedef struct kexpr_s kexpr_t;

#define KEE_UNQU    0x01 // unmatched quotation marks
#define KEE_UNLP    0x02 // unmatched left parentheses
#define KEE_UNRP    0x04 // unmatched right parentheses
#define KEE_UNOP    0x08 // unknown operators
#define KEE_FUNC    0x10 // wrong function
#define KEE_ARG     0x20

#define KEE_UNFUNC  0x40 // undefined function
#define KEE_UNVAR   0x80 // unassigned variable

#ifdef __cplusplus
extern "C" {
#endif

kexpr_t *ke_parse(const char *_s, int *err);
void ke_destroy(kexpr_t *ke);
void ke_print(const kexpr_t *ke);
int ke_set_int(kexpr_t *ke, const char *var, int64_t x);
int ke_set_real(kexpr_t *ke, const char *var, double x);
int ke_set_str(kexpr_t *ke, const char *var, const char *x);
int ke_eval(const kexpr_t *ke, int64_t *_i, double *_r, int *int_ret);

#ifdef __cplusplus
}
#endif

#endif
