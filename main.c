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
        
    if (rxMode) {
        printf("RX mode\r\n");
    } else {
        printf("TX mode\r\n");
        TX_MODE_LED_SetHigh();
    }
    
    char payloadString[] = "ID: 000";
    uint16_t payloadLength = sizeof(payloadString) / sizeof(char); // must be <= 126
    uint16_t id = 0;
    
    while (1) {
        if (rxMode) {
            delay_ms(1);
            //Sleep(); // executes the PWRSAV instruction
        } else {
            printf("Start TX loop\r\n");
            
            delay_ms(5000);
            
            uint8_t frame[2 + payloadLength]; // +2 for MHR
            uint16_t mhr = IEEE_MHR_NO_ACK_NO_SEC_NO_ADDR;
            frame[0] = mhr >> 8;
            frame[1] = mhr & 0xFF;
            memcpy(&(frame[2]), payloadString, payloadLength);
            
            printf("TXing [2]+%d bytes: [0x%x, 0x%x] \"%s\"\r\n", payloadLength, frame[0], frame[1], &(frame[2]));
            
            delay_ms(250);
            
            mrf24j40_txpkt(frame, 2, 0, payloadLength); // write to TXNFIFO
            
//            uint16_t totalLength = 2 + payloadLength;
//            mrf24j40_write_long_ctrl_reg(TXNFIFO, 2);
//            mrf24j40_write_long_ctrl_reg(TXNFIFO + 1, totalLength);
//            const uint16_t txFifoStart = TXNFIFO + 2;
//            uint16_t i;
//            for (i = 0; i < totalLength; i++) {
//                mrf24j40_write_long_ctrl_reg(txFifoStart + i, frame[i]);
//            }
//            mrf24j40_write_short_ctrl_reg(TXNCON, 0x01); // trigger transmit
            
            delay_ms(10);
            
            // read back TXNFIFO  
            printf("TXNFIFO = \"");
            uint16_t fifoOff;
            for (fifoOff = 0; fifoOff < (2 + payloadLength); fifoOff++) {
              txBuffer[fifoOff] = mrf24j40_read_long_ctrl_reg(TXNFIFO + fifoOff);
              if (txBuffer[fifoOff]) {
                printf("%c", txBuffer[fifoOff]);
              } else {
                printf("!");
              }
            }
            printf("\"\r\n");
                        
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
    uint8_t intstat = mrf24j40_read_short_ctrl_reg(INTSTAT);
    
    if (intstat) { // check there is some status
        if (intstat & (RXIF | TXNIF)) { // check if it was an RX or TX interrupt
            if (rxMode) {
                if (intstat & RXIF) {
                    uint8_t lqi, rssi;
                    uint16_t rxLength = mrf24j40_rxpkt_intcb(rxBuffer, &lqi, &rssi);

                    printf("RX payload: \"");
                    uint16_t i;
                    for (i = 0; i < rxLength; i++) {
                        printf("%c", rxBuffer[i]);
                    }
                    printf("\"\r\n");
                }
            } else {
                if (intstat & TXNIF) {
                    uint8_t txstat = mrf24j40_read_short_ctrl_reg(TXSTAT);
                    
                    if (!(txstat & TXNSTAT)) { // TXNSTAT == 0 shows a successful transmission
                        printf("TX successful\r\n");
                    } else {
                        printf("TX failed, retry count exceeded\r\n");
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
}