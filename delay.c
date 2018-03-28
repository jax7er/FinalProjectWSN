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

uint32_t timer45StartValue = 0;

static uint32_t timer45GetCurrentValue(void) { return ((u32(TMR5HLD) << 16) | u32(TMR4)); }
static uint32_t timer45GetResetValue(void) { return ((u32(PR5) << 16) | u32(PR4)); }
static uint32_t timer45GetElapsedCounts(void) { return timer45GetCurrentValue() - timer45StartValue; }

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

void timer_reset(void) {    
    TMR4 = 0; // reset timer 4 value    
    TMR5HLD = 0; // reset timer 5 value 
}

void timer_start(void) {
    timer45StartValue = timer45GetCurrentValue(); // set start time to current timer value
    T4CONbits.TON = 1; // enable timer 4
}

void timer_restart(void) {    
    timer_reset();  
    timer_start();
}

float timer_getTime_us(void) {
    return 64.0 * f(timer45GetElapsedCounts()); // 64us per counter increment
}

void timer_stop(void) {
    T4CONbits.TON = 0; // disable timer 4
}
