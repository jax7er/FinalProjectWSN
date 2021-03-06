/*
 * File:   interface.c
 * Author: jmm546
 *
 * Created on February 27, 2018, 12:43 PM
 * 
 * Functions for sending and receiving data using I2C and SPI.
 */

#include "interface.h"
#include "mcc_generated_files/spi1.h"
#include "utils.h"

// I2C functions

/**
 * Sends an acknowledgement to a slave
 */
void i2c_sendAck(void) {
    i2c_mif = 0; 
    i2c_ackDt = 0; 
    i2c_ackEn = 1; 
    while (!i2c_mif);
}

/**
 * Send a not acknowledgement to a slave
 */
void i2c_sendNack(void) {
    i2c_mif = 0;
    i2c_ackDt = 1;
    i2c_ackEn = 1;
    while (!i2c_mif);
}

/**
 * Asserts the stop condition
 */
void i2c_stop(void) {
    i2c_mif = 0; 
    i2c_pEn = 1; 
    while (!i2c_mif);
}

/**
 * Asserts the repeated start condition
 */
void i2c_repeatedStart(void) {
    i2c_mif = 0; 
    i2c_rsEn = 1; 
    while (!i2c_mif);
}

/**
 * Asserts the start condition
 */
void i2c_start(void) {
    i2c_mif = 0; 
    i2c_sEn = 1; 
    while (!i2c_mif);
}

/**
 * Transmits a byte to a slave
 * @param x the byte to transmit
 */
void i2c_tx(uint8_t x) {
    i2c_mif = 0; 
    i2c_trn = x;    
    while (!i2c_mif);
}

/**
 * Receives a byte from a slave
 * @return the received byte
 */
uint8_t i2c_rx(void) {
    i2c_mif = 0;
    i2c_rcEn = 1; 
    while (!i2c_mif);
    return i2c_rcv;
}

/**
 * Checks whether an acknowledgement was received from a slave
 * @return boolean value, true if ACK, false if NACK
 */
uint8_t i2c_gotAck(void) {
    return i2c_ackStat == 0;
}

/**
 * Checks whether a not acknowledgement was received from a slave
 * @return boolean value, true if NACK, false if ACK
 */
uint8_t i2c_gotNack(void) {
    return i2c_ackStat == 1;
}

/**
 * Formats a 7-bit I2C address with the read/write bit set to 0
 * @param a the 7-bit address
 * @return the formatted control byte set for writing
 */
uint8_t i2c_addrWrite(uint8_t a) {
    return (a << 1) & 0xFE;
}

/**
 * Formats a 7-bit I2C address with the read/write bit set to 1
 * @param a the 7-bit address
 * @return the formatted control byte set for reading
 */
uint8_t i2c_addrRead(uint8_t a) {
    return (a << 1) | 0x01;
}

// SPI functions

uint8_t spi_read(void) {
    volatile uint8_t rx;
    
    while (SPI1STATLbits.SPITBF);       
    SPI1BUFL = 0x00;    
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    
    return rx;
}

void spi_write(uint8_t value) {
    volatile uint8_t rx;
    
    while (SPI1STATLbits.SPITBF);       
    SPI1BUFL = value;    
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
}

//void spi_write_buffer(uint8_t * buf, uint16_t count) {
//    if (buf && count) {
//        while (count--) {
//            while (SPI1STATLbits.SPITBF);
//            SPI1BUFL = *(buf++);
//        }
//
//        volatile uint8_t rx;
//        while (SPI1STATLbits.SPIRBE);
//        rx = SPI1BUFL;
//        while (!SPI1STATLbits.SPIRBE) {
//            rx = SPI1BUFL;
//        }
//    }
//}

uint8_t spi_read_short(uint8_t addr) {
    uint8_t lsbs = ((addr << 1) & 0x7E);
    volatile uint8_t rx;
       
    while (SPI1STATLbits.SPITBF);  // wait for tx buffer to be empty
    SPI1BUFL = lsbs; // bit 7 = 0 for short, bit 0 = 0 for read
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    
    while (SPI1STATLbits.SPITBF);  // wait for tx buffer to be empty
    SPI1BUFL = 0x00; // read data from radio       
    while (SPI1STATLbits.SPIRBE);   
    rx = SPI1BUFL;
    
    return rx;
}

void spi_write_short(uint8_t addr, uint8_t data) {
    uint8_t lsbs = (((addr << 1) & 0x7E) | 0x01);
    volatile uint8_t rx;
       
    while (SPI1STATLbits.SPITBF); // wait for tx buffer to be empty 
    SPI1BUFL = lsbs; // bit 7 = 0 for short, bit 0 = 1 for write
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    
    while (SPI1STATLbits.SPITBF); // wait for tx buffer to be empty    
    SPI1BUFL = data; // write data to radio    
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
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    
    while (SPI1STATLbits.SPITBF);   
    SPI1BUFL = lsbs; // write address LSBs to radio
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    
    while (SPI1STATLbits.SPITBF);   
    SPI1BUFL = 0x00; // read data from radio       
    while (SPI1STATLbits.SPIRBE);   
    rx = SPI1BUFL;
    
    return rx;
}

void spi_write_long(uint16_t addr, uint8_t data) {
    uint8_t msbs = (((addr >> 3) & 0x7F) | 0x80);
    uint8_t lsbs = (((addr << 5) & 0xE0) | (1 << 4));
    volatile uint8_t rx;    
   
    while (SPI1STATLbits.SPITBF); // wait for tx buffer to be empty
    SPI1BUFL = msbs; // write address MSBs to radio
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    
    while (SPI1STATLbits.SPITBF);       
    SPI1BUFL = lsbs; // write address LSBs to radio
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
    
    while (SPI1STATLbits.SPITBF);        
    SPI1BUFL = data; // write data to radio    
    while (SPI1STATLbits.SPIRBE);    
    rx = SPI1BUFL;
}
