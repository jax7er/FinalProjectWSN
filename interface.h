/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef INTERFACE_H
#define	INTERFACE_H

#include <stdint.h>

//#define i2c_txInProgress()  (I2C2STATbits.TRSTAT == 1)
//#define i2c_rxComplete()    (I2C2STATbits.RBF == 1)

#define i2c_ackStat         (I2C2STATbits.ACKSTAT)
#define i2c_ackDt           (I2C2CONLbits.ACKDT)
#define i2c_ackEn           (I2C2CONLbits.ACKEN)
#define i2c_rcEn            (I2C2CONLbits.RCEN) // automatically cleared by hardware at the end of an 8-bit receive data byte
#define i2c_pEn             (I2C2CONLbits.PEN)
#define i2c_rsEn            (I2C2CONLbits.RSEN)
#define i2c_sEn             (I2C2CONLbits.SEN)
#define i2c_mif             (IFS3bits.MI2C2IF)
#define i2c_rcv             (I2C2RCV)
#define i2c_trn             (I2C2TRN)

void i2c_sendAck(void);
void i2c_sendNack(void);
void i2c_stop(void);
void i2c_repeatedStart(void);
void i2c_start(void);
void i2c_tx(uint8_t x);
uint8_t i2c_rx(void);
uint8_t i2c_gotAck(void);
uint8_t i2c_gotNack(void);
uint8_t i2c_addrWrite(uint8_t a);
uint8_t i2c_addrRead(uint8_t a);

uint8_t spi_read(void);
void spi_write(uint8_t value);
uint8_t spi_read_short(uint8_t addr);
void spi_write_short(uint8_t addr, uint8_t data);
uint8_t spi_read_long(uint16_t addr);
void spi_write_long(uint16_t addr, uint8_t data);

#endif	/* INTERFACE_H */

