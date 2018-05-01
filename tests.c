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

static const uint8_t mockData[] = {2, 45, 84, 78, 144, 240, 255, 254, 10, 50};
static const uint16_t mockDataCount = sizeof(mockData) / sizeof(mockData[0]);

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
    const uint32_t desiredSleepTime_ms = 1000; // 1s
    
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
            
            if (radio.ifs.wake) {
                radio.ifs.wake = 0;
            
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
            
            while (!(radio.ifs.event)); 
            
            radio_getIntFlags();
            
            if (radio.ifs.wake) {
                radio.ifs.wake = 0;
            
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

    const uint16_t payloadBytes = button_up ? 1 : 128;
    const uint16_t totalBytes = 10240; // 10KB
    const uint16_t numDummyFrames = totalBytes / payloadBytes;
    uint16_t fifo_i;

    println("Payload bytes = %u", payloadBytes);

    delay_ms(1000);

    if (button_down) { // transmitter if button pressed
        const uint16_t totalFrameBytes = MHR_LENGTH + payloadBytes;
        uint16_t numSent;
        uint8_t mhr_i;        
        
        const uint16_t fc = radio_frameControl16Bit2003PanIdCompData;
        // frame control
        radio.mhr[0] = fc & 0xFF;
        radio.mhr[1] = fc >> 8;
        // address fields
        radio.mhr[3] = 0xFF; // destination PAN ID LSByte (0xFFFF broadcast)
        radio.mhr[4] = 0xFF; // MSByte
        radio.mhr[5] = 0xFF; // destination address LSByte (0xFFFF broadcast)
        radio.mhr[6] = 0xFF; // MSByte
        radio.mhr[7] = radio.srcAddrL; // source address LSByte
        radio.mhr[8] = radio.srcAddrH; // MSByte

        while (1) { // test tx forever
            printf("Transmitter, starting TXs...");      

            numSent = 0;            
            
            // sequence number
            radio.mhr[2] = payload.seqNum;

            // write to TXNFIFO
            fifo_i = TXNFIFO;
            radio_write_fifo(fifo_i++, MHR_LENGTH);
            radio_write_fifo(fifo_i++, totalFrameBytes);

            mhr_i = 0;
            while (mhr_i < MHR_LENGTH) {
                radio_write_fifo(fifo_i++, radio.mhr[mhr_i++]);
            }
            while (fifo_i < totalFrameBytes) {
                radio_write_fifo(fifo_i, u8(fifo_i));
                fifo_i++;
            }

            do {
                LED_Toggle();

                radio_trigger_tx();                
                
                // sequence number
                radio.mhr[2] = payload.seqNum;

                // write to TXNFIFO
                fifo_i = TXNFIFO;
                radio_write_fifo(fifo_i++, MHR_LENGTH);
                radio_write_fifo(fifo_i++, totalFrameBytes);
                
                mhr_i = 0;
                while (mhr_i < MHR_LENGTH) {
                    radio_write_fifo(fifo_i++, radio.mhr[mhr_i++]);
                }
                while (fifo_i < totalFrameBytes) {
                    radio_write_fifo(fifo_i, u8(fifo_i));
                    fifo_i++;
                }

                do {
                    while (!(radio.ifs.event)); // wait for interrupt    

                    radio_getIntFlags();
                } while (!(radio.ifs.tx));
                radio.ifs.tx = 0; // reset TX flag

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
        uint8_t lqis[payloadBytes > 1 ? numDummyFrames : numDummyFrames / 8];
        uint8_t rssis[payloadBytes > 1 ? numDummyFrames : numDummyFrames / 8];
        uint16_t numReceived;
        uint16_t rxPayloadLength;
        uint16_t  fifoEnd;
        uint16_t buf_i;

        while (1) { // continue to to rx test forever           
//            printf("Receiver, waiting for TXs to complete...");

            numReceived = 0;
            timer_reset();

            while (1) { // loop until button is pressed
                while (!(radio.ifs.event || button_down));

                if (radio.ifs.event) { 
                    if (numReceived == 0) { // first packet so start timing 
                        timer_start();
                    }
                    
                    // assume the event was an rx
                    radio_getIntFlags(); // read INSTAT register so more interrupts are produced
                } else { // if button pressed end of transmission   
                    break;
                }
                
                fifo_i = RXFIFO;
                buf_i = 0;

                LED_Toggle();

                radio_set_bit(BBREG1, 2); // RXDECINV = 1, disable receiving packets off air.

                rxPayloadLength = radio_read_fifo(fifo_i++);

                fifoEnd = fifo_i + rxPayloadLength;
                while (fifo_i < fifoEnd) { // read all the payload bytes and 2 FCS bytes
                    radio_read_fifo(fifo_i++);
                }

                // calculate recursive mean of lqi and rssi
                radio.lqi = radio_read_fifo(fifo_i++);
                radio.rssi = radio_read_fifo(fifo_i);

//                    radio_set_bit(RXFLUSH, 0); // reset the RXFIFO pointer, RXFLUSH = 1

                radio_clear_bit(BBREG1, 2); // RXDECINV = 0, enable receiving packets off air.

                LED_Toggle();             
                    
                timeTaken_us = timer_getTime_us();
                
                lqis[payloadBytes > 1 ? numReceived : numReceived / 8] = radio.lqi;
                rssis[payloadBytes > 1 ? numReceived : numReceived / 8] = radio.rssi;

                numReceived++;

//                    println("%u %u %u %u", rxBuffer[1], rxBuffer[2], rxBuffer[4], numReceived);
            }

            timer_stop();
            
            float averageLqi = 0.0;
            float averageRssi = 0.0;
            for_range(i, payloadBytes > 1 ? numReceived : numReceived / 8) {
                averageLqi += f(lqis[i]);
                averageRssi += f(rssis[i]);
            }
            averageLqi /= f(payloadBytes > 1 ? numReceived : numReceived / 8);
            averageRssi /= f(payloadBytes > 1 ? numReceived : numReceived / 8);

//            println("done");
            numSeconds = timeTaken_us / 1000000.0;
            println("%u,%u,%u,%lu,%.3f,%.3f,%.1f,%.1f",
                    payloadBytes,
                    numDummyFrames,  
                    numReceived, 
                    (u32(numReceived) * 100UL) / u32(numDummyFrames),
                    d(numSeconds), 
                    d(f(numReceived * payloadBytes) * 8.0 / 1000.0 / numSeconds), 
                    d(averageLqi), 
                    d(averageRssi));
//            println("%u/%u=%lu%% in %.3fs @ %.3fkb/s with avg lqi=%.1f rssi=%.1f", 
//                    numReceived, 
//                    numDummyFrames,  
//                    (u32(numReceived) * 100UL) / u32(numDummyFrames),
//                    d(numSeconds), 
//                    d(f(numReceived * payloadBytes) * 8.0 / 1000.0 / numSeconds), 
//                    d(averageLqi), 
//                    d(averageRssi));

            while (button_down); // wait until button released
        }  
    }
}