/*
 * File:   main.c
 * Author: Jack
 *
 * Created on 18 February 2018, 10:33
 */

#include <stdio.h>
#include "xc.h"
#include "delay.h"
#include "utils.h"
#include "MRF24J40.h"
#include "mcc_generated_files/mcc.h"

uint8_t rxMode = 1; // default to RX mode
uint8_t rxBuffer[144]; // RX FIFO is 144 bytes long
uint8_t txBuffer[128]; // TX FIFO is 128 bytes long
    
int main(void) {
    SYSTEM_Initialize();
        
    printf("board init done\r\n");      
    
    mrf24j40_initialize();
    
    printf("radio init done\r\n");
        
    rxMode = RX_TX_SELECT_GetValue(); // rx mode if the pin is high
        
    if (rxMode) {
        printf("RX mode\r\n");
    } else {
        printf("TX mode\r\n");
        TX_MODE_LED_SetHigh();
    }
    
    char payloadString[] = "ID: 000\r\n";
    uint16_t payloadLength = sizeof(payloadString) / sizeof(char); // must be <= 126
    uint16_t id = 0;
    
    while (1) {
        if (rxMode) {
            Sleep(); // executes the PWRSAV instruction
        } else {
            printf("Start TX loop\r\n");
            
            delay_ms(5000);
            
            uint8_t frame[2 + payloadLength]; // +2 for MHR
            uint16_t mhr = IEEE_MHR_NO_ACK_NO_SEC_NO_ADDR;
            frame[0] = mhr >> 8;
            frame[1] = mhr & 0xFF;
            
            printf("TXing %d bytes: [0x%x, 0x%x] %s", payloadLength, frame[0], frame[1], payloadString);
            
            delay_ms(250);
            
            mrf24j40_txpkt(frame, 2, 0, payloadLength);
            
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
    uint8_t stat = mrf24j40_read_short_ctrl_reg(INTSTAT);
    
    if (stat) { // check there is some status
        if (stat & (RXIF | TXNIF)) { // check if it was an RX or TX interrupt
            if (rxMode) {
                if (stat & RXIF) {
                    uint8_t lqi, rssi;
                    uint16_t rxLength = mrf24j40_rxpkt_intcb(rxBuffer, &lqi, &rssi);

                    printf("RX payload: ");
                    uint16_t i;
                    for (i = 0; i < rxLength; i++) {
                        printf("%c", rxBuffer[i]);
                    }
                }
            } else {
                if (stat & TXNIF) {
                    printf("TX complete\r\n");
                }
            }
        } else {
            printf("INTSTAT = 0x%x\r\n", stat);
        }
    } else {
        printf("No tasks\r\n");
    }
    
    EX_INT1_InterruptFlagClear();
}