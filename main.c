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
uint8_t volatile adcDoneFlag = 0;
    
int main(void) {
    SYSTEM_Initialize();
    println("board init done");       
    
    radio_initialize();
    println("radio init done");
    
    delay_ms(1000);
    
    if (BUTTON_GetValue()) { // select which mote this one is
        // mote 1 has address 0xAA54 and id 1
        println("Set mote 1, ID = 1, address = 0xAA54"); 
    } else {
        srcAddrL = 0x55; // mote 2 has address 0xAA55
        moteId = 2; // mote 2 has id 2
        println("Set mote 2, ID = 2, address = 0xAA55");
    }
        
    radio_write(SADRH, srcAddrH); // set source address
    radio_write(SADRL, srcAddrL);
    
    delay_ms(1000);
    
    if (BUTTON_GetValue()) { // perform tests by default
        if (runAllTests()) {
            println("Testing complete");
        } else { 
            println("Testing failed");

            toggleLedForever();
        }
    } else {
        println("Testing cancelled");
    }
    
    delay_ms(1000);
        
    rxMode = BUTTON_GetValue(); // rx mode if the button is not pressed (pulled up), tx mode if it is
        
    if (rxMode) {
        println("RX mode");
        
        radio_set_promiscuous(0); // accept all packets, good or bad CRC
    } else {
        println("TX mode");
        
        payload_init(); // initialise data and size of payload
        ADC1_ChannelSelect(ADC1_CHANNEL_AN9); // select channel 9 for ADC
        
        LED_SetHigh();
    } 
    
    while (1) {
        if (rxMode) {
            println("Sleeping...");
            delay_ms(10);
            
            Sleep();
            
            if (ifs.rx) {
                ifs.rx = 0;
                println("Message received");
                
                radio_read_rx();
            }
            if (ifs.wake) {
                ifs.wake = 0;
                println("Radio woke up");
            }
        } else {
            ADC1_Start();
            
            while (!adcDoneFlag);
            adcDoneFlag = 0;
            
            println("ADC done");
            
            while (1);
            
            payload_write();
            
            // read back TXNFIFO
//            mrf24j40PrintTxFifo(payload_totalLength);
//            delay_ms(100); // allow printing to finish
            
            radio_trigger_tx(); // trigger transmit
            
            while (!(ifs.tx)); // wait for TX interrupt                
            ifs.tx = 0; // reset TX flag
            
            mrf24f40_check_txstat();
            
            payload_seqNum++;
            payload_seqNumString[3] = '0' + (payload_seqNum / 100) % 10;
            payload_seqNumString[4] = '0' + (payload_seqNum / 10) % 10;
            payload_seqNumString[5] = '0' + payload_seqNum % 10;
            
            delay_ms(2500);
        }
    }
    
    return 0;
}

// ISR for the radio INT output pin
void __attribute__ ( ( interrupt, no_auto_psv ) ) _INT1Interrupt(void)
{
    EX_INT1_InterruptDisable();
    
    uint8_t intstat = radio_read(INTSTAT);
    
    println("INTSTAT = 0x%.2X", intstat);
    
    ifs.rx = (intstat & RXIF) != 0;
    ifs.tx = (intstat & TXNIF) != 0;
    ifs.wake = (intstat & WAKEIF) != 0;
    
    EX_INT1_InterruptFlagClear();
    
    EX_INT1_InterruptEnable();
}

// ISR for the ADC completion
void __attribute__ ( ( __interrupt__ , auto_psv ) ) _ADC1Interrupt ( void )
{
    adcDoneFlag = 1;
    
    // clear the ADC interrupt flag
    IFS0bits.AD1IF = false;
}