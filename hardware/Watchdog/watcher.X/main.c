/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#include <xc.h>         /* XC8 General Include File */

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "user.h"          /* User funct/params, such as InitApp */

/******************************************************************************/
/* User Global Variable Declaration                                           */
/******************************************************************************/
#define RESETPIN GPIO5
#define SIGNALPIN GPIO3
#define MOTORSTOP GPIO4
#define _XTAL_FREQ     500000L

uint16_t ctr = 0;


/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

void main(void)
{
    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize I/O and Peripherals for application */
    InitApp();
    uint8_t PINGED = 0x0;


    while(1)
    {
       __delay_ms(100);
        if (SIGNALPIN) {
            INTF = 0x0;
            PINGED = 0x1;
            SIGNALPIN = 0x0;
            __delay_ms(500);
        }
        
        if (!PINGED) {
            ctr ++;
            if (ctr > 700) {// 10 seconds
                RESETPIN = 0x1;
                __delay_ms(200);
                RESETPIN = 0x0;
                MOTORSTOP = 0x1;
                ctr = 0;
            } 
        }else {
            ctr = 0;
            PINGED = 0x0;
            MOTORSTOP = 0x0;
        }
    }

}

