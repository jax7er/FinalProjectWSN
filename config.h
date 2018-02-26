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
/*
 * Copyright (C) 2014, Michele Balistreri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef CONFIG_H
#define	CONFIG_H

// Includes
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "MRF24J40.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/ext_int.h"

// Definitions
#define LITTLE_ENDIAN

#define EUI_64_ADDR 0
#define PAN_ID_ADDR 8
#define SHORT_ADDRESS_ADDR 10
#define CHANNEL_ADDR 12
#define MASTER_KEY 16
#define RX_AES_KEY 32
#define TX_AES_KEY 48
#define RX_FRAME_COUNTER 64
#define TX_FRAME_COUNTER 68

// MRF24J40 HAL Configuration
#define mrf24j40_set_ie(v) (IEC1bits.INT1IE = v)
#define mrf24j40_get_ie() (IEC1bits.INT1IE)

#define mrf24j40_wake_pin(v) (_LATB12 = v)
#define mrf24j40_reset_pin(v) (_LATB11 = v)
#define mrf24j40_cs_pin(v) (_LATB10 = v)

#define mrf24j40_spi_read() spi_exchange(0)
#define mrf24j40_spi_write(val) spi_exchange(val)
#define mrf24j40_delay_us(v) delay_us(v)
#define mrf24j40_delay_ms(v) delay_ms(v)

// Function prototypes
uint8_t spi_exchange(uint8_t data);
void delay_us(uint16_t us);
void delay_ms(uint16_t ms);

#endif	/* CONFIG_H */
