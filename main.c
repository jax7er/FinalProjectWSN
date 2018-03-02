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
        
    //uint8_t rfctl = mrf24j40_read_short_ctrl_reg(RFCTL);     
    if (rxMode) {
        printf("RX mode\r\n");
        
        mrf24j40_set_promiscuous(0); // set RX error mode, accept all packets, good or bad CRC
        
//        mrf24j40_write_short_ctrl_reg(RFCTL, rfctl | 0x01); // force rx mode
    } else {
        printf("TX mode\r\n");
        
        mrf24j40_write_short_ctrl_reg(SADRH, 0x55); // set source address to be 0101010110101010
        mrf24j40_write_short_ctrl_reg(SADRL, 0xAA);
           
//        mrf24j40_write_short_ctrl_reg(RFCTL, rfctl | 0x06); // perform rf reset and force tx mode
//        delay_us(200);
//        mrf24j40_write_short_ctrl_reg(RFCTL, rfctl & ~(0x04));
//        delay_us(200);
        
        TX_MODE_LED_SetHigh();
    }
    
    char payloadString[] = "SN: 000";
    uint16_t payloadLength = sizeof(payloadString) / sizeof(char); // must be <= 126
    uint8_t seqNum = 0;
    
    while (1) {
        if (rxMode) {
            //delay_ms(1);
            printf("Sleeping...\r\n");
            delay_ms(10);
            Sleep(); // executes the PWRSAV instruction
            printf("Woken from sleep\r\n");
        } else { 
            // frame control | sequence number | address fields (dest PAN ID, dest addr, src addr)
            // 2 bytes       | 1 byte          | 6 bytes
            uint8_t const frameCtrlLength = 2;
            uint8_t const seqNumLength = 1;
            uint8_t const addrFieldsLength = 6;
            uint8_t const mhrLength = frameCtrlLength + seqNumLength + addrFieldsLength;
            uint16_t const totalLength = mhrLength + payloadLength;
            uint8_t frame[totalLength];
            
            uint8_t const srcAddrH = mrf24j40_read_short_ctrl_reg(SADRH);
            uint8_t const srcAddrL = mrf24j40_read_short_ctrl_reg(SADRL);
            uint8_t mhrIndex = 0;
            
            // frame control
            frame[mhrIndex++] = 0x41; // pan ID compression, data frame
            frame[mhrIndex++] = 0x88; // 16 bit addresses, 2003 frame version
            // sequence number
            frame[mhrIndex++] = seqNum;
            // address fields
            frame[mhrIndex++] = 0xFF; // destination PAN ID LSByte (0xFFFF broadcast)
            frame[mhrIndex++] = 0xFF; // MSByte
            frame[mhrIndex++] = 0xFF; // destination address LSByte (0xFFFF broadcast)
            frame[mhrIndex++] = 0xFF; // MSByte
            frame[mhrIndex++] = srcAddrL; // source address LSByte
            frame[mhrIndex++] = srcAddrH; // MSByte
            memcpy(&(frame[mhrIndex]), payloadString, payloadLength); // payload
            
            printf("TXing [%d]+%d bytes: [0x%.2X%.2X, 0x%.2X, 0x%.2X%.2X, 0x%.2X%.2X, 0x%.2X%.2X] \"%s\"\r\n", 
                    mhrLength, payloadLength,
                    frame[0], frame[1], 
                    frame[frameCtrlLength], 
                    frame[frameCtrlLength + seqNumLength], frame[frameCtrlLength + seqNumLength + 1], frame[frameCtrlLength + seqNumLength + 2], frame[frameCtrlLength + seqNumLength + 3], frame[frameCtrlLength + seqNumLength + 4], frame[frameCtrlLength + seqNumLength + 5], 
                    &(frame[mhrLength]));
            
            delay_ms(250);
                        
            // write to TXNFIFO
            mrf24j40_write_long_ctrl_reg(TXNFIFO, mhrLength);
            mrf24j40_write_long_ctrl_reg(TXNFIFO + 1, totalLength);
            uint16_t const fifoStart = TXNFIFO + 2;
            uint16_t const fifoEnd = fifoStart + totalLength;
            uint16_t fifoIndex = fifoStart;
            uint16_t frameIndex = 0;
            while (fifoIndex < fifoEnd) {
                mrf24j40_write_long_ctrl_reg(fifoIndex++, frame[frameIndex++]);
            }           
            
            // read back TXNFIFO  
//            printf("TXNFIFO = \"");
//            uint8_t value;
//            for (fifoAddr = mhrLength + 2; fifoAddr < totalLength + 2; fifoAddr++) {
//              value = mrf24j40_read_long_ctrl_reg(fifoAddr);              
//              if (value) {
//                printf("%c", value);
//              } else {
//                printf("!");
//              }
//            }
//            printf("\"\r\n");
//            delay_ms(10);
            
            mrf24j40_write_short_ctrl_reg(TXNCON, mrf24j40_read_short_ctrl_reg(TXNCON) | TXNTRIG); // trigger transmit
            
            seqNum++;
            payloadString[4] = '0' + (seqNum / 100) % 10;
            payloadString[5] = '0' + (seqNum / 10) % 10;
            payloadString[6] = '0' + seqNum % 10;       
            
            delay_ms(5000);            
        }
    }
    
    return 0;
}

