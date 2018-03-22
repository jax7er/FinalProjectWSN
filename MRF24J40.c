/*
 * File:   MRF24J40.c
 * Author: Jack
 *
 * Created on 17 February 2018, 21:34
 */


#include "xc.h"

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

#include "config.h"
#include "MRF24J40.h"
#include "interface.h"
#include "utils.h"
#include "stdio.h"
#include "payload.h"
#include "delay.h"

#include <stdlib.h>

radio_if_t ifs = {.event = 0, .rx = 0, .tx = 0, .wake = 0};
uint8_t rxBuffer[RXFIFO_SIZE] = {0};
uint8_t txBuffer[TXNFIFO_SIZE] = {0};
uint8_t srcAddrH = 0x13; // default address = 0x1357
uint8_t srcAddrL = 0x57;
uint8_t mhr[mhrLength] = {0};
uint32_t slpclkPeriod_ns = 0;

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

void mrf24f40_mhr_write(uint16_t * fifo_i_p) {
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

void mrf24f40_check_txstat(void) {
    uint8_t txstat = radio_read(TXSTAT);

    if (~txstat & TXNSTAT) { // TXNSTAT == 0 shows a successful transmission
        println("TX successful, SN = %d, TXSTAT = 0x%.2X", payload_seqNum - 1, txstat);
    } else {
        println("TX failed, SN = %d, TXSTAT = 0x%.2X", payload_seqNum - 1, txstat);
    }
}

uint8_t radio_read_long_ctrl_reg(uint16_t addr) {
    radio_spi_preamble();
    uint8_t value = spi_read_long(addr);
    radio_spi_postamble();

    return value;
}

uint8_t radio_read_short_ctrl_reg(uint8_t addr) {
    radio_spi_preamble();
    uint8_t value = spi_read_short(addr);
    radio_spi_postamble();

    return value;
}

void radio_write_long_ctrl_reg(uint16_t addr, uint8_t value) {
    radio_spi_preamble();
    spi_write_long(addr, value);
    radio_spi_postamble();
}

void radio_write_short_ctrl_reg(uint8_t addr, uint8_t value) {
    radio_spi_preamble();
    spi_write_short(addr, value);
    radio_spi_postamble();
}

void radio_ie(void) {
    radio_write(MRF24J40_INTCON, ~(TXNIE | RXIE | SECIE));
}

void radio_pwr_reset(void) {
    radio_write(SOFTRST, RSTPWR);
}

void radio_bb_reset(void) {
    radio_write(SOFTRST, RSTBB);
}

void radio_mac_reset(void) {
    radio_write(SOFTRST, RSTMAC);
}

void radio_rf_reset(void) {
    uint8_t old = radio_read(RFCTL);

    radio_write(RFCTL, old | RFRST);
    radio_write(RFCTL, old & ~RFRST);
    delay_ms(2);
}

uint8_t radio_get_pending_frame(void) {
    return (radio_read(TXNCON) >> 4) & 0x01;
}

void radio_rxfifo_flush(void) {
    radio_write(RXFLUSH, (radio_read(RXFLUSH) | _RXFLUSH));
}

void radio_set_channel(int16_t ch) {
    radio_write(RFCON0, CHANNEL(ch) | RFOPT(0x03));
    radio_rf_reset();
}

void radio_set_promiscuous(bool crc_check) {
    uint8_t w = NOACKRSP;
    if (!crc_check) {
        w |= ERRPKT;
    } else {
        w |= PROMI;
    }

    radio_write(RXMCR, w);
}

void radio_set_coordinator(void) {
    radio_write(RXMCR, radio_read(RXMCR) | PANCOORD);
}

void radio_clear_coordinator(void) {
    radio_write(RXMCR, radio_read(RXMCR) & ~PANCOORD);
}

void radio_set_pan(uint8_t *pan) {
    radio_write(PANIDL, pan[0]);
    radio_write(PANIDH, pan[1]);
}

void radio_set_short_addr(uint8_t *addr) {
    radio_write(SADRL, addr[0]);
    radio_write(SADRH, addr[1]);
}

void radio_set_eui(uint8_t *eui) {
    radio_write(EADR0, eui[0]);
    radio_write(EADR1, eui[1]);
    radio_write(EADR2, eui[2]);
    radio_write(EADR3, eui[3]);
    radio_write(EADR4, eui[4]);
    radio_write(EADR5, eui[5]);
    radio_write(EADR6, eui[6]);
    radio_write(EADR7, eui[7]);
}

void radio_set_coordinator_short_addr(uint8_t *addr) {
    radio_write(ASSOSADR0, addr[0]);
    radio_write(ASSOSADR1, addr[1]);
}

void radio_set_coordinator_eui(uint8_t *eui) {
    radio_write(ASSOEADR0, eui[0]);
    radio_write(ASSOEADR1, eui[1]);
    radio_write(ASSOEADR2, eui[2]);
    radio_write(ASSOEADR3, eui[3]);
    radio_write(ASSOEADR4, eui[4]);
    radio_write(ASSOEADR5, eui[5]);
    radio_write(ASSOEADR6, eui[6]);
    radio_write(ASSOEADR7, eui[7]);
}

void radio_set_key(uint16_t address, uint8_t *key) {
    radio_spi_preamble();
    spi_write_long(address, 1);

    int16_t i;
    for (i = 0; i < 16; i++) {
        spi_write(key[i]);
    }

    radio_spi_postamble();
}

void radio_hard_reset(void) {
    radio_reset_pin(0);
    delay_ms(500); // wait at least 2ms
    radio_reset_pin(1);
    delay_ms(500); // wait at least 2ms
}

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
    slpclkPeriod_ns = (slpclkPeriod_ns * 50) / 16; // *50ns/16
//    println("Radio sleep clock calibration done, SLPCLK period = %luns", slpclkPeriod_ns);

    //  radio_cs_pin(1);
    //  radio_wake_pin(1);
    //  
    //  radio_hard_reset();
    //  
    //  radio_write(SOFTRST, (RSTPWR | RSTBB | RSTMAC));
    //
    //  delay_us(192);
    // 
    //  radio_write(PACON2, FIFOEN | TXONTS(0x18));
    //  radio_write(TXSTBL, RFSTBL(9) | MSIFS(5));
    //  radio_write(RFCON1, VCOOPT(0x01));
    //  radio_write(RFCON2, PLLEN);
    //  radio_write(RFCON6, _20MRECVR);
    //  radio_write(RFCON7, SLPCLKSEL(0x02));
    //  radio_write(RFCON8, RFVCO);
    //  radio_write(SLPCON1, SLPCLKDIV(1) | CLKOUTDIS);
    //  radio_write(RXFLUSH, (WAKEPAD | WAKEPOL));
    //
    //  radio_write(BBREG2, CCAMODE(0x02) | CCASTH(0x00));
    //  radio_write(CCAEDTH, 0x60);
    //  radio_write(BBREG6, RSSIMODE2);
    //
    //  radio_rxfifo_flush();
    //  
    //  radio_ie();
}

