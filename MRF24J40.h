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

#ifndef MRF24J40_H
#define	MRF24J40_H

#include <stdint.h>
#include <stdbool.h>

#include "config.h"

/* Return values */
#define MRF24J40_INT_RX		0x01
#define MRF24J40_INT_TX		0x02
#define MRF24J40_INT_SEC	0x04
#define MRF24J40_INT_SLP	0x08
#define MRF24J40_INT_ENC	0x10
#define MRF24J40_INT_DEC	0x20

#define EIO			1
#define EBUSY			2

/* IEEE 802.15.4 constants for building the MHR (MAC header) */
#define IEEE_FRAME_TYPE_BEACON    0x0
#define IEEE_FRAME_TYPE_DATA      0x1
#define IEEE_FRAME_TYPE_ACK       0x2 // auto sent by the MRF24J40
#define IEEE_FRAME_TYPE_MACCMD    0x3
#define IEEE_ADDR_MODE_NONE       0x0
#define IEEE_ADDR_MODE_16BIT      0x2 // 0xFFFF is the broadcast address
#define IEEE_ADDR_MODE_64BIT      0x3
#define IEEE_ADDR_BROADCAST_16BIT 0xFFFF
#define IEEE_ADDR_BROADCAST_64BIT 0xFFFFFFFFFFFFFFFF
#define IEEE_FRAME_VER_2003       0x0
#define IEEE_FRAME_VER_2011       0x1

#define IEEE_MAKE_FRAME_CTRL(srcAddrMode, frameVer, destAddrMode, panIdComp, ackReq, framePend, secEn, frameType) \
        (((srcAddrMode) << 14) | ((frameVer) << 12) | ((destAddrMode) << 10) | ((panIdComp) << 6) | ((ackReq) << 5) | ((framePend) << 4) | ((secEn) << 3) | (frameType))

#define IEEE_FRAME_CTRL_16BIT_2003_ACK_NO_SEC_DATA IEEE_MAKE_FRAME_CTRL(IEEE_ADDR_MODE_16BIT, IEEE_FRAME_VER_2003, IEEE_ADDR_MODE_16BIT, 0, 1, 0, 0, IEEE_FRAME_TYPE_DATA)
#define IEEE_FRAME_CTRL_16BIT_2003_NO_ACK_NO_SEC_DATA IEEE_MAKE_FRAME_CTRL(IEEE_ADDR_MODE_16BIT, IEEE_FRAME_VER_2003, IEEE_ADDR_MODE_16BIT, 0, 0, 0, 0, IEEE_FRAME_TYPE_DATA)

/* IEEE 802.15.4 constants needed for some flags */
#define IEEE_802_15_4_HAS_SEC(x)      ((x >> 3) & 0x01)
#define IEEE_802_15_4_WANTS_ACK(x)     ((x >> 5) & 0x01)

/* enc dec parameters */
#define MRF24J40_TX_KEY		0x01
#define MRF24J40_RX_KEY		0x02
#define MRF24J40_UP_KEY		MRF24J40_TX_KEY

#define MRF24J40_AES_CBC_MAC32	0x07
#define MRF24J40_AES_CBC_MAC64	0x06
#define MRF24J40_AES_CBC_MAC128	0x05
#define MRF24J40_AES_CCM32	0x04
#define MRF24J40_AES_CCM64	0x03
#define MRF24J40_AES_CCM128	0x02
#define MRF24J40_AES_CTR	0x01
#define MRF24J40_ALGO_NONE	0x00

/* Short Address Control Register Map */
#define RXMCR		0x00
#define PANIDL		0x01
#define PANIDH		0x02
#define	SADRL		0x03
#define SADRH		0x04
#define EADR0		0x05
#define EADR1		0x06
#define EADR2		0x07
#define EADR3		0x08
#define EADR4		0x09
#define EADR5		0x0A
#define EADR6		0x0B
#define EADR7		0x0C
#define RXFLUSH		0x0D

#define ORDER		0x10
#define TXMCR		0x11
#define ACKTMOUT	0x12
#define ESLOTG1		0x13
#define SYMTICKL	0x14
#define SYMTICKH	0x15
#define PACON0		0x16
#define PACON1		0x17
#define PACON2		0x18
#define TXBCON0		0x1A
#define TXNCON		0x1B
#define TXG1CON		0x1C
#define TXG2CON		0x1D
#define ESLOTG23	0x1E
#define ESLOTG45	0x1F

