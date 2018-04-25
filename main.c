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

typedef enum {
    BASE,
    MOTE
} _nodeType_e;

typedef enum {
    TIMED,
    REQUEST,
    STREAM
} _operationMode_e;

typedef enum {
    MANUAL,
    AUTO
} _requestMode_e;

void setup(void);
void testing(void);
void nodeSelection(void);
void baseLoop(void);
void moteLoop(void);

_operationMode_e operationMode = TIMED;
_requestMode_e requestMode = MANUAL;
uint32_t requestDelay_ms = 30000;
uint32_t radioTimedSleep_ms = 1000;

_nodeType_e nodeType = BASE; // default to base
uint8_t moteId = 0; // default to 0 (base)

int main(void) {
    setup(); // 2 flashes
    testing(); // 2 flashes
    nodeSelection(); // 1 flash then 2 for rx or 3 for tx
    
    if (nodeType == BASE) {
        baseLoop();
    } else {
        moteLoop();
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
    delay_ms(700);
    
    if (button_down) { // radio speed test if button pressed
        tests_runRadioSpeed(); // does not return
    } else {
        println("Radio speed test skipped");
    }
    
    utils_flashLed(1);    
    delay_ms(700);
    
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

void nodeSelection(void) {
    delay_ms(700);
        
    nodeType = button_down ? MOTE : BASE; // base if the button is not pressed (pulled up), mote if it is
        
    if (nodeType == BASE) {
        println("Base node, address = 0x%.2X%.2X", srcAddrH, srcAddrL); 
        
        //radio_set_promiscuous(0); // accept all packets, good or bad CRC
        
        utils_flashLed(2);
    } else {
        println("Mote node");        
    
        sensor_init();
        println("Sensor init done");
        
        payload_init(); // initialise data and size of payload
        println("Payload init done");
        
        utils_flashLed(3);
    
        delay_ms(700);

        if (button_up) { // select which mote this one is
            srcAddrH = 0x13; // mote 1 has address 0x1357
            srcAddrL = 0x57;
            moteId = 1; // mote 1 has id 1
        } else {            
            srcAddrH = 0x24; // mote 2 has address 0x2468
            srcAddrL = 0x68;
            moteId = 2; // mote 2 has id 2
        }

        println("ID = %u, address = 0x%.2X%.2X", moteId, srcAddrH, srcAddrL); 

        utils_flashLed(moteId);
    } 

    radio_write(SADRH, srcAddrH); // set source address
    radio_write(SADRL, srcAddrL);
}

void baseLoop(void) {
    if (operationMode == REQUEST && requestMode == AUTO) {
        timer_restart();
    }
    do {
        if (!(ifs.event)) {
//            if (mode == REQUEST) {
                while (!(button_down || ifs.event)) {
                    if ((requestMode == AUTO) && (timer_getTime_ms() > requestDelay_ms)) {
                        break;
                    } else {
                        delay_ms(100);
                    }
                }
                if (requestMode == AUTO) {
                    timer_restart();
                }
//            } else {
//                Sleep();
//            }
        }

        if (ifs.event) {
            radio_getIntFlags();

            if (ifs.rx) {
                ifs.rx = 0;

                LED_Toggle();

                payload_read();

                LED_Toggle();
            }
        } else { // button_down || (timer_getTime_us() > numDelay_ms)
            while (button_down); // wait for button to be released
            radio_request_readings();
        }
    } while (1);
}

void moteLoop() {
    do {
        while (button_down) {
            switch (operationMode) {
                case TIMED:
                    operationMode = REQUEST;
                    utils_flashLed(2);
                    break;
                case REQUEST:
                    operationMode = STREAM;
                    utils_flashLed(3);
                    break;
                case STREAM:
                    operationMode = TIMED;
                    utils_flashLed(1);
                    break;
                default:
                    operationMode = STREAM;
            }
            delay_ms(500);
        }
//        while (button_down); // wait if pressing the button
//        while (button_up) { // wait for button press or request   
        if (operationMode != STREAM) {
            do {
                if (operationMode == TIMED) {
                    radio_sleep_timed(radioTimedSleep_ms);
                }

                if (!ifs.event) { // if already an event don't sleep as radio will not generate any more interrupts until INTSTAT is read
                    Sleep(); // sleep until interrupt from radio
                }

                if (ifs.event) {
                    radio_getIntFlags();

                    if (ifs.rx) {
                        ifs.rx = 0;

                        if (operationMode == REQUEST) {
                            radio_read_rx();

                            if (payload_isReadingsRequest()) {
                                //TODO work out which readings have been requested, 0 is all readings

                                break;
                            }
                        }
                    } else if (ifs.wake) {
                        ifs.wake = 0;
                        
                        if (operationMode == TIMED) {
                            break;
                        }
                    }
                }
            } while (1);
        }
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