/*
 Name:		IRGuidance.ino
 Created:	3/25/2023 7:08:14 PM
 Author:	Sus
*/
#include "Regulator.h"
#include "Filter.h"
#include <Servo.h>
#include "Demod.h"
#include <avr/io.h>

#define TURN_ON_MODULATION
#define TURN_ON_FILTER

#ifdef TURN_ON_MODULATION
Demodulator dm;
#endif

#ifdef TURN_ON_FILTER
Kalman k[4];
#endif

PID pidX(-125, 125);
PID pidY(-125, 125);

Servo servX;  // create servo object to control a servo
Servo servY;  // create servo object to control a servo
int anglX = 90;
int anglY = 90;

uint32_t timer;

byte ADMUXStart;

uint32_t gtimer;
uint32_t gc = 0;
// the setup function runs once when you press reset or power the board
void setup() {
    gtimer = micros();
    servX.attach(3);  // attaches the servo on pin 9 to the servo object
    servX.write(anglX);
    servY.attach(2);  // attaches the servo on pin 9 to the servo object
    servY.write(anglY);

	Serial.begin(230400);
    /*
    // setup PWM for IR Emitter
    // reset both timer/counters
    TCCR0A = 0;
    TCCR0B = 0;
    // set UNO pin 5/PD5 and pin 6/PD6 to output
    DDRD |= _BV(DDD5);

    // TCCR0A [ COM0A1 COM0A0 COM0B1 COM0B0 0 0 WGM01 WGM00 ] = 0b10110011
    TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(COM0B0) | _BV(WGM01) | _BV(WGM00);
    // TCCR0B [ FOC2A FOC2B 0 0 WGM02 CS02 CS01 CS00 ] = 0b00000011
    TCCR0B = _BV(CS02);// | _BV(CS00);
    
    OCR0B = 127;
    */
    // setup ADC for IR Sensor
    ADCSRA = 0;             // clear ADCSRA register
    ADCSRB = 0;             // clear ADCSRB register
    ADMUX |= (0 & 0x07);    // set ADC0 analog input pin
    ADMUX |= (1 << REFS0);  // set reference voltage
    ADMUX |= (1 << ADLAR);  // left align ADC value to 8 bits from ADCH register
    // sampling rate is [ADC clock] / [prescaler] / [conversion clock cycles]
    // for Arduino Uno ADC clock is 16 MHz and a conversion takes 13 clock cycles
    //ADCSRA |= (1 << ADPS2) | (1 << ADPS0);    // 32 prescaler for 38.5 KHz
    //ADCSRA |= (1 << ADPS2);                     // 16 prescaler for 76.9 KHz
    ADCSRA |= (1 << ADPS1) | (1 << ADPS0);    // 8 prescaler for 153.8 KHz

    ADCSRA |= (1 << ADATE); // enable auto trigger
    ADCSRA |= (1 << ADIE);  // enable interrupts when measurement complete
    ADCSRA |= (1 << ADEN);  // enable ADC
    ADCSRA |= (1 << ADSC);  // start ADC measurements
    ADMUXStart = ADMUX;

#ifdef TURN_ON_MODULATION
    //init demodulator
    double  a[] = {0, 0, 0, 0, 0};
    double  b[] = { 1, 0, 0, 0, 0 };
    dm.setAB(a, b);
#endif

#ifdef TURN_ON_FILTER
    for(int i = 0; i < 4; i++)
        k[i].setAngle(0);
#endif
    pidX.setPos(0);
    pidY.setPos(0);

    uint32_t st = micros() - gtimer;
    Serial.print("Setup time = ");
    Serial.println(st);
    gtimer = micros();
}

//Buffer size
const unsigned int buffL = 4;   //this value could be chosen larger or lover if desired
//here the data will be stored
//we need 2 separate buffer
//in first one we save data from ADC, and from second one we send data via Serial
//creating 1 continious array allows us to switch between first and second buffer just by changing offset
//if offset == 0, then data[offset + i] uses 1st buffer, if offset == buffL - 2nd
//therefore we can use add operator instead of if whick save us from branching and should work much faster
byte data[2 * buffL][4];
//Offset for Serial buffer
uint16_t offsetR = 0;
//Offset for ADC data buffer
uint16_t offsetW = buffL;
//Counter for ADC to populate data array
uint16_t counter = 0;
//Flag to start Serial data transfer
bool start = false;

byte muxCounter = 0;

// ADC complete ISR
ISR(ADC_vect)
{
    //Write data from ADC to data array
    //data[offsetW + counter++] = ADCH;
    //If ADC buffer is full
    data[counter + offsetW][muxCounter] = ADCH;
    ADMUX++;
    muxCounter++;
    if (muxCounter > 3) [[unlikely]]
    {
        muxCounter = 0;
        ADMUX = ADMUXStart;
        counter++;
        if ((counter + offsetW) == buffL) [[unlikely]]
        {
            //Clean counter
            counter = 0;
            //Swap offsets, so Serial now sends data from just populated buffer
            uint16_t temp = offsetW;
            //And ADC populates another buffer
            offsetW = offsetR;
            offsetR = temp;
            //Start Serial data proc
            start = true;
        }
    }
}

double res[4] = { 0, 0, 0, 0 };

void loop() {
    uint32_t st = micros() - gtimer;
    if(st > 1000000)
    {
        Serial.print("Run time = ");
        Serial.println(st);
        Serial.print("Num of cycles = ");
        Serial.println(gc);
        double stS = st / static_cast<double>(1000000);
        double cv = (double)gc / stS;
        gtimer = micros();
    }
    if (start == true)
    {
        double dt = (double)(micros() - timer) / 1000000; // Calculate delta time
        timer = micros();
        //Set start to false to not send same data over and over
        start = false;
        for (int i = 0; i < buffL; i++) 
        {
            //copy data into buffer
            byte tempBuff[4];
            memmove(tempBuff, data[i + offsetR], 4 * sizeof(byte));

            for (int j = 0; j < 4; j++)
            {
            #ifdef TURN_ON_MODULATION
                //demodulation
                res[j] = dm.process(tempBuff[j]);
            #endif
                //filtering
            #ifdef TURN_ON_FILTER
                res[j] = k[j].getAngle(res[j], 0, dt);
            #endif

                //calculate error
                double errX = res[0] + res[3] - res[2] - res[1];
                double errY = res[2] + res[3] - res[0] - res[1];

                anglX += pidX.calcReg(errX, dt);
                anglY += pidY.calcReg(errY, dt);
                
                //servX.write(anglX);
                //servY.write(anglY);
                gc++;
            }      
        }
    }
}
