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
#include "SPI_functions.h"
#include "utils.h"
#include "stdio.h"
#include "payload.h"

#include <stdlib.h>

radio_if_t ifs = {.rx = 0, .tx = 0, .wake = 0};
uint8_t volatile rxBuffer[RXFIFO_SIZE] = {0};
uint8_t volatile txBuffer[TXNFIFO_SIZE] = {0};
uint8_t srcAddrH = 0xAA; // default address = 0xAA54
uint8_t srcAddrL = 0x54;
uint8_t mhr[mhrLength];

void mrf24j40_trigger_tx(void) {
    mrf24j40_write_short_ctrl_reg(TXNCON, mrf24j40_read_short_ctrl_reg(TXNCON) | TXNTRIG); 
}

// creates and writes the MAC header (MHR) to the TXNFIFO on the MRF24J40
void mrf24f40_mhr_write(uint16_t * fifo_i_p, uint16_t totalLength) {
    uint8_t mhr_i = 0;
            
    //TODO use destination address of other mote rather than just broadcast

    // frame control
    mhr[mhr_i++] = 0x41; // pan ID compression, data frame
    mhr[mhr_i++] = 0x88; // 16 bit addresses, 2003 frame version
    // sequence number
    mhr[mhr_i++] = seqNum;
    // address fields
    mhr[mhr_i++] = 0xFF; // destination PAN ID LSByte (0xFFFF broadcast)
    mhr[mhr_i++] = 0xFF; // MSByte
    mhr[mhr_i++] = 0xFF; // destination address LSByte (0xFFFF broadcast)
    mhr[mhr_i++] = 0xFF; // MSByte
    mhr[mhr_i++] = srcAddrL; // source address LSByte
    mhr[mhr_i++] = srcAddrH; // MSByte
    //memcpy(&(frame[mhrIndex]), sequenceNumberString, sequenceNumberLength); // payload

//            println("TXing [%d]+%d bytes: [0x%.2X%.2X, 0x%.2X, 0x%.2X%.2X, 0x%.2X%.2X, 0x%.2X%.2X] \"%s\"", 
//                    mhrLength, sequenceNumberLength,
//                    frame[0], frame[1], 
//                    frame[frameCtrlLength], 
//                    frame[frameCtrlLength + seqNumLength], frame[frameCtrlLength + seqNumLength + 1], frame[frameCtrlLength + seqNumLength + 2], frame[frameCtrlLength + seqNumLength + 3], frame[frameCtrlLength + seqNumLength + 4], frame[frameCtrlLength + seqNumLength + 5], 
//                    &(frame[mhrLength]));
//            
//            delay_ms(250);

    // write to TXNFIFO
    mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, mhrLength);
    mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, totalLength);
    mhr_i = 0;
    while (mhr_i < mhrLength) {
        mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, mhr[mhr_i++]);
    }
}

void mrf24j40_read_rx(void) {
    //uint8_t bbreg1 = mrf24j40_read_short_ctrl_reg(BBREG1);
    //mrf24j40_write_short_ctrl_reg(BBREG1, bbreg1 | RXDECINV); // disable receiving packets off air.

    uint8_t frameLength = mrf24j40_read_long_ctrl_reg(RXFIFO);

    uint16_t const fifoStart = RXFIFO + 1;
    uint16_t const fifoEnd = fifoStart + frameLength;
    uint16_t fifoIndex = fifoStart;
    uint16_t bufferIndex = 0;
    while (fifoIndex < fifoEnd) {
        rxBuffer[bufferIndex++] = mrf24j40_read_long_ctrl_reg(fifoIndex++);
    }

    uint8_t const fcsL = mrf24j40_read_long_ctrl_reg(fifoIndex++);
    uint8_t const fcsH = mrf24j40_read_long_ctrl_reg(fifoIndex++);
    uint8_t const lqi = mrf24j40_read_long_ctrl_reg(fifoIndex++);
    uint8_t const rssi = mrf24j40_read_long_ctrl_reg(fifoIndex++);
    
    mrf24j40_write_short_ctrl_reg(RXFLUSH, mrf24j40_read_short_ctrl_reg(RXFLUSH) | _RXFLUSH); // reset the RXFIFO pointer

    //mrf24j40_write_short_ctrl_reg(BBREG1, bbreg1 | ~(RXDECINV)); // enable receiving packets off air.

    printf("RX payload: [");
    uint16_t const payloadLength = frameLength - 2;
    uint8_t value;
    bufferIndex = 0;
    while (bufferIndex < payloadLength) {
        value = rxBuffer[bufferIndex];
        if (bufferIndex < mhrLength - 1) {
            printf("0x%.2X, ", value);
        } else if (bufferIndex == mhrLength - 1) {
            printf("0x%.2X] \"", value);
        } else {
            if (isValidPayloadChar(value)) {
                printf("%c", value);
            } else {
                printf("<%.2X>", value);
            }
        }
        bufferIndex++;
    }
    println("\"");
    
    println("FCSH = 0x%.2X", fcsH);
    println("FCSL = 0x%.2X", fcsL);
    println("LQI = %d", lqi);
    println("RSSI = %d", rssi);
}

