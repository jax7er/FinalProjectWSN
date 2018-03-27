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
    println("Board init done");
    
    radio_init(); 
    println("Radio init done");
    
//    println("temp = %f", sensor_readTemp());
//    
//    while (1);
            
    delay_ms(1000);
    
    if (BUTTON_GetValue()) { // radio speed test if button not pressed
        println("Radio speed test started");
        
        uint16_t const totalBytes = 1024; // 1KB
        uint16_t const payloadBytes = 1;
        uint16_t const numDummyFrames = totalBytes / payloadBytes;   
        uint16_t const numFinishedFrames = numDummyFrames / 4;    
        
        uint8_t const dummyData = 0xAA;
        uint8_t const finishedData = 0xFF;
        
        delay_ms(1000);
        
        if (!BUTTON_GetValue()) { // transmitter if button pressed
            printf("Transmitter, starting TXs...");            

            uint16_t numSent = 0;
            uint16_t const totalFrameBytes = mhrLength + payloadBytes;
            uint8_t mhr_i;
            uint16_t fifo_i;
            
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
                if (numSent < numDummyFrames) {
                    while (fifo_i < totalFrameBytes) {
                        radio_write_fifo(fifo_i++, dummyData);
                    }
                } else {
                    while (fifo_i < totalFrameBytes) {
                        radio_write_fifo(fifo_i++, finishedData);
                    }
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
            } while (numSent <= (numDummyFrames + numFinishedFrames));
            
            println("done");
        } else { // receiver if button not pressed
            printf("Receiver, waiting for TXs to complete...");
            
            uint16_t numReceived = 0;
            uint8_t lqis[numDummyFrames];
            uint8_t rssis[numDummyFrames];
            float timeTaken_us = 1;            
            
            do {
                while (!(ifs.event || !BUTTON_GetValue()));
                
                if (!BUTTON_GetValue()) { // if button pressed end of transmission
                    break;
                }

                radio_getIntFlags();

                if (ifs.rx) {
                    ifs.rx = 0;
                    
                    if (numReceived == 0) { // first packet so start timing 
                        timer_restart();
                    }
                    
                    timeTaken_us = timer_getTime_us();

                    LED_Toggle();

                    radio_set_bit(BBREG1, 2); // RXDECINV = 1, disable receiving packets off air.

                    uint16_t fifo_i = RXFIFO;
                    uint8_t frameLength = radio_read_fifo(fifo_i++);
                    uint16_t rxPayloadLength = frameLength - 2; // -2 for the FCS bytes

                    uint16_t const fifoEnd = fifo_i + rxPayloadLength;
                    uint16_t buf_i = 0;
                    while (fifo_i < fifoEnd) {
                        rxBuffer[buf_i++] = radio_read_fifo(fifo_i++);
                    }

                    radio_read_fifo(fifo_i++); // fcsL
                    radio_read_fifo(fifo_i++); // fcsH
                    
                    lqis[numReceived] = radio_read_fifo(fifo_i++);
                    rssis[numReceived] = radio_read_fifo(fifo_i++);  

                    radio_set_bit(RXFLUSH, 0); // reset the RXFIFO pointer, RXFLUSH = 1

                    radio_clear_bit(BBREG1, 2); // RXDECINV = 0, enable receiving packets off air.

                    LED_Toggle();
                    
                    numReceived++;
                    
//                    println("%u %u %u %u", rxBuffer[1], rxBuffer[2], rxBuffer[4], numReceived);
                }
            } while (rxBuffer[mhrLength] != finishedData); // break when finished data is received
            
            timer_stop();
            
            uint32_t averageLqi = 0;
            uint32_t averageRssi = 0;
            for_range(i, numReceived) {
                averageLqi += lqis[i];
                averageRssi += rssis[i];
            }
            averageLqi /= numReceived;
            averageRssi /= numReceived;
        
            println("done");
            println("%u/%u=%lu%% in %.0fus @ %.0fB/s with avg lqi=%u rssi=%u", 
                    numReceived, 
                    numDummyFrames,  
                    (u32(numReceived) * 100UL) / u32(numDummyFrames), 
                    d(timeTaken_us), 
                    d(f(numReceived) / timeTaken_us), 
                    u8(averageLqi), 
                    u8(averageRssi));
        }
        
        println("Waiting for reset...");
        
        while (1); // wait for reset
    } else {
        println("Radio speed test skipped");
    }
    
    delay_ms(1000);
    
    if (BUTTON_GetValue()) { // perform tests if button not pushed
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
    
        sensor_init();
        println("Sensor init done");
        
        payload_init(); // initialise data and size of payload
        println("Payload init done");
        
        LED_SetHigh();
    } 
    
    while (1) {
        if (rxMode) {
            Sleep();
            
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