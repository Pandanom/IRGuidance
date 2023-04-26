// Regulator.h

#ifndef _REGULATOR_h
#define _REGULATOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class PID {
	//coefficients
	double ki, kd, kp;
	//previous time to calculate dt	
	//unsigned long preTime;
	//previous error
	double preErr;
	//needed value
	double pos;
	//minimum and maximum output value
	double min, max;


public:
	//constructor
	PID(double min_, double max_);

	//set coefficients
	void setK(double kp_, double ki_, double kd_);
	void setKp(double kp_);
	void setKi(double ki_);
	void setKd(double kd_);
	//set needed position
	void setPos(double pos_);

	//calculate output
	double calcReg(double inp_, double dT);

};

#endif

