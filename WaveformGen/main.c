/*
 * File:   main.c
 * Author: Garrett Smith
 *
 * Created on January 15, 2015, 11:52 PM
 *
 * This assignment uses the PIC18F4620 to generate either a sine, square,
 * triangle, or sawtooth wave. Two potentiometers control amplitude and
 * frequency. It can vary between 1V-5V and 10Hz-100Hz. SPI sends data
 * to a LTC1661 DAC which generates the wave.
 */

#include <xc.h>
#include <p18f4620.h>
#include "adc.h"
#include "switch.h"
#include "spi.h"
#include "timer.h"

#define SINE     0
#define SQUARE   1
#define TRIANGLE 2
#define SAWTOOTH 3

#define _XTAL_FREQ 4000000
#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config PBADEN = OFF

void interrupt ISR(void);
int calculateWaveValue(int waveValue, int amplitude);

int amplitude = 0;
int frequency = 0;
unsigned int volatile timerValue = 0;
int waveType = 0;
int volatile waveCount = 0;

//These are the 1V normalized wave values (204 out of 1024 w/ 10 bits)
int waveArray[4][50] = {
    { 102, 115, 128, 140, 152, 163, 173, 182, 189, 195,
      200, 203, 204, 204, 201, 198, 192, 186, 178, 168,
      158, 146, 134, 121, 109,  95,  83,  70,  58,  46,
       36,  26,  18,  12,   6,   3,   0,   0,   1,   4,
        9,  15,  22,  31,  41,  52,  64,  76,  84,  93 },
    { 204, 204, 204, 204, 204, 204, 204, 204, 204, 204,
      204, 204, 204, 204, 204, 204, 204, 204, 204, 204,
      204, 204, 204, 204, 204,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   8,  17,  25,  33,  42,  50,  58,  67,  75,
       83,  92, 100, 108, 117, 125, 133, 142, 150, 158,
      167, 175, 183, 192, 200, 200, 192, 183, 175, 167,
      158, 150, 142, 133, 125, 117, 108, 100,  92,  83,
       75,  67,  58,  50,  42,  33,  25,  17,   8,   0 },
    {   0,   4,   8,  12,  17,  21,  25,  29,  33,  37,
       42,  46,  50,  54,  58,  62,  67,  71,  75,  79,
       83,  87,  92,  96, 100, 104, 108, 112, 117, 121,
      125, 129, 133, 137, 142, 146, 150, 154, 158, 162,
      167, 171, 175, 179, 183, 187, 192, 196, 199,   0 }
};

int calculatedArray[50];

void main(void) {
    //set internal oscillator freq to 4MHz
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF0 = 0;
    OSCCONbits.SCS1 = 1;

    configADC();
    configSwitch();
    configSPI();

    INTCONbits.GIE = 1; //enable global interrupts

    __delay_ms(50);
    __delay_ms(50);

    while(1){
        //get wavetype
        int prevWavetype = waveType;
        waveType = getWavetype();
        //get amplitude, recalc amplitude array if wavetype changes
        //amplitude array is calculated here because interrupt timer overflows
        //going through the calculation at high freq otherwise
        int prevAmp = amplitude;
        amplitude = getAmplitude(amplitude);
        if(amplitude != prevAmp || waveType != prevWavetype){
            //add waves amplitude values to array from normalized array
            for(int i = 0; i < 50; i++){
                int volatile num = waveArray[waveType][i];
                calculatedArray[i] = calculateWaveValue(num, amplitude);
            }
        }
        //get frequency, recalculate if a change
        int prevFreq = frequency;
        frequency = getFrequency(frequency);
        if(frequency != prevFreq){
            timerValue = configTimer(frequency);
        }
    }
    return;
}

//ISR is called for the rate data values should be output
void interrupt ISR(void) {

    //timer0 IF
    if(INTCONbits.TMR0IF == 1) {
        INTCONbits.TMR0IE = 0; //disable timer0 int
        //first read TMR0L to get new value in TMR0H (from datasheet)
        int volatile tempTMR0L = TMR0L;
        TMR0H = TMR0H + (timerValue >> 8);
        TMR0L = tempTMR0L + timerValue;
        //Here we output via SPI
        int volatile sendValue = calculatedArray[waveCount];
        int volatile SPIData = ((0b1001 << 12) & 0xF000) | ((sendValue << 2) & 0x0FFF);
        transmitData(SPIData);
        if(waveCount == 49)
            waveCount = 0;
        else
            waveCount++;
    }
    INTCONbits.TMR0IF = 0; //clr flag
    INTCONbits.TMR0IE = 1; //enable timer0 int
}

int calculateWaveValue(int waveValue, int amplitude){
    //5 to 1 = 4/1023 = 0.00390068 V/bit
    //1 volt = 204 out of 1024 bits so we calculate wave amp below (1V +1V*amp)
    int volatile num = (int)((waveValue+1) + (waveValue+1)*0.00390086*amplitude);
    return num;
}