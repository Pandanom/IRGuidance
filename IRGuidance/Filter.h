// Filter.h

#ifndef _FILTER_h
#define _FILTER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


#endif

class Filter {
public:
	static const byte order = 5;
	double a[order]; // a1 = a2 ... an = 0
	double b[order];
    byte x[order] = {};
    byte y[order] = {};

    byte process(byte sample)
    {
        memmove(&x[1], &x[0], (order - 1) * sizeof(byte));
        x[0] = sample;

        double yn;
        for (byte i = 0; i < order; i++) {
            yn += b[i] * x[i] - a[i] * y[i];
        }

        memmove(&y[1], &y[0], (order - 1) * sizeof(byte));
        y[0] = yn;
        return(yn);
    }
};