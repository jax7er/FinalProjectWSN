/*
 * File:   main.c
 * Author: Jack
 *
 * Created on 18 February 2018, 10:33
 */

#include <stdio.h>
#include <string.h>
#include "xc.h"
#include "delay.h"
#include "utils.h"
#include "MRF24J40.h"
#include "mcc_generated_files/mcc.h"
#include "SPI_functions.h"

uint8_t rxMode = 1; // default to RX mode
uint8_t rxBuffer[144]; // RX FIFO is 144 bytes long
uint8_t txBuffer[128]; // TX FIFO is 128 bytes long
    
int main(void) {
    SYSTEM_Initialize();
        
    printf("board init done\r\n");      
    
    mrf24j40_initialize();
    
    printf("radio init done\r\n");
    
    delay_ms(1000);
    
    //mrf24j40PrintAllRegisters();
        
    rxMode = RX_TX_SELECT_GetValue(); // rx mode if the pin is high
        
    uint8_t rfctl = mrf24j40_read_short_ctrl_reg(RFCTL);     
    if (rxMode) {
        printf("RX mode\r\n");
        
        mrf24j40_write_short_ctrl_reg(RFCTL, rfctl | 0x01); // force rx mode
    } else {
        printf("TX mode\r\n");
           
        mrf24j40_write_short_ctrl_reg(RFCTL, rfctl | 0x06); // perform rf reset and force tx mode
        delay_us(200);
        mrf24j40_write_short_ctrl_reg(RFCTL, rfctl & ~(0x04));
        delay_us(200);
        
        TX_MODE_LED_SetHigh();
    }
    
    char payloadString[] = "ID: 000";
    uint16_t payloadLength = sizeof(payloadString) / sizeof(char); // must be <= 126
    uint16_t id = 0;
    
    while (1) {
        if (rxMode) {
            //delay_ms(1);
            printf("Sleeping...\r\n");
            delay_ms(10);
            Sleep(); // executes the PWRSAV instruction
            printf("Woken from sleep\r\n");
        } else {
            printf("Start TX loop\r\n");
            
            delay_ms(5000);
            
            // frame control | sequence number | address fields
            // 2 bytes       | 1 byte          | 4-20 bytes
            
            uint16_t totalLength = 2 + payloadLength;
            uint8_t frame[totalLength]; // +2 for MHR
            uint16_t mhr = IEEE_FRAME_CTRL_NO_ACK_NO_SEC_NO_ADDR;
            frame[0] = ((mhr >> 8) & 0xFF);
            frame[1] = (mhr & 0xFF);
            memcpy(&(frame[2]), payloadString, payloadLength);
            
            printf("TXing [2]+%d bytes: [0x%x, 0x%x] \"%s\"\r\n", payloadLength, frame[0], frame[1], &(frame[2]));
            
            delay_ms(250);
            
            //mrf24j40_txpkt(frame, 2, 0, payloadLength);
            
            // write to TXNFIFO
            mrf24j40_write_long_ctrl_reg(TXNFIFO, 0x02);
            mrf24j40_write_long_ctrl_reg(TXNFIFO + 1, totalLength);
            const uint16_t txFifoPayloadStart = TXNFIFO + 2;
            uint16_t i;
            for (i = 0; i < totalLength; i++) {
                mrf24j40_write_long_ctrl_reg(txFifoPayloadStart + i, frame[i]);
            }           
            
            // read back TXNFIFO  
            printf("TXNFIFO = \"");
            uint16_t fifoOff;
            uint8_t value;
            for (fifoOff = txFifoPayloadStart; fifoOff <= totalLength + 2; fifoOff++) {
              value = mrf24j40_read_long_ctrl_reg(TXNFIFO + fifoOff);              
              if (value) {
                printf("%c", value);
              } else {
                printf("!");
              }
              
            }
            printf("\"\r\n");
            delay_ms(10);
            
            mrf24j40_write_short_ctrl_reg(TXNCON, mrf24j40_read_short_ctrl_reg(TXNCON) | TXNTRIG); // trigger transmit
            
            id++;
            payloadString[4] = '0' + (id / 100) % 10;
            payloadString[5] = '0' + (id / 10) % 10;
            payloadString[6] = '0' + id % 10;
        }
    }
    
    return 0;
}

// ISR for the radio INT output pin
void __attribute__ ( ( interrupt, no_auto_psv ) ) _INT1Interrupt(void)
{
    EX_INT1_InterruptDisable();
    
    uint8_t intstat = mrf24j40_read_short_ctrl_reg(INTSTAT);
    
    if (intstat) { // check there is some status
        if (intstat & (RXIF | TXNIF)) { // check if it was an RX or TX interrupt
            if (rxMode) {
                if (intstat & RXIF) {                    
                    mrf24j40_write_short_ctrl_reg(BBREG1, mrf24j40_read_short_ctrl_reg(BBREG1) | RXDECINV); // disable receiving packets off air.
                    
                    uint8_t frameLength = mrf24j40_read_long_ctrl_reg(RXFIFO);
                    
                    uint16_t const firstRxFifoAddr = RXFIFO + 1;
                    uint16_t const lastRxFifoAddr = RXFIFO + frameLength + 2;
                    uint16_t i;
                    for (i = firstRxFifoAddr; i <= lastRxFifoAddr; i++) {
                        rxBuffer[i - firstRxFifoAddr] = mrf24j40_read_long_ctrl_reg(i);
                    }
                    
                    uint8_t lqi = mrf24j40_read_long_ctrl_reg(i + 1);
                    uint8_t rssi = mrf24j40_read_long_ctrl_reg(i + 2);
                    
                    mrf24j40_write_short_ctrl_reg(BBREG1, mrf24j40_read_short_ctrl_reg(BBREG1) | ~(RXDECINV)); // enable receiving packets off air.
                    
//                    uint16_t rxLength = mrf24j40_rxpkt_intcb(rxBuffer, &lqi, &rssi);
//
                    printf("RX payload: \"");
                    for (i = 0; i < frameLength; i++) {
                        printf("%c", rxBuffer[i]);
                    }
                    printf("\"\r\n");
                    
                    printf("LQI = %d\r\n", lqi);
                    printf("RSSI = %d\r\n", rssi);
                }
            } else {
                if (intstat & TXNIF) {
                    uint8_t txstat = mrf24j40_read_short_ctrl_reg(TXSTAT);
                    
                    if (!(txstat & TXNSTAT)) { // TXNSTAT == 0 shows a successful transmission
                        printf("TX successful\r\n");
                    } else {
                        printf("TX failed\r\n");
                    }
                }
            }
        } else if (intstat & SLPIF) {
            printf("Sleep over\r\n");
        } else {
            printf("INTSTAT = 0x%x\r\n", intstat);
        }
    } else {
        printf("No tasks\r\n");
    }
    
    EX_INT1_InterruptFlagClear();
    
    EX_INT1_InterruptEnable();
}