/*
 * File:   tests.c
 * Author: jmm546
 *
 * Created on 05 March 2018, 09:20
 */

#include "tests.h"
#include "xc.h"
#include <stdio.h>
#include "delay.h"
#include "MRF24J40.h"
#include "utils.h"

uint8_t const mockData[] = {2, 45, 84, 78, 144, 240, 255, 254, 10, 50};
uint16_t const mockDataCount = sizeof(mockData) / sizeof(mockData[0]);

uint8_t runAllTests(void) {
    println("Start all tests...");
    
    testId_e test;
    for (test = 0; test < NUM_TESTS; test++) {
        if (!runTest(test)) {
            return 0;
        }
    }
    
    println("All tests succeeded");
    
    return 1;
}

uint8_t runTest(testId_e id) {
    uint16_t mean = 0;
    uint16_t i;
    
    println("Start test, id = %d...", id);
    
    switch (id) {
        case SOFTWARE_TEST_ID_PROCESSING: 
            for (i = 0; i < mockDataCount; i++) {
                mean += mockData[i];
            }
            mean /= mockDataCount;
            
            println("Data mean = %d", mean);
            
            break;
        case SOFTWARE_TEST_ID_SERIAL:
            println("Serial test succeeded");
            
            break;
        case SOFTWARE_TEST_ID_SLEEP:
            println("Sleep for 1 second");
            
            delay_ms(1000);
            
            if (runTest(SOFTWARE_TEST_ID_PROCESSING)) {
                println("Processing complete");
                
                break;
            } else {
                println("Processing failed");
                
                return 0;
            }
        case SYSTEM_TEST_ID_INTERRUPT:
            //TODO
            
            return 1;
        case SYSTEM_TEST_ID_RADIO_READ:
            if (mrf24j40_read_short_ctrl_reg(ORDER)) { // ORDER has POR value of 0xFF
                println("Radio short read successful");
            } else {
                println("Radio short read failed");
                
                return 0;
            }
            
            if (mrf24j40_read_long_ctrl_reg(WAKETIMEL)) { // WAKETIMEL has POR value of 0x0A
                println("Radio long read successful");
            } else {
                println("Radio long read failed");
                
                return 0;
            }
            
            break;
        case SYSTEM_TEST_ID_RADIO_SLEEP:            
            
            
            break;
        case SYSTEM_TEST_ID_TX_RX:
            //TODO
            
            break;
        default:
            println("No such test, id = %d", id);
            return 0;
    }        
    
    println("Test succeeded, id = %d", id);
    
    return 1;
}
