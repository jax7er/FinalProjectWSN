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
#include "radio.h"
#include "utils.h"
#include "payload.h"
#include "mcc_generated_files/pin_manager.h"

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
            radio_sleep_timed(desiredSleepTime_ms);
            
            println("Sleeping MCU...");
            
            Sleep();
            
            radio_getIntFlags();
            
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
            radio_sleep_timed(desiredSleepTime_ms);
            
            println("Waiting for radio to wake after ~%lums...", desiredSleepTime_ms); 
            
            while (!(ifs.event)); 
            
            radio_getIntFlags();
            
            if (ifs.wake) {
                ifs.wake = 0;
            
                println("Radio woken");
            } else {
                println("Interrupt not from radio wake");
                
                return 0;
            }
            
            break;
        default:
            println("No such test, id = %d", id);
            return 0;
    }        
    
    println("Test succeeded, id = %d", id);
    
    return 1;
}

void tests_runRadioSpeed(void) {
    println("Radio speed test started");
        
    delay_ms(1000);

    uint16_t const payloadBytes = button_up ? 1 : 128;
    uint16_t const totalBytes = 10240; // 10KB
    uint16_t const numDummyFrames = totalBytes / payloadBytes;
    uint16_t fifo_i;

    println("Payload bytes = %u", payloadBytes);

    delay_ms(1000);

    if (button_down) { // transmitter if button pressed
        uint16_t const totalFrameBytes = mhrLength + payloadBytes;
        uint16_t numSent;
        uint8_t mhr_i;

        while (1) { // test tx forever
            printf("Transmitter, starting TXs...");      

            numSent = 0;

            do {
                mhr_i = 0;
                // frame control
                mhr[mhr_i++] = 0x41; // pan ID compression, data frame
                mhr[mhr_i++] = 0x88; // 16 bit addresses, 2003 frame version
                // sequence number
                mhr[mhr_i++] = payload_seqNum;
                // address fields
                mhr[mhr_i++] = 0xFF; // destination PAN ID LSByte (0xFFFF broadcast)
                mhr[mhr_i++] = 0xFF; // MSByte
                mhr[mhr_i++] = 0xFF; // destination address LSByte (0xFFFF broadcast)
                mhr[mhr_i++] = 0xFF; // MSByte
                mhr[mhr_i++] = srcAddrL; // source address LSByte
                mhr[mhr_i++] = srcAddrH; // MSByte

                // write to TXNFIFO
                fifo_i = TXNFIFO;
                radio_write_fifo(fifo_i++, mhrLength);
                radio_write_fifo(fifo_i++, totalFrameBytes);
                mhr_i = 0;
                while (mhr_i < mhrLength) {
                    radio_write_fifo(fifo_i++, mhr[mhr_i++]);
                }
                while (fifo_i < totalFrameBytes) {
                    radio_write_fifo(fifo_i, u8(fifo_i));
                    fifo_i++;
                }

                LED_Toggle();

                radio_trigger_tx();

                do {
                    while (!(ifs.event)); // wait for interrupt    

                    radio_getIntFlags();
                } while (!(ifs.tx));
                ifs.tx = 0; // reset TX flag

                LED_Toggle();

                numSent++;
            } while (numSent < numDummyFrames);

            println("done");
            println("Waiting for button press");

            while (button_up);
        }
    } else { // receiver if button not pressed
        float timeTaken_us;       
        float numSeconds;   
        float averageLqi;
        float averageRssi;
        uint8_t frameLength;
        uint16_t numReceived;
        uint16_t rxPayloadLength;
        uint16_t  fifoEnd;
        uint16_t buf_i;

        while (1) { // continue to to rx test forever           
            printf("Receiver, waiting for TXs to complete...");

            numReceived = 0;
            timer_reset();

            while (1) { // loop until button is pressed
                while (!(ifs.event || button_down));

                timeTaken_us = timer_getTime_us(); 

                if (button_down) { // if button pressed end of transmission
                    break;
                }

                ifs.event = 0; // assume the event was an rx
                radio_read(INTSTAT); // read INSTAT register so more interrupts are produced

                if (numReceived == 0) { // first packet so start timing 
                    timer_start();
                }

                LED_Toggle();

                radio_set_bit(BBREG1, 2); // RXDECINV = 1, disable receiving packets off air.

                fifo_i = RXFIFO;
                frameLength = radio_read_fifo(fifo_i++);
                rxPayloadLength = frameLength - 2; // -2 for the FCS bytes

                fifoEnd = fifo_i + rxPayloadLength;
                buf_i = 0;
                while (fifo_i < fifoEnd) {
                    rxBuffer[buf_i++] = radio_read_fifo(fifo_i++);
                }

                radio_read_fifo(fifo_i++); // fcsL
                radio_read_fifo(fifo_i++); // fcsH

                // calculate recursive mean of lqi and rssi
                if (numReceived == 0) {
                    averageLqi = radio_read_fifo(fifo_i++);
                    averageRssi = radio_read_fifo(fifo_i++);
                } else {
                    averageLqi += (f(radio_read_fifo(fifo_i++)) - averageLqi) / f(numReceived);
                    averageRssi += (f(radio_read_fifo(fifo_i++)) - averageRssi) / f(numReceived);
                }

//                    radio_set_bit(RXFLUSH, 0); // reset the RXFIFO pointer, RXFLUSH = 1

                radio_clear_bit(BBREG1, 2); // RXDECINV = 0, enable receiving packets off air.

                LED_Toggle();

                numReceived++;

//                    println("%u %u %u %u", rxBuffer[1], rxBuffer[2], rxBuffer[4], numReceived);
            }

            timer_stop();

            println("done");
            numSeconds = timeTaken_us / 1000000.0;
            println("%u/%u=%lu%% in %.3fs @ %.0fB/s with avg lqi=%.1f rssi=%.1f", 
                    numReceived, 
                    numDummyFrames,  
                    (u32(numReceived) * 100UL) / u32(numDummyFrames), 
                    d(numSeconds), 
                    d(f(numReceived * payloadBytes) / numSeconds), 
                    d(averageLqi), 
                    d(averageRssi));

            delay_ms(1000); // delay so button press does not skip next loop
        }  
    }
}