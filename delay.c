/*
 * File:   delay.c
 * Author: jmm546
 *
 * Created on February 27, 2018, 12:47 PM
 */

#include "xc.h"
#include "delay.h"

void delay_us(uint16_t us) {
    PR3 = us * 4; // each clock tick takes 0.25us
    TMR3 = 0; // reset timer value
    _T3IF = 0; // reset interrupt flag
    
    T3CONbits.TON = 1; // enable timer
    while (!_T3IF); // wait for interrupt flag to be set
    T3CONbits.TON = 0; // disable timer
    
    _T3IF = 0; // reset interrupt flag
}
        
void delay_ms(uint16_t ms) {
    while (ms--) {
        delay_us(999); // assume 1us for loop operations
    }
}
