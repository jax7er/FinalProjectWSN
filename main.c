/*
 * File:   main.c
 * Author: Jack
 *
 * Created on 18 February 2018, 10:33
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "xc.h"
#include "delay.h"
#include "utils.h"
#include "MRF24J40.h"
#include "mcc_generated_files/mcc.h"
#include "SPI_functions.h"
#include "tests.h"

#define payloadElementIdBits     8
#define payloadElementSizeBits   2
#define payloadElementLengthBits 6

// size in bits of payload element = 3 + 2 + 5 + (8 * 2^size * (length + 1)) = 10 + 2^(3 + size) * (length + 1)
typedef struct payloadElementFormat {
    enum payloadId_e {
        SEQUENCE_NUM_ID = 'n',
        PRESSURE_SENSOR_ID = 'p'
    } id : payloadElementIdBits; // up to 2^payloadElementIdBits ids
    enum payloadElementDataSize_e {
        BITS_8 = 0,
        BITS_16,
        BITS_32,
        BITS_64
    } size : payloadElementSizeBits; // number of bits per word
    unsigned int length : payloadElementLengthBits; // number of words per element - 1, up to length + 1 words
    void * data; // pointer to data
} payloadElement_t;

enum payloadElementIndex_e {
    SEQUENCE_NUM_INDEX = 0,
    PRESSURE_SENSOR_INDEX,
    NUM_ELEMENTS
};

payloadElement_t payload[NUM_ELEMENTS];

uint8_t rxMode = 1; // default to RX mode
uint8_t seqNum = 0;
uint8_t mhr[mhrLength]; 
uint8_t moteId = 1;
    
int main(void) {
    SYSTEM_Initialize();
        
    println("board init done");      
    
    mrf24j40_initialize();
    
    println("radio init done");
    
    delay_ms(1000);
    
    if (RX_TX_SELECT_GetValue()) { // select which mote this one is
        // mote 1 has address 0xAA54 and id 1
        println("Set mote 1, ID = 1, address = 0xAA54");
    } else {
        srcAddrL = 0x55; // mote 2 has address 0xAA55
        moteId = 2; // mote 2 has id 2
        println("Set mote 2, ID = 2, address = 0xAA55");
    }
        
    mrf24j40_write_short_ctrl_reg(SADRH, srcAddrH); // set source address
    mrf24j40_write_short_ctrl_reg(SADRL, srcAddrL);
    
    delay_ms(1000);
    
    if (RX_TX_SELECT_GetValue()) { // perform tests by default
        if (runAllTests()) {
            println("Testing complete");
        } else { 
            println("Testing failed");

            while (1) { // do not continue if a test fails
                RX_TX_SELECT_Toggle();
                delay_ms(250);
            }; 
        }
    } else {
        println("Testing cancelled");
    }
    //mrf24j40PrintAllRegisters();
    
    delay_ms(1000);
        
    rxMode = RX_TX_SELECT_GetValue(); // rx mode if the pin is high
    
    uint16_t totalLength;
        
    //uint8_t rfctl = mrf24j40_read_short_ctrl_reg(RFCTL);     
    if (rxMode) {
        println("RX mode");
        
        mrf24j40_set_promiscuous(0); // set RX error mode, accept all packets, good or bad CRC
    } else {
        println("TX mode");
        
        TX_MODE_LED_SetHigh();
                
        uint16_t payloadLengthBits = 0;
        uint16_t payloadElementHeaderBits = payloadElementIdBits + payloadElementSizeBits + payloadElementLengthBits;
        uint16_t element_i;
        for (element_i = 0; element_i < NUM_ELEMENTS; element_i++) {
            payloadLengthBits += payloadElementHeaderBits + ((1 << (payload[element_i].size + 3)) * (payload[element_i].length + 1));
        }
        totalLength = mhrLength + (payloadLengthBits / 8);

        println("mhr length = %d", mhrLength);
        println("payload length bits (bytes) = %d (%d)", payloadLengthBits, payloadLengthBits / 8);
        println("total length = %d", totalLength);
    }
    
    // set up payload
    payload[SEQUENCE_NUM_INDEX] = (payloadElement_t){
        .id = SEQUENCE_NUM_ID, 
        .size = BITS_8, 
        .length = 29, 
        .data = "SN 000 this is a test message"
    };
    payload[PRESSURE_SENSOR_INDEX] = (payloadElement_t){
        .id = PRESSURE_SENSOR_ID, 
        .size = BITS_8,
        .length = 23, 
        .data = "Pressure data goes here"
    };
    
    while (1) {
        if (rxMode) {
            //delay_ms(1);
            println("Sleeping...");
            delay_ms(10);
            Sleep(); // executes the PWRSAV instruction
            println("Woken from sleep");
            if (ifs.rx) {   
                ifs.rx = 0;
                
                mrf24j40_read_rx();
            }
            if (ifs.wake) {
                ifs.wake = 0;
                
                println("Radio wake up");
            }
        } else { 
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
            uint16_t fifo_i = TXNFIFO;
            mrf24j40_write_long_ctrl_reg(fifo_i++, mhrLength);
            mrf24j40_write_long_ctrl_reg(fifo_i++, totalLength);
            mhr_i = 0;
            while (mhr_i < mhrLength) {
                mrf24j40_write_long_ctrl_reg(fifo_i++, mhr[mhr_i++]);
            }
            
            uint16_t element_i = 0;
            while (element_i < NUM_ELEMENTS) {
                println("payload[%d]: num words = %d, size = %d", element_i, payload[element_i].length + 1, payload[element_i].size);
                
                // write the element header (total 2 bytes)
                mrf24j40_write_long_ctrl_reg(fifo_i++, payload[element_i].id);
                mrf24j40_write_long_ctrl_reg(fifo_i++, (((uint8_t)(payload[element_i].size)) << 6) | payload[element_i].length);
                
                // write the element payload (total ((length + 1) * 2^(size + 3) / 8) bytes)
                uint16_t const numWords = payload[element_i].length + 1;
                uint16_t word_i = 0;
                while (word_i < numWords) {
                    switch (payload[element_i].size) {
                        case BITS_8: { // 1 write is required
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t *)(payload[element_i].data))[word_i]);
                            break;
                        }
                        case BITS_16: { // 2 writes are required 
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint16_t *)(payload[element_i].data))[word_i] >> 8)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint16_t *)(payload[element_i].data))[word_i])));
                            break;
                        }
                        case BITS_32: { // 4 writes are required 
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint32_t *)(payload[element_i].data))[word_i] >> 24)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint32_t *)(payload[element_i].data))[word_i] >> 16)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint32_t *)(payload[element_i].data))[word_i] >> 8)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint32_t *)(payload[element_i].data))[word_i])));
                            break;
                        }
                        case BITS_64: { // 8 writes are required 
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 56)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 48)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 40)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 32)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 24)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 16)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 8)));
                            mrf24j40_write_long_ctrl_reg(fifo_i++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i])));
                            break;
                        }
                        default:
                            println("Unknown word size: %d", payload[element_i].size);
                    }
                    
                    word_i++;
                }
                
                element_i++;
            }
            
            // read back TXNFIFO  
            printf("TXNFIFO = \"");
            uint8_t value;
            for (fifo_i = 2 + mhrLength; fifo_i < 2 + totalLength; fifo_i++) {
                value = mrf24j40_read_long_ctrl_reg(fifo_i);    
                if ((value >= '0' && value <= '9') 
                        || (value >= 'a' && value <= 'z') 
                        || (value >= 'A' && value <= 'Z') 
                        || value == ' ') {
                    printf("%c", value);
                } else {
                    printf("<%.2X>", value);
                }
            }
            printf("\"\r\n");
            delay_ms(10);
            
            //while (1);
            
            mrf24j40_write_short_ctrl_reg(TXNCON, mrf24j40_read_short_ctrl_reg(TXNCON) | TXNTRIG); // trigger transmit
            
            seqNum++;
            ((char *)(payload[SEQUENCE_NUM_INDEX].data))[3] = '0' + (seqNum / 100) % 10;
            ((char *)(payload[SEQUENCE_NUM_INDEX].data))[4] = '0' + (seqNum / 10) % 10;
            ((char *)(payload[SEQUENCE_NUM_INDEX].data))[5] = '0' + seqNum % 10;
                                    
            while (!(ifs.tx)); // wait for TX interrupt
                
            ifs.tx = 0; // reset TX flag
            mrf24f40_check_txstat();
            
            delay_ms(2500);
        }
    }
    
    return 0;
}

// ISR for the radio INT output pin
void __attribute__ ( ( interrupt, no_auto_psv ) ) _INT1Interrupt(void)
{
    EX_INT1_InterruptDisable();
    
    uint8_t intstat = mrf24j40_read_short_ctrl_reg(INTSTAT);
    
    println("INTSTAT = 0x%.2X", intstat);
    
    ifs.rx = (intstat & RXIF) != 0;
    ifs.tx = (intstat & TXNIF) != 0;
    ifs.wake = (intstat & WAKEIF) != 0;
    
    EX_INT1_InterruptFlagClear();
    
    EX_INT1_InterruptEnable();
}