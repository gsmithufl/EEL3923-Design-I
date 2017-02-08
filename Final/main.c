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
#include <stdio.h>
#include "adc.h"
#include "switch.h"
#include "spi.h"
#include "timer.h"

#define _XTAL_FREQ 4000000
#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config PBADEN = OFF

void interrupt ISR(void);
void configLED(void);
void configLCD(void);
void checkLCDWriteReady(void);
void writeTempData(char* str1, char* str2);
void writeData(char*);
void getAndWriteData(int, int);

int chipTemp = 0;
int thermTemp = 0;
unsigned int volatile timerValue = 0;
int volatile waveCount = 0;
bit volatile buttonTrue = 0;
bit volatile buttonFlag = 0;

//These are the 1V normalized wave values (204 out of 1024 w/ 10 bits)
int waveArray[50] = {
      102, 115, 128, 140, 152, 163, 173, 182, 189, 195,
      200, 203, 204, 204, 201, 198, 192, 186, 178, 168,
      158, 146, 134, 121, 109,  95,  83,  70,  58,  46,
       36,  26,  18,  12,   6,   3,   0,   0,   1,   4,
        9,  15,  22,  31,  41,  52,  64,  76,  84,  93 
};

int waveArray2[50] = {
      10, 12, 13, 14, 15, 16, 17, 18, 19, 19, 19, 20, 20,
      20, 20, 20, 19, 19, 18, 17, 16, 15, 13, 12, 11, 10,
      8, 7, 6, 5, 4, 3, 2, 1, 1, 0, 0, 0,
      0, 0, 1, 1, 2, 3, 4, 5, 6, 8, 9, 10
};

