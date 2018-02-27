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
    uint16_t i;
    for (i = 0; i <= 0x3F; i++) {
        uint8_t val = mrf24j40_read_short_ctrl_reg(i);
        printf("0x%x = 0x%x\r\n", i, val);
    }
    for (i = 0x200; i <= 0x24C; i++) {
        uint8_t val = mrf24j40_read_long_ctrl_reg(i);
        printf("0x%x = 0x%x\r\n", i, val);
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