#define ESLOTG67	0x20
#define TXPEND		0x21
#define WAKECON		0x22
#define FRMOFFSET	0x23
#define TXSTAT		0x24
#define TXBCON1		0x25
#define GATECLK		0x26
#define TXTIME		0x27
#define HSYMTMRL	0x28
#define HSYMTMRH	0x29
#define SOFTRST		0x2A
#define SECCON0		0x2c
#define SECCON1		0x2d
#define TXSTBL		0x2e

#define RXSR		0x30
#define INTSTAT		0x31
#define MRF24J40_INTCON 0x32
#define GPIO		0x33
#define TRISGPIO	0x34
#define SLPACK		0x35
#define RFCTL		0x36
#define SECCR2		0x37
#define BBREG0		0x38
#define BBREG1		0x39
#define BBREG2		0x3A
#define BBREG3		0x3B
#define BBREG4		0x3C
#define BBREG6		0x3E
#define CCAEDTH		0x3F

/* Long Address Control Register Map */
#define RFCON0		0x200
#define RFCON1		0x201
#define RFCON2		0x202
#define RFCON3		0x203
#define RFCON5		0x205
#define RFCON6		0x206
#define RFCON7		0x207
#define RFCON8		0x208
#define SLPCAL0     0x209
#define SLPCAL1     0x20A
#define SLPCAL2     0x20B
#define RFSTATE		0x20F

#define RSSI		0x210
#define SLPCON0		0x211
#define SLPCON1		0x220

#define WAKETIMEL	0x222
#define WAKETIMEH	0x223

#define MAINCNT0	0x226
#define MAINCNT1	0x227
#define MAINCNT2	0x228
#define MAINCNT3	0x229

#define ASSOEADR0       0x230
#define ASSOEADR1       0x231
#define ASSOEADR2       0x232
#define ASSOEADR3       0x233
#define ASSOEADR4       0x234
#define ASSOEADR5       0x235
#define ASSOEADR6       0x236
#define ASSOEADR7       0x237

#define ASSOSADR0       0x238
#define ASSOSADR1       0x239

#define UPNONCE0	0x240
#define UPNONCE1	0x241
#define UPNONCE2	0x242
#define UPNONCE3	0x243
#define UPNONCE4	0x244
#define UPNONCE5	0x245
#define UPNONCE6	0x246
#define UPNONCE7	0x247
#define UPNONCE8	0x248
#define UPNONCE9	0x249
#define UPNONCE10	0x24A
#define UPNONCE11	0x24B
#define UPNONCE12	0x24C

/* Long Address Memory Map */
#define TXNFIFO		0x000 /* - 0x07F, 128 bytes */
#define TXBFIFO		0x080 /* - 0x0FF, 128 bytes */
#define TXG1FIFO	0x100 /* - 0x17F, 128 bytes */
#define TXG2FIFO	0x180 /* - 0x1FF, 128 bytes */
#define SECKFIFO	0x280 /* - 0x2BF, 64 bytes */
#define SECKTXNFIFO	0x280 /* - 0x28F, 16 bytes */
#define SECKRXFIFO	0x2B0 /* - 0x2BF, 16 bytes */
#define RXFIFO		0x300 /* - 0x38F, 144 bytes */
#define TXNFIFO_SIZE 128
#define RXFIFO_SIZE  144

/* RXMCR */
#define NOACKRSP	(1<<5)
#define PANCOORD	(1<<3)
#define COORD		(1<<2)
#define ERRPKT		(1<<1)
#define PROMI		(1)

/* RXFLUSH */
#define WAKEPOL		(1<<6)
#define WAKEPAD		(1<<5)
#define CMDONLY		(1<<3)
#define DATAONLY	(1<<2)
#define BCNONLY		(1<<1)
#define _RXFLUSH	(1)

/* TXMCR */
#define NOCSMA		(1<<7)
#define BATLIFEXT	(1<<6)
#define SLOTTED		(1<<5)
#define MACMINBE(x)	((x & 0x03)<<3)
#define CSMABF(x)	(x & 0x07)

/* ACKTMOUT */
#define DRPACK		(1<<7)

/* PACON2 */
#define FIFOEN		(1<<7)
#define TXONTS(x)       (x & 0x3F)

