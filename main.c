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
    
    if (button_down) { // radio speed test if button pressed
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
    } else {
        println("Radio speed test skipped");
    }
    
    delay_ms(1000);
    
    if (button_up) { // perform tests if button not pushed
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
    
    if (button_down) { // select which mote this one is
        srcAddrH = 0x24; // mote 2 has address 0x2468
        srcAddrL = 0x68;
        moteId = 2; // mote 2 has id 2
    }
                
    radio_write(SADRH, srcAddrH); // set source address
    radio_write(SADRL, srcAddrL);
    
    println("Mote ID = %u, address = 0x%.2X%.2X", moteId, srcAddrH, srcAddrL); 
    
    delay_ms(1000);
        
    rxMode = button_up; // rx mode if the button is not pressed (pulled up), tx mode if it is
        
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
            while (button_down); // wait if pressing the button
            
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