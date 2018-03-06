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
#include "payload.h"

uint8_t rxMode = 1; // default to RX mode
uint8_t moteId = 1; // default to mote 1
uint16_t totalLength = 0;
    
int main(void) {
    printlnAfter(SYSTEM_Initialize(), "board init done");            
    printlnAfter(mrf24j40_initialize(), "radio init done");
    
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
    
    delay_ms(1000);
        
    rxMode = RX_TX_SELECT_GetValue(); // rx mode if the pin is high
        
    if (rxMode) {
        printlnBefore(mrf24j40_set_promiscuous(0), "RX mode"); // accept all packets, good or bad CRC
    } else {
        printlnBefore(TX_MODE_LED_SetHigh(), "TX mode");
        
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
    
    while (1) {
        if (rxMode) {
            printlnBefore(Sleep(), "Sleeping...");
            
            if (ifs.rx) {   
                ifs.rx = 0;
                
                mrf24j40_read_rx();
            }
            if (ifs.wake) {
                ifs.wake = 0;
                
                println("Radio wake up");
            }
        } else {
            uint16_t fifo_i = TXNFIFO;
            mrf24f40_mhr_write(&fifo_i, totalLength);
            payload_write(&fifo_i);
            
            // read back TXNFIFO
            mrf24j40PrintTxFifo(totalLength);
            delay_ms(100); // allow printing to finish
            
            // trigger transmit
            mrf24j40_trigger_tx();
            
            seqNum++;
            sequenceNumberString[3] = '0' + (seqNum / 100) % 10;
            sequenceNumberString[4] = '0' + (seqNum / 10) % 10;
            sequenceNumberString[5] = '0' + seqNum % 10;
            
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