void mrf24f40_check_txstat(void) {
    uint8_t txstat = mrf24j40_read_short_ctrl_reg(TXSTAT);

    if (~txstat & TXNSTAT) { // TXNSTAT == 0 shows a successful transmission
        println("TX successful, TXSTAT = 0x%.2X", txstat);
    } else {
        println("TX failed, TXSTAT = 0x%.2X", txstat);
    }
}

uint8_t mrf24j40_read_long_ctrl_reg(uint16_t addr) {
  mrf24j40_spi_preamble();
  uint8_t value = spi_read_long(addr);
  mrf24j40_spi_postamble();

  return value;
}

uint8_t mrf24j40_read_short_ctrl_reg(uint8_t addr) {
  mrf24j40_spi_preamble();
  uint8_t value = spi_read_short(addr);
  mrf24j40_spi_postamble();
  
  return value;
}

void mrf24j40_write_long_ctrl_reg(uint16_t addr, uint8_t value) {
  mrf24j40_spi_preamble();
  spi_write_long(addr, value);
  mrf24j40_spi_postamble();
}

void mrf24j40_write_short_ctrl_reg(uint8_t addr, uint8_t value) {
  mrf24j40_spi_preamble();
  spi_write_short(addr, value);
  mrf24j40_spi_postamble();
}

void mrf24j40_ie(void) {
  mrf24j40_write_short_ctrl_reg(MRF24J40_INTCON, ~(TXNIE | RXIE | SECIE));
}

void mrf24j40_pwr_reset(void) {
  mrf24j40_write_short_ctrl_reg(SOFTRST, RSTPWR);
}

void mrf24j40_bb_reset(void) {
  mrf24j40_write_short_ctrl_reg(SOFTRST, RSTBB);
}

void mrf24j40_mac_reset(void) {
  mrf24j40_write_short_ctrl_reg(SOFTRST, RSTMAC);
}

void mrf24j40_rf_reset(void) {
  uint8_t old = mrf24j40_read_short_ctrl_reg(RFCTL);

  mrf24j40_write_short_ctrl_reg(RFCTL, old | RFRST);
  mrf24j40_write_short_ctrl_reg(RFCTL, old & ~RFRST);
  mrf24j40_delay_ms(2);
}

uint8_t mrf24j40_get_pending_frame(void) {
  return (mrf24j40_read_short_ctrl_reg(TXNCON) >> 4) & 0x01;
}

void mrf24j40_rxfifo_flush(void) {
  mrf24j40_write_short_ctrl_reg(RXFLUSH, (mrf24j40_read_short_ctrl_reg(RXFLUSH) | _RXFLUSH));
}

void mrf24j40_set_channel(int16_t ch) {
  mrf24j40_write_long_ctrl_reg(RFCON0, CHANNEL(ch) | RFOPT(0x03));
  mrf24j40_rf_reset();
}

void mrf24j40_set_promiscuous(bool crc_check) {
  uint8_t w = NOACKRSP;
  if (!crc_check) {
    w |= ERRPKT;
  } else {
    w |= PROMI;
  }

  mrf24j40_write_short_ctrl_reg(RXMCR, w);
}

void mrf24j40_set_coordinator(void) {
  mrf24j40_write_short_ctrl_reg(RXMCR, mrf24j40_read_short_ctrl_reg(RXMCR) | PANCOORD);
}

void mrf24j40_clear_coordinator(void) {
  mrf24j40_write_short_ctrl_reg(RXMCR, mrf24j40_read_short_ctrl_reg(RXMCR) & ~PANCOORD);
}

void mrf24j40_set_pan(uint8_t *pan) {
  mrf24j40_write_short_ctrl_reg(PANIDL, pan[0]);
  mrf24j40_write_short_ctrl_reg(PANIDH, pan[1]);
}

void mrf24j40_set_short_addr(uint8_t *addr) {
  mrf24j40_write_short_ctrl_reg(SADRL, addr[0]);
  mrf24j40_write_short_ctrl_reg(SADRH, addr[1]);
}

