#ifndef KEIGEN_H
#define KEIGEN_H

#define KE_EXCESS_ITER (-1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compute eigenvalues/vectors for a dense symmetric matrix
 *
 * @param n       dimension
 * @param a       input matrix and eigenvalues on return ([n*n]; in & out)
 * @param v       eigenvalues ([n]; out)
 * @param cal_ev  compute eigenvectos or not (faster without vectors)
 * @param eps     precision (<=0 for default)
 * @param max_itr max iteration (<=0 for detaul)
 *
 * @return 0 on success; KE_EXCESS_ITER if too many iterations
 */
int ke_eigen_sd(int n, double *a, double *v, int cal_ev, double eps, int max_iter);

/**
 * Transform a real symmetric matrix to a tridiagonal matrix
 *
 * @param n       dimension
 * @param q       input matrix and transformation matrix ([n*n]; in & out)
 * @param b       diagonal ([n]; out)
 * @param c       subdiagonal ([n]; out)
 */
void ke_core_strq(int n, double *q, double *b, double *c);

/**
 * Compute eigenvalues and eigenvectors for a tridiagonal matrix
 *
 * @param n       dimension
 * @param b       diagonal and eigenvalues on return ([n]; in & out)
 * @param c       subdiagonal ([n]; in)
 * @param q       transformation matrix and eigenvectors on return ([n*n]; in & out)
 * @param cal_ev  compute eigenvectors or not (faster without vectors)
 * @param eps     precision
 * @param l       max iterations
 *
 * @return 0 on success; KE_EXCESS_ITER if too many iterations
 */
int ke_core_sstq(int n, double *b, double *c, double *q, int cal_ev, double eps, int l);

#ifdef __cplusplus
}
#endif

#endif
