#ifndef AC_KMATH_H
#define AC_KMATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	/**************************
	 * Non-linear programming *
	 **************************/

	#define KMIN_RADIUS  0.5
	#define KMIN_EPS     1e-7
	#define KMIN_MAXCALL 50000

	typedef double (*kmin_f)(int, double*, void*);
	typedef double (*kmin1_f)(double, void*);

	double kmin_hj(kmin_f func, int n, double *x, void *data, double r, double eps, int max_calls); // Hooke-Jeeves'
	double kmin_brent(kmin1_f func, double a, double b, void *data, double tol, double *xmin); // Brent's 1-dimenssion

	/*********************
	 * Special functions *
	 *********************/

	double kf_lgamma(double z); // log gamma function
	double kf_erfc(double x); // complementary error function
	double kf_gammap(double s, double z); // regularized lower incomplete gamma function
	double kf_gammaq(double s, double z); // regularized upper incomplete gamma function
	double kf_betai(double a, double b, double x); // regularized incomplete beta function

#ifdef __cplusplus
}
#endif

#endif