void mrf24j40_set_eui(uint8_t *eui) {
  mrf24j40_write_short_ctrl_reg(EADR0, eui[0]);
  mrf24j40_write_short_ctrl_reg(EADR1, eui[1]);
  mrf24j40_write_short_ctrl_reg(EADR2, eui[2]);
  mrf24j40_write_short_ctrl_reg(EADR3, eui[3]);
  mrf24j40_write_short_ctrl_reg(EADR4, eui[4]);
  mrf24j40_write_short_ctrl_reg(EADR5, eui[5]);
  mrf24j40_write_short_ctrl_reg(EADR6, eui[6]);
  mrf24j40_write_short_ctrl_reg(EADR7, eui[7]);
}

void mrf24j40_set_coordinator_short_addr(uint8_t *addr) {
  mrf24j40_write_long_ctrl_reg(ASSOSADR0, addr[0]);
  mrf24j40_write_long_ctrl_reg(ASSOSADR1, addr[1]);
}

void mrf24j40_set_coordinator_eui(uint8_t *eui) {
  mrf24j40_write_long_ctrl_reg(ASSOEADR0, eui[0]);
  mrf24j40_write_long_ctrl_reg(ASSOEADR1, eui[1]);
  mrf24j40_write_long_ctrl_reg(ASSOEADR2, eui[2]);
  mrf24j40_write_long_ctrl_reg(ASSOEADR3, eui[3]);
  mrf24j40_write_long_ctrl_reg(ASSOEADR4, eui[4]);
  mrf24j40_write_long_ctrl_reg(ASSOEADR5, eui[5]);
  mrf24j40_write_long_ctrl_reg(ASSOEADR6, eui[6]);
  mrf24j40_write_long_ctrl_reg(ASSOEADR7, eui[7]);
}

void mrf24j40_set_key(uint16_t address, uint8_t *key) {
  mrf24j40_spi_preamble();
  spi_write_long(address, 1);
  
  int16_t i;
  for (i = 0; i < 16; i++) {
    spi_write(key[i]);
  }

  mrf24j40_spi_postamble();
}

void mrf24j40_hard_reset(void) {
  mrf24j40_reset_pin(0);
  mrf24j40_delay_ms(500); // wait at least 2ms
  mrf24j40_reset_pin(1);
  mrf24j40_delay_ms(500); // wait at least 2ms
}

void mrf24j40_initialize(void) {        
  mrf24j40_cs_pin(1);
  mrf24j40_wake_pin(1);
  
  mrf24j40_hard_reset();
  
  //mrf24j40_write_short_ctrl_reg(SOFTRST, 0x07); // Perform a software Reset. The bits will be automatically cleared to ?0? by hardware.
 
  mrf24j40_write_short_ctrl_reg(PACON2, 0x98); // Initialize FIFOEN = 1 and TXONTS = 0x6.
  mrf24j40_write_short_ctrl_reg(TXSTBL, 0x95); // Initialize RFSTBL = 0x9.
  
  mrf24j40_write_long_ctrl_reg(RFCON0, 0x03); // Initialize RFOPT = 0x03, Channel 11 (2.405GHz)
  mrf24j40_write_long_ctrl_reg(RFCON1, 0x01); // Initialize VCOOPT = 0x02.
  mrf24j40_write_long_ctrl_reg(RFCON2, 0x80); // Enable PLL (PLLEN = 1).
  mrf24j40_write_long_ctrl_reg(RFCON6, 0x90); // Initialize TXFIL = 1 and 20MRECVR = 1.
  mrf24j40_write_long_ctrl_reg(RFCON7, 0x80); // Initialize SLPCLKSEL = 0x2 (100 kHz Internal oscillator).
  mrf24j40_write_long_ctrl_reg(RFCON8, 0x10); // Initialize RFVCO = 1.
  mrf24j40_write_long_ctrl_reg(SLPCON1, 0x21); // Initialize CLKOUTEN = 1 and SLPCLKDIV = 0x01.

  mrf24j40_write_short_ctrl_reg(BBREG2, 0x80); // Set CCA mode to ED.
  mrf24j40_write_short_ctrl_reg(CCAEDTH, 0x60); // Set CCA ED threshold.
  mrf24j40_write_short_ctrl_reg(BBREG6, 0x40); // Set appended RSSI value to RXFIFO.

  mrf24j40_write_short_ctrl_reg(MRF24J40_INTCON, 0xB6); // Enable wake, RX and TX normal interrupts.
  // tx power set to 0dBm at reset
  
  mrf24j40_write_short_ctrl_reg(RFCTL, 0x04); // Reset RF state machine.
  mrf24j40_delay_us(200);
  mrf24j40_write_short_ctrl_reg(RFCTL, 0x00);
  mrf24j40_delay_us(200); // delay at least 192 ?s 
  
//  mrf24j40_cs_pin(1);
//  mrf24j40_wake_pin(1);
//  
//  mrf24j40_hard_reset();
//  
//  mrf24j40_write_short_ctrl_reg(SOFTRST, (RSTPWR | RSTBB | RSTMAC));
//
//  mrf24j40_delay_us(192);
// 
//  mrf24j40_write_short_ctrl_reg(PACON2, FIFOEN | TXONTS(0x18));
//  mrf24j40_write_short_ctrl_reg(TXSTBL, RFSTBL(9) | MSIFS(5));
//  mrf24j40_write_long_ctrl_reg(RFCON1, VCOOPT(0x01));
//  mrf24j40_write_long_ctrl_reg(RFCON2, PLLEN);
//  mrf24j40_write_long_ctrl_reg(RFCON6, _20MRECVR);
//  mrf24j40_write_long_ctrl_reg(RFCON7, SLPCLKSEL(0x02));
//  mrf24j40_write_long_ctrl_reg(RFCON8, RFVCO);
//  mrf24j40_write_long_ctrl_reg(SLPCON1, SLPCLKDIV(1) | CLKOUTDIS);
//  mrf24j40_write_short_ctrl_reg(RXFLUSH, (WAKEPAD | WAKEPOL));
//
//  mrf24j40_write_short_ctrl_reg(BBREG2, CCAMODE(0x02) | CCASTH(0x00));
//  mrf24j40_write_short_ctrl_reg(CCAEDTH, 0x60);
//  mrf24j40_write_short_ctrl_reg(BBREG6, RSSIMODE2);
//
//  mrf24j40_rxfifo_flush();
//  
//  mrf24j40_ie();
}

