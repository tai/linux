#ifndef __SCALOR_COEFF_H__
#define __SCALOR_COFEE_H__

#ifdef __cplusplus
extern "C" {
#endif

int mpa_mul(unsigned int x1, unsigned int x2);
int mpa_mulu(unsigned int x1, unsigned int x2, unsigned int *hi, unsigned int *lo);
int mpa_mul_q30(int x1, int x2);
int mpa_mul_q29(int x1, int x2);
int mpa_mul_q28(int x1, int x2);
int ABS(int x);
unsigned int mpa_div64(unsigned int n_hi, unsigned int n_lo, unsigned int d);
int Cubic(int s, unsigned int m);
int Linear(int s);

#ifdef __cplusplus
}
#endif

#endif
