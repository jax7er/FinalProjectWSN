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

void mrf24j40PrintAllRegisters(void) {
    // print out the values of all the registers on the MRF24J40
    uint16_t addr;
    for (addr = 0x00; addr <= 0x3F; addr++) {
        printf("%x=%x\r\n", addr, mrf24j40_read_short_ctrl_reg(addr));
    }
    for (addr = 0x200; addr <= 0x24C; addr++) {
        printf("%x=%x\r\n", addr, mrf24j40_read_long_ctrl_reg(addr));
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