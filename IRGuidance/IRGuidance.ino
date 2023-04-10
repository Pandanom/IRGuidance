/*
 Name:		IRGuidance.ino
 Created:	3/25/2023 7:08:14 PM
 Author:	Sus
*/
#include "Filter.h"
#include <avr/io.h>

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(230400);
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
}

//Buffer size
const unsigned int buffL = 512;   //this value could be chosen larger or lover if desired
//here the data will be stored
//we need 2 separate buffer
//in first one we save data from ADC, and from second one we send data via Serial
//creating 1 continious array allows us to switch between first and second buffer just by changing offset
//if offset == 0, then data[offset + i] uses 1st buffer, if offset == buffL - 2nd
//therefore we can use add operator instead of if whick save us from branching and should work much faster
byte data[2 * buffL];
//Offset for Serial buffer
uint16_t offsetR = 0;
//Offset for ADC data buffer
uint16_t offsetW = buffL;
//Counter for ADC to populate data array
uint16_t counter = 0;
//Flag to start Serial data transfer
bool start = false;
//1/8 buffer used to send data via serial by parts
const uint16_t part = buffL / 8;

// ADC complete ISR
ISR(ADC_vect)
{
    //Write data from ADC to data array
    data[offsetW + counter++] = ADCH;
    //If ADC buffer is full
    if (counter == buffL) [[unlikely]]
    {
        //Clean counter
        counter = 0;
        //Swap offsets, so Serial now sends data from just populated buffer
        uint16_t temp = offsetW;
        //And ADC populates another buffer
        offsetW = offsetR;
        offsetR = temp;
        //Start Serial data transfer
        start = true;
    }
}

void loop() {
    if (start)
    {
        //Set start to false to not send same data over and over
        start = false;
        //Send data in parts
        for (int i = 0; i < 8; i++)
        {
            uint16_t serOffset = offsetR + part * i;
            Serial.write(data + serOffset, part);
            //If ADC filled second buffer we need to swap and send data from the start
            if (start = true)
                break;
        }
    }
}
