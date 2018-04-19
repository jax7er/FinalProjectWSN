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

void setup(void);
void testing(void);
void modeSelection(void);
void rxLoop(void);
void txLoop(void);

uint8_t rxMode = 1; // default to RX mode
uint8_t moteId = 1; // default to mote 1

int main(void) {
    setup(); // 2 flashes
    testing(); // 2 flashes
    modeSelection(); // 1 flash then 2 for rx or 3 for tx
    
    if (rxMode) {
        rxLoop();
    } else {
        txLoop();
    }
    
    return 0;
}

void setup(void) {
    SYSTEM_Initialize();
    
#if (PRINT_EN == 0)  
    U1MODEbits.UARTEN = 0; // disable UART1
    _LATB6 = 0; // set TX pin of UART1 low so it doesn't back power the FT232RL board
#endif
    
    println("Board init done");
    
    utils_flashLed(1);
    
    radio_init(); 
    println("Radio init done");
     
//    println("RH = %u", sensor_readRh());
//    println("temp = %f", d(sensor_readTemp()));
//    println("pressure = %lu", sensor_readPressure()); 
//    
//    while (1);
    
    utils_flashLed(1);
}

void testing(void) {
    delay_ms(1000);
    
    if (button_down) { // radio speed test if button pressed
        tests_runRadioSpeed(); // does not return
    } else {
        println("Radio speed test skipped");
    }
    
    utils_flashLed(1);    
    delay_ms(1000);
    
    if (button_down) { // perform tests if button not pushed 
        if (tests_runAll()) {
            println("Testing complete");
        } else { 
            println("Testing failed");

            utils_flashLedForever();
        }
    } else {
        println("Testing skipped");
    }
    
    utils_flashLed(1);      
}

void modeSelection(void) {
    delay_ms(1000);
    
    if (button_down) { // select which mote this one is
        srcAddrH = 0x24; // mote 2 has address 0x2468
        srcAddrL = 0x68;
        moteId = 2; // mote 2 has id 2
    }
                
    radio_write(SADRH, srcAddrH); // set source address
    radio_write(SADRL, srcAddrL);
    
    println("Mote ID = %u, address = 0x%.2X%.2X", moteId, srcAddrH, srcAddrL); 
    
    utils_flashLed(1);
    delay_ms(1000);
        
    rxMode = button_up; // rx mode if the button is not pressed (pulled up), tx mode if it is
        
    if (rxMode) {
        println("RX mode");
        delay_ms(10);
        
        //radio_set_promiscuous(0); // accept all packets, good or bad CRC
        
        utils_flashLed(2);
    } else {
        println("TX mode");        
    
        sensor_init();
        println("Sensor init done");
        
        payload_init(); // initialise data and size of payload
        println("Payload init done");
        
        utils_flashLed(3);
    } 
}

void rxLoop(void) {
    uint32_t const numDelay_ms = 30 * 1000;
    timer_restart();
    do {
//        Sleep();
        while (!(button_down || ifs.event || (timer_getTime_ms() > numDelay_ms))) {
            delay_ms(100);
        }
        timer_restart();

        if (ifs.event) {
            radio_getIntFlags();

            if (ifs.rx) {
                ifs.rx = 0;

                LED_Toggle();

                payload_read();

                LED_Toggle();
            }
            if (ifs.wake) {
                ifs.wake = 0;

                println("Radio woke up"); 
            }
        } else { // button_down || (timer_getTime_us() > numDelay_ms)
            while (button_down); // wait for button to be released
            radio_request_readings();
        }
    } while (1);
}

void txLoop(void) {
    do {
//        while (button_down); // wait if pressing the button
//        while (button_up) { // wait for button press or request   
        do {
            radio_sleep_timed(10000);
            
            if (!ifs.event) { // if already an event don't sleep as radio will not generate any more interrupts until INTSTAT is read
                Sleep(); // sleep until interrupt from radio
            }

            if (ifs.event) {
                radio_getIntFlags();

                if (ifs.rx) {
                    ifs.rx = 0;

                    radio_read_rx();

                    if (payload_isReadingsRequest()) {
                        //TODO work out which readings have been requested, 0 is all readings

                        break;
                    }
                } else if (ifs.wake) {
                    ifs.wake = 0;
                    break;
                }
            }
        } while (1);
//        }
//        while (button_down); // wait for button to be released if previously pressed

        payload_update();          

        payload_write();

        // read back TXNFIFO
//        radio_printTxFifo();
//        delay_ms(100); // allow printing to finish

        LED_Toggle();

        radio_trigger_tx(); // trigger transmit

        do {            
            while (!(ifs.event)); // wait for interrupt from radio

            radio_getIntFlags();
        } while (!(ifs.tx));
        ifs.tx = 0; // reset TX flag

        LED_Toggle();

//        radio_check_txstat();

//        delay_ms(2500);
    } while (1);
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