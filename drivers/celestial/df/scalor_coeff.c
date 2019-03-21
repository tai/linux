#include "scalor_coeff.h"

int mpa_mul(unsigned int x1, unsigned int x2)
{
    int val;

    val = (int)(((unsigned long long)x1 * (unsigned long long)x2) >> 32);

    return val;
}

int mpa_mulu(unsigned int x1, unsigned int x2, unsigned int *hi, unsigned int *lo)
{
    unsigned long long val;

    val = (unsigned long long)x1 * (unsigned long long)x2;
    *hi = (unsigned int)(val >> 32);
    *lo = (unsigned int)(val & 0xffffffff);

    return 1;
}

int mpa_mul_q30(int x1, int x2)	
{
    int val;

    x1 <<= 1;
    x2 <<= 2;
    
    val = (int)(((signed long long)x1 * (signed long long)x2) >> 32);

    return val;
}

int mpa_mul_q29(int x1, int x2)	
{
    int val;

    x1 <<= 2;
    x2 <<= 2;
    
    val = (int)(((signed long long)x1 * (signed long long)x2) >> 32);

    return val;
}

int mpa_mul_q28(int x1, int x2)	
{
    int val;

    x1 <<= 3;
    x2 <<= 3;
    
    val = (int)(((signed long long)x1 * (signed long long)x2) >> 34);

    return val;
}

int ABS(int x)
{
	if (x > 0)
		return x;
	else
		return -x;
}

unsigned int mpa_div64(unsigned int n_hi, unsigned int n_lo, unsigned int d)
{
	unsigned int q = 0;
    unsigned int r;
    unsigned int N = 32;

    r = n_hi;
    do 
	{
		N--;
        r <<= 1;
        r = r + ((n_lo >> N) & 0x1);
		if (r >= d)
        {
            r = r - d;
            q += (1 << N);
        }
	} while(N);

    /* round */
    if ((r << 1) >= d)
        q++;

	return q;
}

int Cubic(int s, unsigned int m)		// sample position of the filter              
{
	int val1, val2, val;

	s = ABS(s);
	s = mpa_mul(s, m);
	if (s <= 0x10000000){ // Q0.28 
		/* val = 1.5 * s * s * s - 2.5 * s * s + 1; */
		val1 = mpa_mul_q28(s, s);
		val2 = (val1 << 1) + (val1 >> 1);
		val1 = mpa_mul_q28(val1, s);
		val1 = (val1 >> 1) + val1; // Q2.28

		val = val1 - val2 + 0x10000000; // Q0.28
	}
	else if (s < 0x20000000){ // Q1.28
		/* val = -0.5 * s * s * s + 2.5 * s * s - 4 * s + 2; */
		val1 = mpa_mul_q29(s, s); //  Q2.28
		val2 = (val1 << 1) + (val1 >> 1);
		val1 = mpa_mul_q30(val1, s); // Q3.28

		val = -val1 + val2 - (s << 2) + 0x20000000;
	}
	else 
	{
		val = 0;
	}

	return val;
}

/* in --> Q3.28   out --> Q0.28 */
int Linear(int s)
{
    int val = 0;
    
    s = ABS(s);

    if (s <= 0x10000000)
		val = 0x10000000 - s;
	else
		val = 0;

    return val;
}

