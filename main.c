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
    
static void uart1Print(char const * const str);
static void printAllRegisters(void);

int main(void) {
    SYSTEM_Initialize();
        
    printf("board init done\r\n");      
    
    mrf24j40_initialize();
    
    printf("radio init done\r\n");
        
    uint8_t rxMode = RX_TX_SELECT_GetValue(); // rx mode if the pin is high
        
    if (rxMode) {
        printf("RX mode\r\n");
    } else {
        printf("TX mode\r\n");
        TX_MODE_LED_SetHigh();
    }
    
    while (1) {
        delay_ms(1000);
    }
    
    return 0;
}

uint8_t spi_read(void) {
    while (SPI1STATLbits.SPITBF);   
    
    SPI1BUFL = SPI1_DUMMY_DATA;
    while (SPI1STATLbits.SPIRBE);    
    volatile uint8_t rx = SPI1BUFL;
    
    return rx;
}

void spi_write(uint8_t value) {
    while (SPI1STATLbits.SPITBF);   
    
    SPI1BUFL = value;
    while (SPI1STATLbits.SPIRBE);    
    volatile uint8_t rx = SPI1BUFL;
}

uint8_t spi_read_short(uint8_t addr) {
    // wait for tx buffer to be empty
    while (SPI1STATLbits.SPITBF);      
    
    // write address to radio
    SPI1BUFL = ((addr << 1) & 0x7E); // bit 7 = 0 for short, bit 0 = 0 for read
    while (SPI1STATLbits.SPIRBE);    
    volatile uint8_t rx = SPI1BUFL;
    
    // read data from radio
    SPI1BUFL = SPI1_DUMMY_DATA;
    while (SPI1STATLbits.SPIRBE);   
    rx = SPI1BUFL;
    
    return rx;
}

void spi_write_short(uint8_t addr, uint8_t data) {
    // wait for tx buffer to be empty
    while (SPI1STATLbits.SPITBF);      
    
    // write address to radio
    SPI1BUFL = (((addr << 1) & 0x7E) | 0x01); // bit 7 = 0 for short, bit 0 = 1 for write
    while (SPI1STATLbits.SPIRBE);    
    volatile uint8_t rx = SPI1BUFL;
    
    // write data to radio
    SPI1BUFL = data;
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
}

uint8_t spi_read_long(uint16_t addr) {
    // wait for tx buffer to be empty
    while (SPI1STATLbits.SPITBF);      
        
    // write address MSBs to radio
    SPI1BUFL = (((addr >> 3) & 0x7F) | 0x80);
    while (SPI1STATLbits.SPIRBE);    
    volatile uint8_t rx = SPI1BUFL;
    
    // write address LSBs to radio
    SPI1BUFL = (((addr << 5) & 0xE0) | (1 << 4));
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    
    // read data from radio
    SPI1BUFL = SPI1_DUMMY_DATA;
    while (SPI1STATLbits.SPIRBE);   
    rx = SPI1BUFL;
    
    return rx;
}

void spi_write_long(uint16_t addr, uint8_t data) {
    // wait for tx buffer to be empty
    while (SPI1STATLbits.SPITBF);      
    
    // write address MSBs to radio
    SPI1BUFL = (((addr >> 3) & 0x7F) | 0x80);
    while (SPI1STATLbits.SPIRBE);    
    volatile uint8_t rx = SPI1BUFL;
    
    // write address LSBs to radio
    SPI1BUFL = (((addr << 5) & 0xE0) | (1 << 4));
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
        
    // write data to radio
    SPI1BUFL = data;
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
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
        delay_us(999); // assume 1us for loop operations
    }
}

void printAllRegisters(void) {
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
