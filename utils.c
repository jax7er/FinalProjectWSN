/*
 * File:   utils.c
 * Author: jmm546
 *
 * Created on February 27, 2018, 12:44 PM
 */

#include "utils.h"
#include "xc.h"
#include <stdio.h>
#include "mcc_generated_files/uart1.h"
#include "mcc_generated_files/pin_manager.h"
#include "radio.h"
#include "payload.h"
#include "delay.h"

#define TOGGLE_LED_PERIOD_MS 250

void utils_uart1Print(char const * const str) {
    if (str) {
        uint16_t i = 0; 
        while (str[i] != '\0') {
            UART1_Write(u8(str[i++]));
        }    
    }
}

void utils_flashLed(uint16_t num) {
    uint32_t toggles = num * 2;
    while (toggles--) {
        LED_Toggle();
        delay_ms(TOGGLE_LED_PERIOD_MS);
    };
}

void utils_flashLedForever(void) {
    while (1) {
        utils_flashLed(0xFFFF);
    }; 
}