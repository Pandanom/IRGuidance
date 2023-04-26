// Filter.h

#ifndef _FILTER_h
#define _FILTER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


#endif

#define ORDER 5

class Demodulator {
	double a[ORDER]; // a1 = a2 ... an = 0
	double b[ORDER];
    byte x[ORDER] = {};
	double y[ORDER] = {};
public:
	Demodulator() {};
	void setAB(double* a_, double* b_);
	double process(byte sample);
};