///*
// * File:   mrf24j40_driver_old.c
// * Author: jmm546
// *
// * Created on 22 March 2018, 12:38
// */
//
//#include "mrf24j40_driver_old.h"
//
//void radio_ie(void) {
//    radio_write(MRF24J40_INTCON, ~(TXNIE | RXIE | SECIE));
//}
//
//void radio_pwr_reset(void) {
//    radio_write(SOFTRST, RSTPWR);
//}
//
//void radio_bb_reset(void) {
//    radio_write(SOFTRST, RSTBB);
//}
//
//void radio_mac_reset(void) {
//    radio_write(SOFTRST, RSTMAC);
//}
//
//void radio_rf_reset(void) {
//    uint8_t old = radio_read(RFCTL);
//
//    radio_write(RFCTL, old | RFRST);
//    radio_write(RFCTL, old & ~RFRST);
//    delay_ms(2);
//}
//
//uint8_t radio_get_pending_frame(void) {
//    return (radio_read(TXNCON) >> 4) & 0x01;
//}
//
//void radio_set_channel(int16_t ch) {
//    radio_write(RFCON0, CHANNEL(ch) | RFOPT(0x03));
//    radio_rf_reset();
//}
//
//void radio_set_promiscuous(bool crc_check) {
//    uint8_t w = NOACKRSP;
//    if (!crc_check) {
//        w |= ERRPKT;
//    } else {
//        w |= PROMI;
//    }
//
//    radio_write(RXMCR, w);
//}
//
//void radio_set_coordinator(void) {
//    radio_write(RXMCR, radio_read(RXMCR) | PANCOORD);
//}
//
//void radio_clear_coordinator(void) {
//    radio_write(RXMCR, radio_read(RXMCR) & ~PANCOORD);
//}
//
//void radio_set_pan(uint8_t *pan) {
//    radio_write(PANIDL, pan[0]);
//    radio_write(PANIDH, pan[1]);
//}
//
//void radio_set_short_addr(uint8_t *addr) {
//    radio_write(SADRL, addr[0]);
//    radio_write(SADRH, addr[1]);
//}
//
//void radio_set_eui(uint8_t *eui) {
//    radio_write(EADR0, eui[0]);
//    radio_write(EADR1, eui[1]);
//    radio_write(EADR2, eui[2]);
//    radio_write(EADR3, eui[3]);
//    radio_write(EADR4, eui[4]);
//    radio_write(EADR5, eui[5]);
//    radio_write(EADR6, eui[6]);
//    radio_write(EADR7, eui[7]);
//}
//
//void radio_set_coordinator_short_addr(uint8_t *addr) {
//    radio_write(ASSOSADR0, addr[0]);
//    radio_write(ASSOSADR1, addr[1]);
//}
//
//void radio_set_coordinator_eui(uint8_t *eui) {
//    radio_write(ASSOEADR0, eui[0]);
//    radio_write(ASSOEADR1, eui[1]);
//    radio_write(ASSOEADR2, eui[2]);
//    radio_write(ASSOEADR3, eui[3]);
//    radio_write(ASSOEADR4, eui[4]);
//    radio_write(ASSOEADR5, eui[5]);
//    radio_write(ASSOEADR6, eui[6]);
//    radio_write(ASSOEADR7, eui[7]);
//}
//
//void radio_set_key(uint16_t address, uint8_t *key) {
//    radio_spi_preamble();
//    spi_write_long(address, 1);
//
//    int16_t i;
//    for (i = 0; i < 16; i++) {
//        spi_write(key[i]);
//    }
//
//    radio_spi_postamble();
//}
//
//void radio_init(void) {
//    radio_cs_pin(1);
//    radio_wake_pin(1);
//
//    radio_hard_reset();
//
//    radio_write(SOFTRST, (RSTPWR | RSTBB | RSTMAC));
//
//    delay_us(192);
//
//    radio_write(PACON2, FIFOEN | TXONTS(0x18));
//    radio_write(TXSTBL, RFSTBL(9) | MSIFS(5));
//    radio_write(RFCON1, VCOOPT(0x01));
//    radio_write(RFCON2, PLLEN);
//    radio_write(RFCON6, _20MRECVR);
//    radio_write(RFCON7, SLPCLKSEL(0x02));
//    radio_write(RFCON8, RFVCO);
//    radio_write(SLPCON1, SLPCLKDIV(1) | CLKOUTDIS);
//    radio_write(RXFLUSH, (WAKEPAD | WAKEPOL));
//
//    radio_write(BBREG2, CCAMODE(0x02) | CCASTH(0x00));
//    radio_write(CCAEDTH, 0x60);
//    radio_write(BBREG6, RSSIMODE2);
//
//    radio_rxfifo_flush();
//
//    radio_ie();
//}
//
//void radio_sleep(void) {
//    radio_write(WAKECON, IMMWAKE);
//
//    uint8_t r = radio_read(SLPACK);
//    radio_wake_pin(0);
//
//    radio_pwr_reset();
//    radio_write(SLPACK, r | _SLPACK);
//}
//
//void radio_wakeup(void) {
//    radio_wake_pin(1);
//    radio_rf_reset();
//}
//
//void radio_txpkt(uint8_t *frame, int16_t hdr_len, int16_t sec_hdr_len, int16_t payload_len) {
//    int16_t frame_len = hdr_len + sec_hdr_len + payload_len;
//
//    uint8_t w = radio_read(TXNCON);
//    w &= ~(TXNSECEN);
//    w &= ~(TXNACKREQ);
//
//    if (IEEE_802_15_4_HAS_SEC(frame[0])) {
//        w |= TXNSECEN;
//    }
//
//    if (IEEE_802_15_4_WANTS_ACK(frame[0])) {
//        w |= TXNACKREQ;
//    }
//
//    radio_spi_preamble();
//
//    spi_write_long(TXNFIFO, hdr_len);
//    spi_write(frame_len);
//
//    while (frame_len-- > 0) {
//        spi_write(*frame++);
//    }
//
//    radio_spi_postamble();
//
//    //radio_write(TXNCON, w | TXNTRIG);
//}
//
//void radio_set_cipher(uint8_t rxcipher, uint8_t txcipher) {
//    radio_write(SECCON0, RXCIPHER(rxcipher) | TXNCIPHER(txcipher));
//}
//
//bool radio_rx_sec_fail(void) {
//    bool rx_sec_fail = (radio_read(RXSR) >> 2) & 0x01;
//    radio_write(RXSR, 0x00);
//    return rx_sec_fail;
//}
//
//void radio_sec_intcb(bool accept) {
//    uint8_t w = radio_read(SECCON0);
//
//    w |= accept ? SECSTART : SECIGNORE;
//    radio_write(SECCON0, w);
//}
//
//int16_t radio_txpkt_intcb(void) {
//    uint8_t stat = radio_read(TXSTAT);
//    if (stat & TXNSTAT) {
//        if (stat & CCAFAIL) {
//            return EBUSY;
//        } else {
//            return EIO;
//        }
//    } else {
//        return 0;
//    }
//}
//
//int16_t radio_rxpkt_intcb(uint8_t *buf, uint8_t *plqi, uint8_t *prssi) {
//    radio_write(BBREG1, radio_read(BBREG1) | RXDECINV);
//
//    radio_spi_preamble();
//    uint16_t flen = spi_read_long(RXFIFO);
//
//    uint16_t i;
//    for (i = 0; i < flen; i++) {
//        *buf++ = spi_read();
//    }
//
//    uint8_t lqi = spi_read();
//    uint8_t rssi = spi_read();
//
//    if (plqi != NULL) {
//        *plqi = lqi;
//    }
//
//    if (prssi != NULL) {
//        *prssi = rssi;
//    }
//
//    radio_spi_postamble();
//
//    radio_rxfifo_flush();
//    radio_write(BBREG1, radio_read(BBREG1) & ~RXDECINV);
//
//    return flen;
//}
//
//int16_t radio_int_tasks(void) {
//    int16_t ret = 0;
//
//    uint8_t stat = radio_read(INTSTAT);
//
//    if (stat & RXIF) {
//        ret |= MRF24J40_INT_RX;
//    }
//
//    if (stat & TXNIF) {
//        ret |= MRF24J40_INT_TX;
//    }
//
//    if (stat & SECIF) {
//        ret |= MRF24J40_INT_SEC;
//    }
//
//    return ret;
//}
//