// ISR for the radio INT output pin
void __attribute__ ( ( interrupt, no_auto_psv ) ) _INT1Interrupt(void)
{
    EX_INT1_InterruptDisable();
    
    uint8_t intstat = mrf24j40_read_short_ctrl_reg(INTSTAT);
    
    printf("INTSTAT = 0x%.2X\r\n", intstat);
    
    if (intstat) { // check there is some status
        if (intstat & RXIF) {                    
            mrf24j40_write_short_ctrl_reg(BBREG1, mrf24j40_read_short_ctrl_reg(BBREG1) | RXDECINV); // disable receiving packets off air.

            uint8_t frameLength = mrf24j40_read_long_ctrl_reg(RXFIFO);

            uint16_t const fifoStart = RXFIFO + 1;
            uint16_t const fifoEnd = fifoStart + frameLength + 2;
            uint16_t fifoIndex = fifoStart;
            uint16_t bufferIndex = 0;
            while (fifoIndex < fifoEnd) {
                rxBuffer[bufferIndex++] = mrf24j40_read_long_ctrl_reg(fifoIndex++);
            }

            uint8_t lqi = mrf24j40_read_long_ctrl_reg(fifoIndex++);
            uint8_t rssi = mrf24j40_read_long_ctrl_reg(fifoIndex++);

            mrf24j40_write_short_ctrl_reg(BBREG1, mrf24j40_read_short_ctrl_reg(BBREG1) | ~(RXDECINV)); // enable receiving packets off air.

            printf("RX payload: \"");
            bufferIndex = 0;
            while (bufferIndex < frameLength) {
                printf("%c", rxBuffer[bufferIndex++]);
            }
            printf("\"\r\n");

            printf("LQI = %d\r\n", lqi);
            printf("RSSI = %d\r\n", rssi);
        }
        if (intstat & TXNIF) {
            uint8_t txstat = mrf24j40_read_short_ctrl_reg(TXSTAT);

            if (~txstat & TXNSTAT) { // TXNSTAT == 0 shows a successful transmission
                printf("TX successful, TXSTAT = 0x%.2X\r\n", txstat);
            } else {
                printf("TX failed, TXSTAT = 0x%.2X\r\n", txstat);
            }
        }
        if (intstat & WAKEIF) {
            printf("Sleep over\r\n");
        }
    }
    
    EX_INT1_InterruptFlagClear();
    
    EX_INT1_InterruptEnable();
}