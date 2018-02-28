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

void spi_write_buffer(uint8_t * buf, uint16_t count) {
    if (buf && count) {
        while (count--) {
            while (SPI1STATLbits.SPITBF);
            SPI1BUFL = *(buf++);
        }

        volatile uint8_t rx;
        while (SPI1STATLbits.SPIRBE);
        rx = SPI1BUFL;
        while (!SPI1STATLbits.SPIRBE) {
            rx = SPI1BUFL;
        }
    }
}

uint8_t spi_read_short(uint8_t addr) {
    uint8_t lsbs = ((addr << 1) & 0x7E);
    volatile uint8_t rx;
       
    while (SPI1STATLbits.SPITBF);  // wait for tx buffer to be empty
    SPI1BUFL = lsbs; // bit 7 = 0 for short, bit 0 = 0 for read
    while (SPI1STATLbits.SPITBF);  // wait for tx buffer to be empty
    SPI1BUFL = SPI1_DUMMY_DATA; // read data from radio
       
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    while (SPI1STATLbits.SPIRBE);   
    rx = SPI1BUFL;
    
    return rx;
}

void spi_write_short(uint8_t addr, uint8_t data) {
    uint8_t lsbs = (((addr << 1) & 0x7E) | 0x01);
    volatile uint8_t rx;
       
    while (SPI1STATLbits.SPITBF); // wait for tx buffer to be empty 
    SPI1BUFL = lsbs; // bit 7 = 0 for short, bit 0 = 1 for write
    while (SPI1STATLbits.SPITBF); // wait for tx buffer to be empty    
    SPI1BUFL = data; // write data to radio
    
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
}

uint8_t spi_read_long(uint16_t addr) {
    uint8_t msbs = (((addr >> 3) & 0x7F) | 0x80);
    uint8_t lsbs = (((addr << 5) & 0xE0));
    volatile uint8_t rx;
    
    // wait for tx buffer to be empty
    while (SPI1STATLbits.SPITBF);         
    SPI1BUFL = msbs; // write address MSBs to radio
    while (SPI1STATLbits.SPITBF);   
    SPI1BUFL = lsbs; // write address LSBs to radio
    while (SPI1STATLbits.SPITBF);   
    SPI1BUFL = SPI1_DUMMY_DATA; // read data from radio    
   
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    while (SPI1STATLbits.SPIRBE);   
    rx = SPI1BUFL;
    
    return rx;
}

void spi_write_long(uint16_t addr, uint8_t data) {
    uint8_t msbs = (((addr >> 3) & 0x7F) | 0x80);
    uint8_t lsbs = (((addr << 5) & 0xE0) | (1 << 4));
    volatile uint8_t rx;
    
    // wait for tx buffer to be empty
    while (SPI1STATLbits.SPITBF);          
    
    SPI1BUFL = msbs; // write address MSBs to radio
    while (SPI1STATLbits.SPITBF);       
    SPI1BUFL = lsbs; // write address LSBs to radio
    while (SPI1STATLbits.SPITBF);        
    SPI1BUFL = data; // write data to radio
    
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
}
