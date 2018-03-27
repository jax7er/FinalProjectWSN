/*
 * File:   radio.c
 * Author: Jack
 *
 * Created on 17 February 2018, 21:34
 */

/*
 * Copyright (C) 2014, Michele Balistreri
 *
 * Derived from code originally Copyright (C) 2011, Alex Hornung
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

#include "radio.h"
#include <xc.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "interface.h"
#include "utils.h"
#include "stdio.h"
#include "payload.h"
#include "delay.h"
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
#define radio_set_ie(v) (IEC1bits.INT1IE = v)
#define radio_get_ie() (IEC1bits.INT1IE)

#define radio_wake_pin(v) (_LATB12 = v)
#define radio_reset_pin(v) (_LATB11 = v)
#define radio_cs_pin(v) (_LATB10 = v)

radio_if_t ifs = {.event = 0, .rx = 0, .tx = 0, .wake = 0};
uint8_t rxBuffer[RXFIFO_SIZE] = {0};
uint8_t txBuffer[TXNFIFO_SIZE] = {0};
uint8_t srcAddrH = 0x13; // default address = 0x1357
uint8_t srcAddrL = 0x57;
uint8_t mhr[mhrLength] = {0};
uint32_t slpclkPeriod_ns = 0;

void radio_init(void) {
    radio_cs_pin(1);
    radio_wake_pin(1);

    radio_hard_reset();

    //radio_write(SOFTRST, 0x07); // Perform a software Reset. The bits will be automatically cleared to ?0? by hardware.

    radio_write(PACON2, 0x98); // Initialize FIFOEN = 1 and TXONTS = 0x6.
    radio_write(TXSTBL, 0x95); // Initialize RFSTBL = 0x9.

    radio_write(RFCON0, 0x03); // Initialize RFOPT = 0x03, Channel 11 (2.405GHz)
    radio_write(RFCON1, 0x01); // Initialize VCOOPT = 0x02.
    radio_write(RFCON2, 0x80); // Enable PLL (PLLEN = 1).
    radio_write(RFCON6, 0x90); // Initialize TXFIL = 1 and 20MRECVR = 1.
    radio_write(RFCON7, 0x80); // Initialize SLPCLKSEL = 0x2 (100 kHz Internal oscillator).
    radio_write(RFCON8, 0x10); // Initialize RFVCO = 1.
    radio_write(SLPCON1, 0x21); // Initialize CLKOUTEN = 1 and SLPCLKDIV = 0x01.

    radio_write(BBREG2, 0x80); // Set CCA mode to ED.
    radio_write(CCAEDTH, 0x60); // Set CCA ED threshold.
    radio_write(BBREG6, 0x40); // Set appended RSSI value to RXFIFO.

    radio_write_bits(ORDER, 0b11110000, 0xF0); // set BO = 15
    radio_clear_bit(TXMCR, 5); // set slotted = 0


    // internal 100kHz clock is selected by default
    radio_write_bits(SLPCON1, 0b00111111, 0x21); // set !CLKOUTEN = 1 and clock divisor to be 2^1 = 2
    radio_write(WAKETIMEL, 0x43); // set WAKECNT = 67 (WAKETIME > WAKECNT)
    radio_write_bits(SLPACK, 0b01111111, 0x42); // set WAKECNT = 66

    radio_write(MRF24J40_INTCON, 0b10110110); // Enable wake, RX and TX normal interrupts, active low
    // tx power set to 0dBm at reset

    radio_write(RFCTL, 0x04); // Reset RF state machine.
    delay_us(200);
    radio_write(RFCTL, 0x00);
    delay_us(200); // delay at least 192 us
    
    radio_set_bit(SLPCAL2, 4); // start sleep clock calibration            
    delay_us(1); // sleep calibration takes 800ns            
    while (!radio_read_bit(SLPCAL2, 7)); // check sleep calibration is complete
    slpclkPeriod_ns = (u32(radio_read(SLPCAL2) & 0x0F) << 16) | (u32(radio_read(SLPCAL1)) << 8) | u32(radio_read(SLPCAL0));
    slpclkPeriod_ns = (slpclkPeriod_ns * 50UL) / 16UL; // *50ns/16
//    println("Radio sleep clock calibration done, SLPCLK period = %luns", slpclkPeriod_ns);
}

void radio_hard_reset(void) {
    radio_reset_pin(0);
    delay_ms(500); // wait at least 2ms
    radio_reset_pin(1);
    delay_ms(500); // wait at least 2ms
}

void radio_getIntFlags(void) {
    ifs.event = 0;
    
    uint8_t intstat = radio_read(INTSTAT);
    
//    println("INTSTAT = 0x%.2X", intstat);
    
    ifs.rx = (intstat & RXIF) != 0;
    ifs.tx = (intstat & TXNIF) != 0;
    ifs.wake = (intstat & WAKEIF) != 0;
}

void radio_sleep_timed(uint32_t ms) { // uint32_t with units of ms gives up to 49 days of sleep
    uint32_t sleepClockCycles = u32((u64(ms) * 1000000) / u64(slpclkPeriod_ns)); // calculate the number of clock cycles needed to wait for desired time
    
    println("sleepClockCycles = %lu", sleepClockCycles);
    
    radio_write_bits(MAINCNT3, 0b00000011, (sleepClockCycles >> 24) & 0b00000011);
    radio_write(MAINCNT2, (sleepClockCycles >> 16) & 0xFF);
    radio_write(MAINCNT1, (sleepClockCycles >> 8) & 0xFF);
    radio_write(MAINCNT0, sleepClockCycles & 0xFF);
    
    radio_set_bit(MAINCNT3, 7); // start the radio sleeping
}

void radio_trigger_tx(void) {
    radio_set_bit(TXNCON, 0); // set the TXNTRIG bit
    payload_seqNum++;
}

// creates and writes the MAC header (MHR) to the TXNFIFO on the MRF24J40

void radio_mhr_write(uint16_t * fifo_i_p) {
    uint8_t mhr_i = 0;

    //TODO use destination address of other mote rather than just broadcast

    // frame control
    mhr[mhr_i++] = 0x41; // pan ID compression, data frame
    mhr[mhr_i++] = 0x88; // 16 bit addresses, 2003 frame version
    // sequence number
    mhr[mhr_i++] = payload_seqNum;
    // address fields
    mhr[mhr_i++] = 0xFF; // destination PAN ID LSByte (0xFFFF broadcast)
    mhr[mhr_i++] = 0xFF; // MSByte
    mhr[mhr_i++] = 0xFF; // destination address LSByte (0xFFFF broadcast)
    mhr[mhr_i++] = 0xFF; // MSByte
    mhr[mhr_i++] = srcAddrL; // source address LSByte
    mhr[mhr_i++] = srcAddrH; // MSByte
    //memcpy(&(frame[mhrIndex]), sequenceNumberString, sequenceNumberLength); // payload

//    println("TXing [%d]+%d bytes: [0x%.2X%.2X, 0x%.2X, 0x%.2X%.2X, 0x%.2X%.2X, 0x%.2X%.2X] \"%s\"", 
//            mhrLength, sequenceNumberLength,
//            frame[0], frame[1], 
//            frame[frameCtrlLength], 
//            frame[frameCtrlLength + seqNumLength], frame[frameCtrlLength + seqNumLength + 1], frame[frameCtrlLength + seqNumLength + 2], frame[frameCtrlLength + seqNumLength + 3], frame[frameCtrlLength + seqNumLength + 4], frame[frameCtrlLength + seqNumLength + 5], 
//            &(frame[mhrLength]));
//
//    delay_ms(250);

    // write to TXNFIFO
    radio_write_fifo((*fifo_i_p)++, mhrLength);
    radio_write_fifo((*fifo_i_p)++, payload_totalLength);
    mhr_i = 0;
    while (mhr_i < mhrLength) {
        radio_write_fifo((*fifo_i_p)++, mhr[mhr_i++]);
    }
}

void radio_check_txstat(void) {
    uint8_t txstat = radio_read(TXSTAT);

    if (~txstat & TXNSTAT) { // TXNSTAT == 0 shows a successful transmission
        println("TX successful, SN = %d, TXSTAT = 0x%.2X", payload_seqNum - 1, txstat);
    } else {
        println("TX failed, SN = %d, TXSTAT = 0x%.2X", payload_seqNum - 1, txstat);
    }
}

uint8_t radio_read_long(uint16_t addr) {
    radio_spi_preamble();
    uint8_t value = spi_read_long(addr);
    radio_spi_postamble();

    return value;
}

uint8_t radio_read_short(uint8_t addr) {
    radio_spi_preamble();
    uint8_t value = spi_read_short(addr);
    radio_spi_postamble();

    return value;
}

void radio_write_long(uint16_t addr, uint8_t value) {
    radio_spi_preamble();
    spi_write_long(addr, value);
    radio_spi_postamble();
}

void radio_write_short(uint8_t addr, uint8_t value) {
    radio_spi_preamble();
    spi_write_short(addr, value);
    radio_spi_postamble();
}

void radio_rxfifo_flush(void) {
    radio_write(RXFLUSH, (radio_read(RXFLUSH) | _RXFLUSH));
}

void radio_printTxFifo() {
    printf("TXNFIFO = \"");
    uint8_t value;
    uint16_t fifo_i;
    for (fifo_i = 0; fifo_i < 6 + payload_totalLength; fifo_i++) {
        value = radio_read_long(fifo_i);    
        if (payload_isValidChar(value)) {
            printf("%c", value);
        } else {
            printf("<%.2X>", value);
        }
    }
    println("\"");
}

void radio_printAllRegisters(void) {
    // print out the values of all the registers on the MRF24J40
    uint16_t addr;
    for (addr = 0x00; addr <= 0x3F; addr++) {
        printf("%X=%X\r\n", addr, radio_read(addr));
    }
    for (addr = 0x200; addr <= 0x24C; addr++) {
        printf("%X=%X\r\n", addr, radio_read(addr));
    }
}