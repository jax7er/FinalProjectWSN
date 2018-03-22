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
#include "radio.h"
#include "mcc_generated_files/mcc.h"
#include "interface.h"
#include "tests.h"
#include "payload.h"
#include "sensor.h" 

uint8_t rxMode = 1; // default to RX mode
uint8_t moteId = 1; // default to mote 1

int main(void) {
    SYSTEM_Initialize();
    println("board init done");
    
    radio_init(); 
    println("radio init done");
    
    sensor_init();
    println("sensor init done");
    
//    println("temp = %f", sensor_readTemp());
//    
//    while (1);
            
    delay_ms(1000);
    
    if (!BUTTON_GetValue()) { // perform tests if button pushed
        if (tests_runAll()) {
            println("Testing complete");
        } else { 
            println("Testing failed");

            utils_flashLedForever();
        }
    } else {
        println("Testing skipped");
    }
        
    delay_ms(1000);
    
    if (!BUTTON_GetValue()) { // select which mote this one is
        srcAddrH = 0x24; // mote 2 has address 0x2468
        srcAddrL = 0x68;
        moteId = 2; // mote 2 has id 2
    }
                
    radio_write(SADRH, srcAddrH); // set source address
    radio_write(SADRL, srcAddrL);
    
    println("Mote ID = %u, address = 0x%.2X%.2X", moteId, srcAddrH, srcAddrL); 
    
    delay_ms(1000);
        
    rxMode = BUTTON_GetValue(); // rx mode if the button is not pressed (pulled up), tx mode if it is
        
    if (rxMode) {
        println("RX mode");
        delay_ms(10);
        
        //radio_set_promiscuous(0); // accept all packets, good or bad CRC
    } else {
        println("TX mode");
        
        payload_init(); // initialise data and size of payload
        
        LED_SetHigh();
    } 
    
    while (1) {
        if (rxMode) {
            Sleep();
            
            radio_getIntFlags();
            
            if (ifs.rx) {
                ifs.rx = 0;
                
                payload_read();
            }
            if (ifs.wake) {
                ifs.wake = 0;
                
                println("Radio woke up");
            }
        } else { // TX mode
            while (!BUTTON_GetValue()); // wait if pressing the button
            
            payload_update();          
            
            payload_write();
            
            // read back TXNFIFO
//            radio_printTxFifo();
//            delay_ms(100); // allow printing to finish
            
            LED_Toggle();
            
            radio_trigger_tx(); // trigger transmit
            
            do {
                while (!(ifs.event)); // wait for interrupt    
                
                radio_getIntFlags();
            } while (!(ifs.tx));
            ifs.tx = 0; // reset TX flag
            
            LED_Toggle();
            
//            radio_check_txstat();
            
//            delay_ms(2500);
        }
    }
    
    return 0;
}

// ISR for the radio INT output pin
void __attribute__ ( ( interrupt, no_auto_psv ) ) _INT1Interrupt(void)
{
//    EX_INT1_InterruptDisable();
//    
    ifs.event = 1;
//    
    EX_INT1_InterruptFlagClear();
//    
//    EX_INT1_InterruptEnable();
}