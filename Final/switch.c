#include <xc.h>
#include <p18f4620.h>
#include "switch.h"

void configSwitch(){
    //set portB for switch interrupt (default anyway)
    TRISBbits.RB5 = 0; //set to output to disable OR
    TRISBbits.RB6 = 0; //set to output to disable OR
    TRISBbits.RB7 = 0; //set to output to disable OR
    TRISBbits.RB4 = 1; //bit4 input
    PORTBbits.KBI0 = 1; //mode interrupt on change

    RCONbits.IPEN = 0;

    INTCONbits.RBIF = 0;   //reset flag
    INTCONbits.RBIE = 1;   //enable portB port change interrupt
}

