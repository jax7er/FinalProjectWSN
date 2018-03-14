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

uint8_t tests_runAll(void) {
    println("Start all tests...");
    
    testId_e test;
    for (test = 0; test < NUM_TESTS; test++) {
        if (!tests_run(test)) {
            return 0;
        }
    }
    
    println("All tests succeeded");
    
    return 1;
}

uint8_t tests_run(testId_e id) {
    uint16_t mean = 0;
    uint16_t i;
    uint32_t const desiredSleepTime_ms = 1000; // 1s
    
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
            
            if (tests_run(SOFTWARE_TEST_ID_PROCESSING)) {
                println("Processing complete");
                
                break;
            } else {
                println("Processing failed");
                
                return 0;
            }
        case SYSTEM_TEST_ID_INTERRUPT:
            radio_set_sleep_time(desiredSleepTime_ms);
                        
            radio_sleep_timed_start();
            
            println("Sleeping MCU...");
            delay_ms(10);
            
            Sleep();
            
            if (ifs.wake) {
                ifs.wake = 0;
            
                println("Radio woken");
                
                break;
            } else {
                println("Interrupt not from radio wake");
                
                return 0;
            }            
        case SYSTEM_TEST_ID_RADIO_READ:
            if (radio_read(ORDER)) { // ORDER has POR value of 0xFF
                println("Radio short read successful");
            } else {
                println("Radio short read failed");
                
                return 0;
            }
            
            if (radio_read(WAKETIMEL)) { // WAKETIMEL has POR value of 0x0A
                println("Radio long read successful");
            } else {
                println("Radio long read failed");
                
                return 0;
            }
            
            break;
        case SYSTEM_TEST_ID_RADIO_SLEEP:                        
            radio_set_sleep_time(desiredSleepTime_ms);
                        
            radio_sleep_timed_start();
            
            println("Waiting for radio to wake after ~%lums...", desiredSleepTime_ms); 
            
            while (!(ifs.wake)); 
            ifs.wake = 0;
            
            println("Radio woken");
            
            break;
        default:
            println("No such test, id = %d", id);
            return 0;
    }        
    
    println("Test succeeded, id = %d", id);
    
    return 1;
}