/* TXNCON */
#define FPSTAT		(1<<4)
#define INDIRECT	(1<<3)
#define TXNACKREQ	(1<<2)
#define TXNSECEN	(1<<1)
#define TXNTRIG		(1)

/* TXPEND */
#define FPACK		(1)

/* WAKECON */
#define IMMWAKE		(1<<7)
#define REGWAKE		(1<<6)

/* TXSTAT */
#define CCAFAIL		(1<<5)
#define TXNSTAT		(1)

/* SOFTRST */
#define RSTPWR		(1<<2)
#define RSTBB		(1<<1)
#define RSTMAC		(1)

/* SECCON0 */
#define SECIGNORE	(1<<7)
#define SECSTART	(1<<6)
#define RXCIPHER(x)	((x & 0x07) << 3)
#define TXNCIPHER(x)	((x & 0x07))

/* SECCON1 */
#define DISDEC		(1<<1)
#define DISENC		(1)

/* TXSTBL */
#define RFSTBL(x)	((x & 0x0f) << 4)
#define MSIFS(x)	((x & 0x0f))

/* RXSR */
#define UPSECERR	(1<<6)
#define SECDECERR	(1<<2)

/* INTSTAT */
#define SLPIF		(1<<7)
#define WAKEIF		(1<<6)
#define HSYMTMRIF	(1<<5)
#define SECIF		(1<<4)
#define RXIF		(1<<3)
#define TXG2IF		(1<<2)
#define TXG1IF		(1<<1)
#define TXNIF		(1)

/* INTCON */
#define SLPIE		(1<<7)
#define WAKEIE		(1<<6)
#define HSYMTMRIE	(1<<5)
#define SECIE		(1<<4)
#define RXIE		(1<<3)
#define TXG2IE		(1<<2)
#define TXG1IE		(1<<1)
#define TXNIE		(1)

/* SLPACK */
#define _SLPACK		(1<<7)
#define WAKECNT_L(x)	(x & 0x03F)

/* RFCTL */
#define WAKECNT_H(x)	((x & 0x03) << 3)
#define RFRST		(1<<2)
#define RFTXMODE	(1<<1)
#define RFRXMODE	(1)

/* SECCR2 */
#define UPDEC		(1<<7)
#define UPENC		(1<<6)

/* BBREG0 */
#define TURBO		(1)

/* BBREG1 */
#define RXDECINV	(1<<2)

/* BBREG2 */
#define CCAMODE(x)	((x & 0x03) <<6)
#define CCASTH(x)	((x & 0x0F) <<2)

/* BBREG3 */
#define PREVALIDTH(x)	((x & 0x0F) <<4)

/* BBREG4 */
#define CSTH(x)		((x & 0x07) << 5)

/* BBREG6 */
#define RSSIMODE1	(1 << 7)
#define RSSIMODE2	(1<<6)
#define RSSIRDY		(1)

/* RFCON0 */
#define CHANNEL(x)	((x & 0x0F) << 4)
#define RFOPT(x)	((x & 0x0F))

/* RFCON1 */
#define VCOOPT(x)	((x & 0xFF))

/* RFCON2 */
#define PLLEN		(1<<7)

/* RFCON3 */
#define TXPWRL(x)	((x & 0x03) << 6)
#define TXPWRS(x)	((x & 0x07) << 3)

/* RFCON6 */
#define TXFIL		(1 << 7)
#define _20MRECVR       (1 << 4)
#define BATEN           (1 << 3)

/* RFCON 7 */
#define SLPCLKSEL(x)	((x & 0x03) << 6)
#define SLPCLKSEL_100k	(SLPCLKSEL(0x02))
#define SLPCLKSEL_32k	(SLPCLKSEL(0x01))

/* RFCON8 */
#define RFVCO		(1 << 4)

/* SLPCON0 */
#define SLPCLKEN	(1)

/* SLPCON1 */
#define CLKOUTDIS	(1 << 5)	/* CLKOUTEN' */
#define SLPCLKDIV(x)	((x & 0x1F))	/* division ratio: 2^(SLPCLKDIV) */

#define radio_spi_preamble() volatile uint8_t tmpIE = radio_get_ie(); radio_set_ie(0); radio_cs_pin(0);
#define radio_spi_postamble() radio_cs_pin(1); radio_set_ie(tmpIE);

