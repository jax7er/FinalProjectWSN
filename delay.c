/*
 * File:   delay.c
 * Author: jmm546
 *
 * Created on February 27, 2018, 12:47 PM
 * 
 * Functions to produce delays and time events.
 * Delay functions use the 16-bit timer 3 and timing functions use the 32-bit
 * combined timers 4 and 5.
 */

#include "xc.h"
#include "delay.h"
#include "radio.h"
#include "utils.h"

uint32_t timer45StartValue = 0;

static uint32_t timer45GetCurrentValue(void) { 
    uint32_t value = TMR4; // read TMR4 first to latch TMR5 value into TMR5HLD
    value += (u32(TMR5HLD) << 16);
    return value; 
}

static uint32_t timer45GetElapsedCounts(void) { 
    return timer45GetCurrentValue() - timer45StartValue; 
}

/**
 * Blocks for a given number of microseconds
 * @param us the number of microseconds to block for
 */
void delay_us(uint16_t us) {
    PR3 = us * 4; // each clock tick takes 0.25us
    TMR3 = 0; // reset timer 3 value
    _T3IF = 0; // reset interrupt flag
    
    T3CONbits.TON = 1; // enable timer 3
    while (!_T3IF); // wait for interrupt flag to be set 
    T3CONbits.TON = 0; // disable timer 3
    
    _T3IF = 0; // reset interrupt flag
}

/**
 * Blocks for a given number of milliseconds
 * @param ms the number of milliseconds to block for
 */
void delay_ms(uint16_t ms) {
    while (ms--) {
        delay_us(999); // assume 1us for loop operations
    }
}

/**
 * Resets the timer 4 and 5 registers to make the timer ready to start timing again
 */
void timer_reset(void) {    
    TMR4 = 0; // reset timer 4 value    
    TMR5HLD = 0; // reset timer 5 value 
}

/**
 * Stores the current value of the timer and enables the increment again
 */
void timer_start(void) {
    timer45StartValue = timer45GetCurrentValue(); // set start time to current timer value
    T4CONbits.TON = 1; // enable timer 4
}

/**
 * Begins timing a new event
 */
void timer_restart(void) {    
    timer_reset();  
    timer_start();
}

/**
 * Gets the number of elapsed microseconds since timer_start() was called
 * @return the number of microseconds since timer_start() was called
 */
float timer_getTime_us(void) {
    return 64.0 * f(timer45GetElapsedCounts()); // 64us per counter increment
}

uint32_t timer_getTime_ms(void) {
    return (timer45GetElapsedCounts() / u32(1000)) * u32(64);
}

/**
 * Disables the timer
 */
void timer_stop(void) {
    T4CONbits.TON = 0; // disable timer 4
}
