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
            if (radio_read_short_ctrl_reg(ORDER)) { // ORDER has POR value of 0xFF
                println("Radio short read successful");
            } else {
                println("Radio short read failed");
                
                return 0;
            }
            
            if (radio_read_long_ctrl_reg(WAKETIMEL)) { // WAKETIMEL has POR value of 0x0A
                println("Radio long read successful");
            } else {
                println("Radio long read failed");
                
                return 0;
            }
            
            break;
        case SYSTEM_TEST_ID_RADIO_SLEEP:
            println("Setting up radio sleep registers");            
            
            println("SLPCON1 = %X", radio_read_reg(SLPCON1));
            println("WAKETIMEH = %X", radio_read_reg(WAKETIMEH));
            println("WAKETIMEL = %X", radio_read_reg(WAKETIMEL));
            println("RFCON7 = %X", radio_read_reg(RFCON7));
            
            // set up mrf24f40 for sleeping, main counter sleep interval = 1.00352s
            // internal 100kHz clock is selected by default
            radio_write_bits_reg(SLPCON1, 0b00111111, 0x21); // set !CLKOUTEN = 1 and clock divisor to be 2^1 = 2
            //radio_write_reg(WAKETIMEH, 0x00); // set wake time of 98 clock cycles
            radio_write_reg(WAKETIMEL, 0xD2); // set wake time of 210 clock cycles
            radio_write_bits_reg(SLPACK, 0b01111111, 0x42); // set WAKECNT = 66
            //radio_write_bits_reg(RFCON7, 0b11000000, 0x80); // select internal 100kHz clock as SLPCLK, bits [7:6] = 10
            
            //*********************NEED TO SET THESE BITS IN THESE REGISTERS TO SLEEP! NOT WAKETIMEL AND H??****************************
            radio_write_reg(MAINCNT1, 0xFF);
            radio_write_reg(MAINCNT0, 0xFF);
            
            radio_set_bit_reg(SLPCAL2, 4); // start sleep clock calibration            
            delay_us(1); // sleep calibration takes 800ns            
            while (!radio_read_bit_reg(SLPCAL2, 7)); // check sleep calibration is complete
            println("Radio sleep clock calibration done, SLPCLK period = %luns", (((uint32_t)(radio_read_reg(SLPCAL2) & 0x0F)) << 16) | (((uint32_t)(radio_read_reg(SLPCAL1))) << 8) | ((uint32_t)(radio_read_reg(SLPCAL0))));
            
            println("SLPCON1 = %X", radio_read_reg(SLPCON1));
            println("WAKETIMEH = %X", radio_read_reg(WAKETIMEH));
            println("WAKETIMEL = %X", radio_read_reg(WAKETIMEL));
            println("RFCON7 = %X", radio_read_reg(RFCON7));
            
            radio_set_bit_reg(MAINCNT3, 7); // start the radio sleeping
            println("Waiting for radio to wake..."); 
            
            while (!(ifs.wake));
            ifs.wake = 0;
            
            println("Radio woken");
            
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
