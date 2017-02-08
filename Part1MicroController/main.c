/*
 * File:   main.c
 * Author: Garrett Smith
 *
 * Created on January 12, 2015, 4:52 PM
 *
 * The purpose of this assignment is either blink an LED @ 2Hz or produce a
 * square wave 2kHz output based on a switch input.
 *
 * This is interrupt driven, when the switch triggers the interrupt it then
 * checks what the value on the pin is and sets the ports appropriately
 */


#include <xc.h>
#include <p18f4620.h>
#define _XTAL_FREQ 4000000
#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config PBADEN = OFF

#define LED_FREQ_OFFSET 15731
#define SPEAKER_FREQ_OFFSET 12

void setTimerLED();
void setTimerSpeaker();

bit timerFlag = 0;
bit RB4Bit = 0;
unsigned int counter = 0;

void main(void) {
    //before endless loop we need to check switch value and set timer based
    //on that

    //set internal oscillator freq
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF0 = 0;
    OSCCONbits.SCS1 = 1;

    //set the port I/O
    TRISAbits.RA0 = 0; //speaker
    TRISAbits.RA1 = 0; ///LED
    LATAbits.LA0 = 0;
    LATAbits.LA1 = 0;
    //set portB for switch interrupt (default anyway)
    TRISBbits.RB5 = 0; //set to output to disable OR
    TRISBbits.RB6 = 0; //set to output to disable OR
    TRISBbits.RB7 = 0; //set to output to disable OR
    TRISBbits.RB4 = 1; //bit4 input
    PORTBbits.KBI0 = 1; //mode interrupt on change

    RCONbits.IPEN = 0;

    INTCONbits.RBIF = 0;   //reset flag
    INTCONbits.RBIE = 1;   //enable portB port change interrupt

    //INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;    //enable overall interrupts

    //if high voltage set speaker timer
    if(PORTBbits.RB4 == 1)
        setTimerSpeaker();
    else
        setTimerLED();


    
    while(1) {
        NOP();
    }
    return;
}

/*
 * If there is a logic change on the switch we change the timer to the correct
 * logic level. Else the interrupt is timer overflow and we toggle input based
 * on the specified freq. There is a global var that governs level so we dont
 * poll portB (not good to do) */
void interrupt ISR(void) {
    if(INTCONbits.RBIF == 1) {
        __delay_ms(20); //debounce
        //check what logic level and change timer (high = speaker, low = LED)
        RB4Bit = PORTBbits.RB4;
        NOP();
        if(RB4Bit == 1) {
            setTimerSpeaker();
        }
        else {
            setTimerLED();
        }
        INTCONbits.RBIF = 0; //clear interrupt
    }

    if(INTCONbits.TMR0IF == 1) {
        INTCONbits.TMR0IE = 0; //disable timer0 int
        if(timerFlag == 1) {
            //speaker
            LATAbits.LA0 = !LATAbits.LA0;
            LATAbits.LA1 = 0;
            TMR0L = TMR0L + SPEAKER_FREQ_OFFSET;
        }
        else {
            //LED
            //first read TMR0L to get new value in TMR0H (from datasheet)
            volatile int tempTMR0L = TMR0L;
            TMR0H = TMR0H + (LED_FREQ_OFFSET >> 8);
            TMR0L = tempTMR0L + LED_FREQ_OFFSET;
            counter++;
            if(counter == 5) {
                counter = 0;
                LATAbits.LA1 = !LATAbits.LA1;
                LATAbits.LA0 = 0;
            }
        }
        INTCONbits.TMR0IF = 0; //clr flag
        INTCONbits.TMR0IE = 1; //enable timer0 int
    }

    //in the timer interrupt we either toggle the LED or square wave
}

void setTimerSpeaker() {
    timerFlag = 1;
    T0CONbits.TMR0ON = 0; //turn off timer
    INTCONbits.TMR0IE = 0; //disable timer0 int

    T0CONbits.T08BIT = 1; //8 bit timer
    T0CONbits.T0CS = 0;
    T0CONbits.PSA = 1; //no prescaler

    //set timer reg so 
    TMR0H = 0;
    TMR0L = SPEAKER_FREQ_OFFSET;

    INTCONbits.TMR0IE = 1; //enable timer0 int
    INTCONbits.TMR0IF = 0; //clr flag
    T0CONbits.TMR0ON = 1; //turn on timer
}

void setTimerLED() {
    timerFlag = 0;
    T0CONbits.TMR0ON = 0; //turn off timer
    INTCONbits.TMR0IE = 0; //disable timer0 int

    T0CONbits.T08BIT = 0; //16 bit timer
    T0CONbits.T0CS = 0;
    T0CONbits.PSA = 1; //no prescaler

    //set timer reg so it starts at 15535
    TMR0H = (LED_FREQ_OFFSET >> 8);
    TMR0L = LED_FREQ_OFFSET;
    __delay_ms(100);
    INTCONbits.TMR0IE = 1; //enable timer0 int
    INTCONbits.TMR0IF = 0; //clr flag
    T0CONbits.TMR0ON = 1; //turn on timer
}