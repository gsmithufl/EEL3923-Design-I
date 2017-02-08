#include <xc.h>
#include <p18f4620.h>
#include "spi.h"

/*
 * SPI sends bit values to the LTC1661 at the rate specified by the timer.
 */
void configSPI(){
    SSPSTATbits.SMP = 1; //input data sampled at end of data output time
    SSPSTATbits.CKE = 1; //transmit on active to idle
    SSPCON1bits.SSPEN = 1; //enables serial port pins (sck, sdo, sdi, /ss)
    TRISCbits.RC5 = 0; // data out
    TRISCbits.RC3 = 0; // clk out
    TRISAbits.RA5 = 0; //use ss as output for select
    LATAbits.LA5 = 1;
    SSPCON1bits.CKP = 0; //idle clk low level
    SSPCON1 |= 0b0000; // master and clk = Fosc/4
}

/*
 * Transmit data via SPI
 */
void transmitData(int volatile data){
    static bit flag;
    LATAbits.LA5 = 0;
    unsigned char temp = SSPBUF; //read to
    SSPBUF = data >> 8; //high 8
    do{
        flag = SSPSTATbits.BF;
    }while(flag);
    temp = SSPBUF;
    SSPBUF = data; //low 8
    do{
        flag = SSPSTATbits.BF;
    }while(flag);
    LATAbits.LA5 = 1;
}