void radio_sleep(void) {
    radio_write(WAKECON, IMMWAKE);

    uint8_t r = radio_read(SLPACK);
    radio_wake_pin(0);

    radio_pwr_reset();
    radio_write(SLPACK, r | _SLPACK);
}

void radio_wakeup(void) {
    radio_wake_pin(1);
    radio_rf_reset();
}

void radio_txpkt(uint8_t *frame, int16_t hdr_len, int16_t sec_hdr_len, int16_t payload_len) {
    int16_t frame_len = hdr_len + sec_hdr_len + payload_len;

    uint8_t w = radio_read(TXNCON);
    w &= ~(TXNSECEN);
    w &= ~(TXNACKREQ);

    if (IEEE_802_15_4_HAS_SEC(frame[0])) {
        w |= TXNSECEN;
    }

    if (IEEE_802_15_4_WANTS_ACK(frame[0])) {
        w |= TXNACKREQ;
    }

    radio_spi_preamble();

    spi_write_long(TXNFIFO, hdr_len);
    spi_write(frame_len);

    while (frame_len-- > 0) {
        spi_write(*frame++);
    }

    radio_spi_postamble();

    //radio_write(TXNCON, w | TXNTRIG);
}

void radio_set_cipher(uint8_t rxcipher, uint8_t txcipher) {
    radio_write(SECCON0, RXCIPHER(rxcipher) | TXNCIPHER(txcipher));
}

bool radio_rx_sec_fail(void) {
    bool rx_sec_fail = (radio_read(RXSR) >> 2) & 0x01;
    radio_write(RXSR, 0x00);
    return rx_sec_fail;
}

void radio_sec_intcb(bool accept) {
    uint8_t w = radio_read(SECCON0);

    w |= accept ? SECSTART : SECIGNORE;
    radio_write(SECCON0, w);
}

int16_t radio_txpkt_intcb(void) {
    uint8_t stat = radio_read(TXSTAT);
    if (stat & TXNSTAT) {
        if (stat & CCAFAIL) {
            return EBUSY;
        } else {
            return EIO;
        }
    } else {
        return 0;
    }
}

int16_t radio_rxpkt_intcb(uint8_t *buf, uint8_t *plqi, uint8_t *prssi) {
    radio_write(BBREG1, radio_read(BBREG1) | RXDECINV);

    radio_spi_preamble();
    uint16_t flen = spi_read_long(RXFIFO);

    uint16_t i;
    for (i = 0; i < flen; i++) {
        *buf++ = spi_read();
    }

    uint8_t lqi = spi_read();
    uint8_t rssi = spi_read();

    if (plqi != NULL) {
        *plqi = lqi;
    }

    if (prssi != NULL) {
        *prssi = rssi;
    }

    radio_spi_postamble();

    radio_rxfifo_flush();
    radio_write(BBREG1, radio_read(BBREG1) & ~RXDECINV);

    return flen;
}

int16_t radio_int_tasks(void) {
    int16_t ret = 0;

    uint8_t stat = radio_read(INTSTAT);

    if (stat & RXIF) {
        ret |= MRF24J40_INT_RX;
    }

    if (stat & TXNIF) {
        ret |= MRF24J40_INT_TX;
    }

    if (stat & SECIF) {
        ret |= MRF24J40_INT_SEC;
    }

    return ret;
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
        printf("%x=%x\r\n", addr, radio_read(addr));
    }
    for (addr = 0x200; addr <= 0x24C; addr++) {
        printf("%x=%x\r\n", addr, radio_read(addr));
    }
}