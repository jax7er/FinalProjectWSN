/*
 * File:   SPI_functions.c
 * Author: jmm546
 *
 * Created on February 27, 2018, 12:43 PM
 */

#include "SPI_functions.h"
#include "mcc_generated_files/spi1.h"

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
