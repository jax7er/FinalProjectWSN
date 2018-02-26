/*
 * File:   main.c
 * Author: Jack
 *
 * Created on 18 February 2018, 10:33
 */


#include "xc.h"
#include "MRF24J40.h"
#include "config.h"
#include "mcc_generated_files/mcc.h"
#include "mcc_generated_files/uart1.h"
#include "mcc_generated_files/tmr3.h"
#include "mcc_generated_files/spi1.h"
#include <stdio.h>

void delay_ms(uint16_t ms);
void delay_us(uint16_t ms);
    
static void uart1Print(char const * const str);

int main(void) {
    SYSTEM_Initialize();
    
    uart1Print("board init done\r\n");    
    
    mrf24j40_cs_pin(1);
    mrf24j40_wake_pin(1);

    mrf24j40_hard_reset();

    mrf24j40_write_short_ctrl_reg(SOFTRST, (RSTPWR | RSTBB | RSTMAC));

    mrf24j40_delay_us(192);
    
    // print out the values of all the registers on the MRF24J40
    uint16_t i;
    uint8_t flag = 0;
    for (i = 0; i <= 0x3F; i++) {
        if (i == 0x10) {
            flag = 1;
        }
        uint8_t val = mrf24j40_read_short_ctrl_reg(i);
        printf("reg %x = %d\r\n", i, val);
    }
    for (i = 0x200; i <= 0x24C; i++) {
        uint8_t val = mrf24j40_read_long_ctrl_reg(i);
        printf("reg %x = %d\r\n", i, val);
    } 
    
    mrf24j40_initialize();
    
    uart1Print("radio init done\r\n");
    
    uart1Print("infinite loop\r\n");
    while (1) {
        delay_ms(1000);
    }
    
    return 0;
}

uint8_t spi_read(void) {
    uint8_t rxData;
    SPI1_Exchange8bitBuffer(SPI1_DUMMY_DATA, 1, &rxData);
    return rxData;
}
void spi_write(uint8_t data) {
    SPI1_Exchange8bit(data);
}

void delay_us(uint16_t us) {
    PR3 = us * 2; // each clock tick takes 0.5us
    TMR3 = 0; // reset timer value
    _T3IF = 0; // reset interrupt flag
    
    T3CONbits.TON = 1; // enable timer
    while (!_T3IF); // wait for interrupt flag to be set
    T3CONbits.TON = 0; // disable timer
    
    _T3IF = 0; // reset interrupt flag
}
        
void delay_ms(uint16_t ms) {
    while (ms--) {
        delay_us(1000);
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