void main(void) {
    //set internal oscillator freq to 4MHz
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF0 = 0;
    OSCCONbits.SCS1 = 1;

    configADC();
    configSwitch();
    configSPI();
    configLED();
    configLCD();

    INTCONbits.GIE = 1; //enable global interrupts

    __delay_ms(50);
    __delay_ms(50);

    while(1){
        int prevThermTemp = thermTemp;
        int prevChipTemp = chipTemp;
        chipTemp = getChipTemp(prevChipTemp);
        thermTemp = getThermistorTemp(prevThermTemp);
        if(thermTemp != prevThermTemp){
            timerValue = configTimer(thermTemp);
            getAndWriteData(thermTemp, chipTemp);
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
        int volatile sendValue = waveArray2[waveCount];
        int volatile SPIData = ((0b1001 << 12) & 0xF000) | ((sendValue << 2) & 0x0FFF);
        transmitData(SPIData);
        if(waveCount == 49) {
            waveCount = 0;
            //LED = speaker freq/100 so every other wave
            buttonFlag = !buttonFlag;
            if(buttonTrue && buttonFlag){
                LATAbits.LA2 = !LATAbits.LA2;
            }
            else if (!buttonTrue) {
                LATAbits.LA2 = 0;
            }
        }
        else
            waveCount++;

        INTCONbits.TMR0IF = 0; //clr flag
        INTCONbits.TMR0IE = 1; //enable timer0 int
    }


    if(INTCONbits.RBIF == 1) {
        __delay_ms(20); //debounce
        __delay_ms(20); //debounce
        __delay_ms(20); //debounce
        __delay_ms(20); //debounce
        __delay_ms(20); //debounce
        __delay_ms(20); //debounce
        __delay_ms(20); //debounce
        //check what logic level and change timer (high = speaker, low = LED)
        int volatile tempBit;
        tempBit = PORTBbits.RB4;
        buttonTrue = !buttonTrue;
        NOP();
        INTCONbits.RBIF = 0; //clear interrupt
    }
}

void configLED(void) {
    TRISAbits.RA2 = 0; //set to output for LED
}

void getAndWriteData(int thermNum, int chipNum) {
    //compute chip temp
    float tempChipNum = (chipNum + 19)*0.48875855;
    float tempNum2 = 25 - (75 - tempChipNum);
    long int chipTemp = (long int) tempNum2;
    char buffer1[8];
    sprintf(buffer1, "%lu", chipTemp);
    //compute term temp .1875
    float tempThermNum = ((0x7E + (thermNum-0x1F3)*0.19) + 19)*0.48875855;
    float tempThermNum2 = 25 - (75 - tempThermNum);
    long int thermTemp = (long int) tempThermNum2;
    char buffer2[8];
    sprintf(buffer2, "%lu", thermTemp);
    
    writeTempData(buffer1, buffer2);
}

void configLCD(void) {
    TRISD = 0b00000000;
    __delay_ms(50);

    //function set 8 bit
    PORTD = 0b11000011; //X,E,RS,R/W,DB7,DB6,DB5,DB4
    PORTD = 0b10000011;

    __delay_us(50);

    //function set 4 bit
    PORTD = 0b11000010;
    PORTD = 0b10000010; //E to 0 per datasheet
    PORTD = 0b11001000; //N - 2 line, F = 5x8
    PORTD = 0b10001000; //N - 2 line, F = 5x8

    __delay_us(50);

    //function set 4 bit
    PORTD = 0b11000010;
    PORTD = 0b10000010; //E to 0 per datasheet
    PORTD = 0b11001000; //E to 1, N - 2 line, F = 5x8
    PORTD = 0b10001000; //E to 0, N - 2 line, F = 5x8

    __delay_us(50);

    //display ON/OFF control
    PORTD = 0b11000000;
    PORTD = 0b10000000; //E to 0 per datasheet
    PORTD = 0b11001111; //display on, cursor on, blink on
    PORTD = 0b10001111; //display on, cursor on, blink on

    __delay_us(50);

    //display clear
    PORTD = 0b11000000;
    PORTD = 0b10000000; //E to 0 per datasheet
    PORTD = 0b11000001; //clear display
    PORTD = 0b10000001; //clear display

    __delay_ms(3);

    //entry mode set
    PORTD = 0b11000000;
    PORTD = 0b10000000; //E to 0 per datasheet
    PORTD = 0b11000110; //i,s
    PORTD = 0b10000110; //i,s

    __delay_us(50);
}

void checkLCDWriteReady(void) {
    __delay_us(81); //delay for datasheet
    static bit volatile flag;
    int volatile tempPort = TRISD;
    TRISD = 0b10001111; //set low4 to read
    do {
        __delay_us(81); //delay for datasheet
        PORTD = 0b11010000; //set e = 1, rs = 0 (instr reg) , r/w = 1 (read)
        flag = PORTDbits.RD3; //read DB7
        PORTD = 0b10010000; //set e = 0, rs = 0, r/w = 1
        PORTD = 0b11010000; //set e = 1, rs = 0, r/w = 1
        PORTD = 0b10010000; //set e = 0
    } while(flag == 1);
    flag = 1;
    TRISD = tempPort;
    __delay_ms(10);

}

void writeTempData(char* str1, char* str2) {
    //set DDRAM address to far right
    TRISD = 0b00000000;
    __delay_us(50);
    //display clear
    PORTD = 0b11000000;
    PORTD = 0b10000000; //E to 0 per datasheet
    PORTD = 0b11000001; //clear display
    PORTD = 0b10000001; //clear display
    __delay_ms(3);

    //write 8 bits to LCD
    writeData("TMP36: ");
    writeData(str1);
    writeData(" C");
    
    PORTD = 0xCC;       //X,E,RS,R/W,DB7,DB6,DB5,DB4
    PORTD = 0x8C;
    PORTD = 0xC0;
    PORTD = 0x80;
    __delay_ms(3);

    writeData("Therm: ");
    writeData(str2);
    writeData(" C");
    __delay_us(50);

}

void writeData(char* str) {
    do {
            PORTD = ((0b11100000 & 0xf0) | ((*str >> 4) & 0x0f));
            PORTD = ((0b10100000 & 0xf0) | ((*str >> 4) & 0x0f));
            PORTD = ((0b11100000 & 0xf0) | (*str & 0x0f));
            PORTD = ((0b10100000 & 0xf0) | (*str & 0x0f));
            __delay_us(50);
            str++;
    }while(*str != 0x00);
}