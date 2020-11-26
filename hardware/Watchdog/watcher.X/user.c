/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#include <xc.h>         /* XC8 General Include File */


#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */

#include "user.h"

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/* <Initialize variables in user.h and insert code for user algorithms.> */

void InitApp(void)
{
    TRISIObits.TRISIO5 = 0x0;
    TRISIObits.TRISIO3 = 0x1;
    //disable analog
    ANSEL = 0x0;
    //disable comprator
    CMCON = 0x7; 

}

