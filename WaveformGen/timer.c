#include <xc.h>
#include <p18f4620.h>
#include "timer.h"

/*
 * The timer is used to change the frequency of data value transmission
 * via SPI. The ADC recieves a voltage from a potentiometer which is converted
 * to a frequency between 10Hz and 100Hz via the timer.
 */
int configTimer(int frequency){
    T0CONbits.TMR0ON = 0; //turn off timer
    INTCONbits.TMR0IE = 0; //disable timer0 int

    T0CONbits.T08BIT = 0; //16 bit timer
    T0CONbits.T0CS = 0;
    T0CONbits.PSA = 1; //no prescaler
    /*
     * 800 = # cycles for 1/5000, 8000 for 1/500
     * 8000 - 800 = 7200 -> 7200 / 1024 = 7.03125 cycles/bit
     * 2000 - 200 = 1800 -> 1.7578125
     * 65536 - 8000 = 57536 starting point for 10Hz*50 points
     * --The timer0 clk appears to be 1/4*Fosc so we divide 8000 and 800 by 4--
    */
    unsigned int timerValue = (unsigned int)((float)63535 + 1.775*(float)frequency);
    TMR0H = timerValue >> 8;
    TMR0L = timerValue;
    INTCONbits.TMR0IE = 1; //enable timer0 int
    INTCONbits.TMR0IF = 0; //clr flag
    T0CONbits.TMR0ON = 1; //turn on timer
    return timerValue;
}
