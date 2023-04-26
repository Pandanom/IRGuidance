// 
// 
// 

#include "Demod.h"


void Demodulator::setAB(double* a_, double* b_)
{
    memmove(a, a_, ORDER * sizeof(byte));
    memmove(b, b_, ORDER * sizeof(byte));
}

double Demodulator::process(byte sample)
{
    memmove(&x[1], &x[0], (ORDER - 1) * sizeof(byte));
    x[0] = sample;

    double yn;
    for (byte i = 0; i < ORDER; i++) {
        yn += b[i] * x[i] - a[i] * y[i];
    }

    memmove(&y[1], &y[0], (ORDER - 1) * sizeof(double));
    y[0] = yn;
    return(yn);
}