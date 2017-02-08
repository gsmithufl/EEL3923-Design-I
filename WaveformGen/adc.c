#include <xc.h>
#include <p18f4620.h>
#include "adc.h"

/*
 * The ADC receives two potentiometer inputs for voltage and amplitude of
 * output waveform
 */
void configADC(){
    TRISAbits.RA0 = 1; //set to input for Amplitude
    TRISAbits.RA1 = 1; //set to input for Frequency
    ADCON1 = 0b00001101; //set RA0 & RA1 to analog, vref+ to Vss vref- to Vdd
    ADCON2 = 0b10001000;
}

/*
 * Get amplitude from one of the potentiometers
 * Pin RA0
 */
int getAmplitude(int prevAmplitude) {
    unsigned int notReady;
    ADCON0 = 0b00000000; //select RA0 for conversion, enable ADC
    ADCON0bits.ADON = 1;
    ADCON0bits.GO = 1;
    //__delay_us(100);
    do {
        notReady = ADCON0 & 0x02;
    } while(notReady);
    ADCON0bits.ADON = 0;
    int newAmplitude = (ADRESH << 8 & 0xFF00) | (ADRESL & 0xFF);
    //If the number is very close it may be random fluctuation so discard
    if((newAmplitude <= prevAmplitude + 10) && (newAmplitude >= prevAmplitude - 10))
        return prevAmplitude;
    else
        return newAmplitude;
}

/*
 * Get frequency from one of the potentiometers
 * Pin RA1
 */
int getFrequency(int prevFrequency) {
    unsigned int notReady;
    ADCON0 = 0b00000100; //select RA1 for conversion, enable ADC
    ADCON0bits.ADON = 1;
    ADCON0bits.GO = 1;
    do {
        notReady = ADCON0 & 0x02;
    } while(notReady);
    ADCON0bits.ADON = 0;
    int newFrequency = (ADRESH << 8 & 0xFF00) | (ADRESL & 0xFF);
    //If the number is very close it may be random fluctuation so discard
    if((newFrequency <= prevFrequency + 10) && (newFrequency >= prevFrequency - 10))
        return prevFrequency;
    else
        return newFrequency;
}