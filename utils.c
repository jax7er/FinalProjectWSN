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

uint8_t checkAndClear(uint8_t volatile * flag_p) {
    if (*flag_p) {
        *flag_p = 0;
        return 1;
    } else {
        return 0;
    }
}

void mrf24j40PrintTxFifo(uint16_t totalLength) {
    printf("TXNFIFO = \"");
    uint8_t value;
    uint16_t fifo_i;
    for (fifo_i = 0; fifo_i < 6 + totalLength; fifo_i++) {
        value = radio_read_long_ctrl_reg(fifo_i);    
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
        printf("%x=%x\r\n", addr, radio_read_short_ctrl_reg(addr));
    }
    for (addr = 0x200; addr <= 0x24C; addr++) {
        printf("%x=%x\r\n", addr, radio_read_long_ctrl_reg(addr));
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

void toggleLedForever(void) {
    while (1) { // do not continue if a test fails
        LED_Toggle();
        delay_ms(250);
    }; 
}