/*
 * File:   delay.c
 * Author: jmm546
 *
 * Created on February 27, 2018, 12:47 PM
 */

#include "xc.h"
#include "delay.h"
#include "radio.h"
#include "utils.h"

#define timer45CurrentValue ((u32(TMR5HLD) << 16) | u32(TMR4))
#define timer45ResetValue ((u32(PR5) << 16) | u32(PR4))
#define timer45ElapsedCounts (timer45StartValue - timer45CurrentValue)

uint32_t timer45StartValue = 0;

void delay_us(uint16_t us) {
    PR3 = us * 4; // each clock tick takes 0.25us
    TMR3 = 0; // reset timer 3 value
    _T3IF = 0; // reset interrupt flag
    
    T3CONbits.TON = 1; // enable timer 3
    while (!_T3IF); // wait for interrupt flag to be set 
    T3CONbits.TON = 0; // disable timer 3
    
    _T3IF = 0; // reset interrupt flag
}
        
void delay_ms(uint16_t ms) {
    while (ms--) {
        delay_us(999); // assume 1us for loop operations
    }
}

void timer_start(void) {
    timer45StartValue = timer45CurrentValue; // set start time to current timer value
    T4CONbits.TON = 1; // enable timer 4
}

void timer_restart(void) {    
    TMR4 = 0; // reset timer 4 value    
    TMR5HLD = 0; // reset timer 5 value    
    timer45StartValue = timer45ResetValue; // set start time to reset value
    T4CONbits.TON = 1; // enable timer 4
}

float timer_getTime_us(void) {
    return 64.0 * f(timer45ElapsedCounts); // 64us per counter increment
}

void timer_stop(void) {
    T4CONbits.TON = 0; // disable timer 4
}
