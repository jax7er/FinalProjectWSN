/*
 * File:   main.c
 * Author: Jack
 *
 * Created on 18 February 2018, 10:33
 */

#include "xc.h"
#include "delay.h"
#include "utils.h"
#include "radio.h"
#include "mcc_generated_files/mcc.h"
#include "tests.h"
#include "payload.h"
#include "sensor.h" 
#include "node.h"

void setup(void);
void testing(void);
void nodeSelection(void);

int main(void) {
    setup(); // 2 flashes
    testing(); // 2 flashes
    nodeSelection(); // 2 flashes for base or 3+x flashes for mote x
    
    node_run(); // never returns
    
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
        
    node.type = button_down ? MOTE : BASE; // base if the button is not pressed (pulled up), mote if it is
        
    if (node.type == BASE) {
        println("Base node, address = 0x%.2X%.2X", radio.srcAddrH, radio.srcAddrL); 
        
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
            radio.srcAddrH = 0x13; // mote 1 has address 0x1357
            radio.srcAddrL = 0x57;
            node.id = 1; // mote 1 has id 1
        } else {            
            radio.srcAddrH = 0x24; // mote 2 has address 0x2468
            radio.srcAddrL = 0x68;
            node.id = 2; // mote 2 has id 2
        }

        println("ID = %u, address = 0x%.2X%.2X", node.id, radio.srcAddrH, radio.srcAddrL); 

        utils_flashLed(node.id);
    } 

    radio_write(SADRH, radio.srcAddrH); // set source address
    radio_write(SADRL, radio.srcAddrL);
}

// ISR for the radio INT output pin
void __attribute__ ( ( interrupt, no_auto_psv ) ) _INT1Interrupt(void)
{
//    EX_INT1_InterruptDisable();
//    
    radio.ifs.event = 1;
//    
    EX_INT1_InterruptFlagClear();
//    
//    EX_INT1_InterruptEnable();
}