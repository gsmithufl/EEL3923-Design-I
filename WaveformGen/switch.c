#include <xc.h>
#include <p18f4620.h>
#include "switch.h"

#define SINE     0
#define SQUARE   1
#define TRIANGLE 2
#define SAWTOOTH 3

/*
 * Two switches select between the four waveform types
 */
void configSwitch(){
    TRISAbits.RA2 = 1; //set to input
    TRISAbits.RA3 = 1;
}

int getWavetype(){
    static bit sw1;
    static bit sw2;
    sw1 = PORTAbits.RA2;
    sw2 = PORTAbits.RA3;
    if(!sw2 && !sw1){
        return SINE;
    }
    else if(!sw2 && sw1){
        return SQUARE;
    }
    else if(sw2 && !sw1){
        return TRIANGLE;
    }
    else{
        return SAWTOOTH;
    }
}

