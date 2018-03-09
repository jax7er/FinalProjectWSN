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
#include "MRF24J40.h"
#include "payload.h"
#include "delay.h"

#define TOGGLE_LED_PERIOD_MS 250

uint16_t readAdc(void) {
    AD1CON1bits.DONE = 0;
    AD1CON1bits.SAMP = 1; // start sampling

    while (!AD1CON1bits.DONE); // wait for conversion to complete

    return ADC1BUF0; // read value
}

void mrf24j40PrintTxFifo(uint16_t totalLength) {
    printf("TXNFIFO = \"");
    uint8_t value;
    uint16_t fifo_i;
    for (fifo_i = 0; fifo_i < 6 + totalLength; fifo_i++) {
        value = radio_read_long(fifo_i);    
        if (isValidPayloadChar(value)) {
            printf("%c", value);
        } else {
            printf("<%.2X>", value);
        }
    }
    println("\"");
}

void mrf24j40PrintAllRegisters(void) {
    // print out the values of all the registers on the MRF24J40
    uint16_t addr;
    for (addr = 0x00; addr <= 0x3F; addr++) {
        printf("%x=%x\r\n", addr, radio_read(addr));
    }
    for (addr = 0x200; addr <= 0x24C; addr++) {
        printf("%x=%x\r\n", addr, radio_read(addr));
    }
}

void uart1Print(char const * const str) {
    if (str) {
        uint16_t i = 0; 
        while (str[i] != '\0') {
            UART1_Write((uint8_t)str[i++]);
        }    
    }
}

void toggleLed(uint16_t num) {
    uint32_t toggles = num * 2;
    while (toggles--) {
        LED_Toggle();
        delay_ms(TOGGLE_LED_PERIOD_MS);
    };
}

void toggleLedForever(void) {
    while (1) {
        toggleLed(0xFFFF);
    }; 
}