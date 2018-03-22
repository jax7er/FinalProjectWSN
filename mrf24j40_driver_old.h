///* Microchip Technology Inc. and its subsidiaries.  You may use this software 
// * and any derivatives exclusively with Microchip products. 
// * 
// * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
// * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
// * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
// * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
// * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
// *
// * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
// * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
// * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
// * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
// * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
// * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
// * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
// *
// * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
// * TERMS. 
// */
//
///* 
// * File:   
// * Author: 
// * Comments:
// * Revision history: 
// */
//
//// This is a guard condition so that contents of this file are not included
//// more than once.  
//#ifndef MRF24J40_DRIVER_OLD
//#define	MRF24J40_DRIVER_OLD
//
//#include <xc.h> // include processor files - each processor file is guarded.  
//
//
//uint8_t radio_get_pending_frame(void);
//void radio_sleep(void);
//void radio_wakeup(void);
//void radio_set_short_addr(uint8_t *addr);
//void radio_set_coordinator_short_addr(uint8_t *addr);
//void radio_set_coordinator_eui(uint8_t *eui);
//void radio_set_eui(uint8_t *eui);
//void radio_set_pan(uint8_t *pan);
//void radio_set_key(uint16_t address, uint8_t *key);
//#define radio_tx_key(key) radio_set_key(SECKTXNFIFO, key);
//#define radio_rx_key(key) radio_set_key(SECKRXFIFO, key);
//void radio_set_channel(int16_t ch);
//void radio_set_promiscuous(bool crc_check);
//void radio_set_coordinator(void);
//void radio_clear_coordinator(void);
//void radio_txpkt(uint8_t *frame, int16_t hdr_len, int16_t sec_hdr_len, int16_t payload_len);
//void radio_set_cipher(uint8_t rxcipher, uint8_t txcipher);
//bool radio_rx_sec_fail(void);
//uint8_t radio_get_channel(void);
//int16_t radio_int_tasks(void);
//int16_t radio_rxpkt_intcb(uint8_t *buf, uint8_t *plqi, uint8_t *prssi);
//int16_t radio_txpkt_intcb(void);
//void radio_sec_intcb(bool accept);
//
//#endif	/* MRF24J40_DRIVER_OLD */
//