#define radio_is_short_addr(a)    (((uint16_t)(a)) <= 0x3F)
#define radio_read_short          radio_read_short_ctrl_reg
#define radio_read_long           radio_read_long_ctrl_reg
#define radio_read_fifo           radio_read_long
#define radio_read(r)             (radio_is_short_addr(r) ? radio_read_short(((uint8_t)(r))) : radio_read_long(((uint16_t)(r))))
#define radio_read_bit(r, b)      ((radio_read(r) & (1 << (b))) != 0)
#define radio_write_short         radio_write_short_ctrl_reg
#define radio_write_long          radio_write_long_ctrl_reg
#define radio_write_fifo          radio_write_long
#define radio_write(r, d)         do { if (radio_is_short_addr(r)) radio_write_short(((uint8_t)(r)), d); else radio_write_long_ctrl_reg(((uint16_t)(r)), d); } while (0)
#define radio_set_bit(r, b)       radio_write(r, radio_read(r) | (1 << (b)))
#define radio_set_bits(r, m)      radio_write(r, radio_read(r) | (m))
#define radio_clear_bit(r, b)     radio_write(r, radio_read(r) & ~(1 << (b)))
#define radio_clear_bits(r, m)    radio_write(r, radio_read(r) & ~(m))
#define radio_write_bits(r, m, b) radio_write(r, (radio_read(r) & ~(m)) | (b));

// frame control | sequence number | address fields (dest PAN ID, dest addr, src addr)
// 2 bytes       | 1 byte          | 6 bytes
#define frameCtrlLength 2
#define seqNumLength 1
#define addrFieldsLength 6
#define mhrLength (frameCtrlLength + seqNumLength + addrFieldsLength)

typedef struct radio_interrupt_flags {
    volatile uint8_t event : 1;
    volatile uint8_t rx : 1;
    volatile uint8_t tx : 1;
    volatile uint8_t wake : 1;
} radio_if_t;

extern radio_if_t ifs;
extern uint8_t rxBuffer[];
extern uint8_t txBuffer[];
extern uint8_t srcAddrH;
extern uint8_t srcAddrL;
extern uint8_t mhr[]; 

void radio_getIntFlags(void);
void radio_sleep_timed_start(void);
void radio_set_sleep_time(uint32_t ms);
void radio_trigger_tx(void);
void mrf24f40_mhr_write(uint16_t * fifo_i_p);
void mrf24f40_check_txstat(void);
uint8_t radio_read_long_ctrl_reg(uint16_t addr);
uint8_t radio_read_short_ctrl_reg(uint8_t addr);
void radio_write_long_ctrl_reg(uint16_t addr, uint8_t value);
void radio_write_short_ctrl_reg(uint8_t addr, uint8_t value);
void radio_rxfifo_flush(void);
uint8_t radio_get_pending_frame(void);
void radio_hard_reset(void);
void radio_init(void);
void radio_sleep(void);
void radio_wakeup(void);
void radio_set_short_addr(uint8_t *addr);
void radio_set_coordinator_short_addr(uint8_t *addr);
void radio_set_coordinator_eui(uint8_t *eui);
void radio_set_eui(uint8_t *eui);
void radio_set_pan(uint8_t *pan);
void radio_set_key(uint16_t address, uint8_t *key);
#define radio_tx_key(key) radio_set_key(SECKTXNFIFO, key);
#define radio_rx_key(key) radio_set_key(SECKRXFIFO, key);
void radio_set_channel(int16_t ch);
void radio_set_promiscuous(bool crc_check);
void radio_set_coordinator(void);
void radio_clear_coordinator(void);
void radio_txpkt(uint8_t *frame, int16_t hdr_len, int16_t sec_hdr_len, int16_t payload_len);
void radio_set_cipher(uint8_t rxcipher, uint8_t txcipher);
bool radio_rx_sec_fail(void);
uint8_t radio_get_channel(void);
int16_t radio_int_tasks(void);
int16_t radio_rxpkt_intcb(uint8_t *buf, uint8_t *plqi, uint8_t *prssi);
int16_t radio_txpkt_intcb(void);
void radio_sec_intcb(bool accept);
void radio_printAllRegisters(void);
void radio_printTxFifo();

#endif /* MRF24J40_H */

