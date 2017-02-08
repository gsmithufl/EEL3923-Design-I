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
#include <stdio.h>
#define _XTAL_FREQ 8000000
#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF

void configADC(void);
int getADCValue();
void configLCD(void);
void checkLCDWriteReady(void);
void writeData(char* str);
void getAndWriteData();

void main(void) {
    //before endless loop we need to check switch value and set timer based
    //on that

    //set internal oscillator freq
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF0 = 1;//0 for 4mhz
    OSCCONbits.SCS1 = 1;

    configLCD();
    //writeData("Out of Range");
    //int volatile i = 5;
    //i++;
    configADC();
    while(1) {
    getAndWriteData();
    __delay_ms(10);
    __delay_ms(10);
    __delay_ms(10);
    __delay_ms(10);
    __delay_ms(10);
    }

    
    return;
}

void getAndWriteData() {
    __delay_us(100);
    int num = getADCValue();
    NOP();
    //float voltage = (num-2)*.004132265 + .24; //0.004882812; // .00501002;// 0x3E8 max, 0x002 min -> 0x3E6 total
    if(num > 0x3A6) {
        writeData("Out of Range");
    }
    if(num <= 0x3A6 && num > 0x05D) {
        float voltage = (num-2)*0.004882812 + 0.005;
        float v2 = 5 - voltage;
        float fResist = voltage*98200/v2;
        long int resistance = (long int) fResist;
        char buffer[8];
        sprintf(buffer, "%lu", resistance);
        if(resistance > 1000000){
            writeData("Out of Range");
        }
        else {
            writeData(buffer);
        }
    }
    else if(num <= 0x05D) {
        float voltage = (num-2)*0.004882812 + 0.0115;
        float v2 = 5 - voltage;
        float fResist = voltage*98200/v2;
        long int resistance = (long int) fResist;
        char buffer[8];
        sprintf(buffer, "%lu", resistance);
        if(resistance < 1000) {
            writeData("Out of Range");
        }
        else {
            writeData(buffer);
        }
    }
}

void configADC(void) {
    TRISAbits.RA0 = 1; //set to input
    ADCON1 = 0b00001110;
    ADCON0 = 0b00000000; //select RA0 for conversion, disabled ADC
    ADCON2 = 0b10001110;
}

int getADCValue() {
    unsigned char test;

    ADCON0bits.ADON = 1;
    ADCON0bits.GO = 1;
    __delay_us(100);
    do {
        test = ADCON0 & 0x02;
    } while(test);
    ADCON0bits.ADON = 0;
    return ((ADRESH << 8 & 0xFF00) | (ADRESL & 0xFF));
//
//    ADCON0 = 0x00;          // Clear the register
//    ADCON0 = (0 << 2);      // Shift in the Analog channel to check
//    ADCON0 |= 0x01;         // Turn on the ADC
//    ADCON0 |= 0x02;         // Start the ADC read
//    while(ADCON0 & 0x02);   // Wait until the conversion  is complete
//    ADCON0 &= ~0x01;         // Turn off the ADC
//    return (ADRESH << 8) | ADRESL;
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
    PORTD = 0b11000000; //N - 1 line, F = 5x8
    PORTD = 0b10000000; //N - 1 line, F = 5x8

    __delay_us(50);

    //function set 4 bit
    PORTD = 0b11000010;
    PORTD = 0b10000010; //E to 0 per datasheet
    PORTD = 0b11000000; //E to 1, N - 1 line, F = 5x8
    PORTD = 0b10000000; //E to 0, N - 1 line, F = 5x8

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

void writeData(char* str) {
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

    if(str == "Out of Range") {
        do {
            PORTD = ((0b11100000 & 0xf0) | ((*str >> 4) & 0x0f));
            PORTD = ((0b10100000 & 0xf0) | ((*str >> 4) & 0x0f));
            PORTD = ((0b11100000 & 0xf0) | (*str & 0x0f));
            PORTD = ((0b10100000 & 0xf0) | (*str & 0x0f));
            __delay_us(50);
            str++;
        }while(*str != 0x00);
    } else {
        do {
            PORTD = ((0b11100000 & 0xf0) | ((*str >> 4) & 0x0f));
            PORTD = ((0b10100000 & 0xf0) | ((*str >> 4) & 0x0f));
            PORTD = ((0b11100000 & 0xf0) | (*str & 0x0f));
            PORTD = ((0b10100000 & 0xf0) | (*str & 0x0f));
            __delay_us(50);
            str++;
        }while(*str != 0x00);
        
        PORTD = 0b11100010; //space
        PORTD = 0b10100010;
        PORTD = 0b11100000;
        PORTD = 0b10100000;
        __delay_us(50);
        PORTD = 0b11100110; //o
        PORTD = 0b10101111;
        PORTD = 0b11101111;
        PORTD = 0b10101111;
        __delay_us(50);
        PORTD = 0b11100110; //h
        PORTD = 0b10100110;
        PORTD = 0b11101000;
        PORTD = 0b10101000;
        __delay_us(50);
        PORTD = 0b11100110; //m
        PORTD = 0b10100110;
        PORTD = 0b11101101;
        PORTD = 0b10101101;
        __delay_us(50);
        PORTD = 0b11100111; //s
        PORTD = 0b10100111;
        PORTD = 0b11100011;
        PORTD = 0b10100011;
        __delay_us(50);
    }
    //sprintf(buffer, "%lu", num);
//    unsigned char volatile test = 0;
//    for(int i = 0; i < 7; i++) {
//        PORTD = (0b1110 << 4) | 0b0011;
//        PORTD = (0b1010 << 4) | 0b0011;
//        unsigned char volatile test2 = buffer[6-i];
//        test = ((0b11100000 & 0xf0) | (test2 & 0x0f));
//        PORTD = test;
//        test = ((0b10100000 & 0xf0) | (test2 & 0x0f));
//        PORTD = test;
//        __delay_us(50);
//    }

    
    

    //above: we need to find the null thingy and calculate how to do
    //the for loop from that because sprintf is left aligned

    __delay_us(50);
}