void mrf24j40_sleep(void) {
  mrf24j40_write_short_ctrl_reg(WAKECON, IMMWAKE);

  uint8_t r = mrf24j40_read_short_ctrl_reg(SLPACK);
  mrf24j40_wake_pin(0);

  mrf24j40_pwr_reset();
  mrf24j40_write_short_ctrl_reg(SLPACK, r | _SLPACK);
}

void mrf24j40_wakeup(void) {
  mrf24j40_wake_pin(1);
  mrf24j40_rf_reset();
}

void mrf24j40_txpkt(uint8_t *frame, int16_t hdr_len, int16_t sec_hdr_len, int16_t payload_len) {
  int16_t frame_len = hdr_len + sec_hdr_len + payload_len;

  uint8_t w = mrf24j40_read_short_ctrl_reg(TXNCON);
  w &= ~(TXNSECEN);
  w &= ~(TXNACKREQ);

  if (IEEE_802_15_4_HAS_SEC(frame[0])) {
    w |= TXNSECEN;
  }

  if (IEEE_802_15_4_WANTS_ACK(frame[0])) {
    w |= TXNACKREQ;
  }

  mrf24j40_spi_preamble();
  
  spi_write_long(TXNFIFO, hdr_len);
  spi_write(frame_len);

  while (frame_len-- > 0) {
    spi_write(*frame++);
  }
  
  mrf24j40_spi_postamble();

  //mrf24j40_write_short_ctrl_reg(TXNCON, w | TXNTRIG);
}

void mrf24j40_set_cipher(uint8_t rxcipher, uint8_t txcipher) {
  mrf24j40_write_short_ctrl_reg(SECCON0, RXCIPHER(rxcipher) | TXNCIPHER(txcipher));
}

bool mrf24j40_rx_sec_fail(void) {
  bool rx_sec_fail = (mrf24j40_read_short_ctrl_reg(RXSR) >> 2) & 0x01;
  mrf24j40_write_short_ctrl_reg(RXSR, 0x00);
  return rx_sec_fail;
}

void mrf24j40_sec_intcb(bool accept) {
  uint8_t w = mrf24j40_read_short_ctrl_reg(SECCON0);

  w |= accept ? SECSTART : SECIGNORE;
  mrf24j40_write_short_ctrl_reg(SECCON0, w);
}

int16_t mrf24j40_txpkt_intcb(void) {
  uint8_t stat = mrf24j40_read_short_ctrl_reg(TXSTAT);
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

int16_t mrf24j40_rxpkt_intcb(uint8_t *buf, uint8_t *plqi, uint8_t *prssi) {
  mrf24j40_write_short_ctrl_reg(BBREG1, mrf24j40_read_short_ctrl_reg(BBREG1) | RXDECINV);

  mrf24j40_spi_preamble();
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

  mrf24j40_spi_postamble();

  mrf24j40_rxfifo_flush();
  mrf24j40_write_short_ctrl_reg(BBREG1, mrf24j40_read_short_ctrl_reg(BBREG1) & ~RXDECINV);
  
  return flen;
}

int16_t mrf24j40_int_tasks(void) {
  int16_t ret = 0;

  uint8_t stat = mrf24j40_read_short_ctrl_reg(INTSTAT);

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
