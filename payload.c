/*
 * File:   payload.c
 * Author: jmm546
 *
 * Created on March 6, 2018, 3:23 PM
 */

#include "payload.h"
#include "xc.h"
#include "utils.h"
#include "MRF24J40.h"

uint16_t payload_length_bits = 0;
uint16_t payload_totalLength = 0;
uint8_t payload_seqNum = 0;
uint8_t payload_seqNumString[] = "SN 000 this is a test message";
uint8_t payload_pressureSensorString[] = "Pressure data goes here";
uint32_t payload_data32Bit[] = {
    0x12345678,
    0x23456781,
    0x34567812,
    0x45678123,
    0x56781234,
    0x67812345
};
uint8_t payload_lastElementString[] = "Last digit is a two  2";

payloadElement_t payload_elements[NUM_ELEMENTS]; // allocate memory for elements

void payload_init(void) {
    payload_elements[SEQUENCE_NUM_INDEX] = (payloadElement_t){
        .id = SEQUENCE_NUM_ID, 
        .size = BITS_8, 
        .length = sizeof(payload_seqNumString) / sizeof(uint8_t),
        .data = payload_seqNumString
    };
    
    payload_elements[PRESSURE_SENSOR_INDEX] = (payloadElement_t){
        .id = PRESSURE_SENSOR_ID, 
        .size = BITS_8,
        .length = sizeof(payload_pressureSensorString) / sizeof(uint8_t),
        .data = payload_pressureSensorString
    };
    
    payload_elements[DATA_32_BIT_INDEX] = (payloadElement_t){
        .id = DATA_32_BIT_ID, 
        .size = BITS_32,
        .length = sizeof(payload_data32Bit) / sizeof(uint32_t),
        .data = payload_data32Bit
    };
    
    payload_elements[LAST_ELEMENT_INDEX] = (payloadElement_t){
        .id = LAST_ELEMENT_ID, 
        .size = BITS_8,
        .length = sizeof(payload_lastElementString) / sizeof(uint8_t),
        .data = payload_lastElementString
    };
    
    payload_length_bits = payloadElementHeaderBits * NUM_ELEMENTS;
    uint16_t element_i;
    for (element_i = 0; element_i < NUM_ELEMENTS; element_i++) {
        payload_length_bits += ((1 << (payload_elements[element_i].size + 3)) * (payload_elements[element_i].length + 1));
    }    
    payload_totalLength = mhrLength + (payload_length_bits / 8);        
    
    println("mhr length = %d", mhrLength);
    println("payload length bits (bytes) = %d (%d)", payload_length_bits, payload_length_bits / 8);
    println("total length = %d", payload_totalLength);
    
    if (payload_totalLength > payload_maxLength) {
        println("Total length too big! Maximum is %d", payload_maxLength);

        toggleLedForever();
    }
}

void payload_write() {
    uint16_t fifo_i = TXNFIFO;
    mrf24f40_mhr_write(&fifo_i);
    
    println("Start writing payload at address %d", fifo_i);
    
    uint16_t element_i = 0;
    while (element_i < NUM_ELEMENTS) {
        println("payload[%d]: num words = %d, word length = %d bits", element_i, payload_elements[element_i].length + 1, 1 << (payload_elements[element_i].size + 3));

        // write the element header (total 2 bytes)
        radio_write_fifo(fifo_i++, payload_elements[element_i].id);
        radio_write_fifo(fifo_i++, (((uint8_t)(payload_elements[element_i].size)) << 6) | payload_elements[element_i].length);

        // write the element payload (total ((length + 1) * 2^(size + 3) / 8) bytes)
        uint16_t const numWords = payload_elements[element_i].length + 1;
        uint16_t word_i = 0;
        while (word_i < numWords) {
            switch (payload_elements[element_i].size) { // allow fallthrough to save lines of code
                case BITS_64: { // 8 writes are required 
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint64_t *)(payload_elements[element_i].data))[word_i] >> 56)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint64_t *)(payload_elements[element_i].data))[word_i] >> 48)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint64_t *)(payload_elements[element_i].data))[word_i] >> 40)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint64_t *)(payload_elements[element_i].data))[word_i] >> 32)));
                }
                case BITS_32: { // 4 writes are required 
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint32_t *)(payload_elements[element_i].data))[word_i] >> 24)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint32_t *)(payload_elements[element_i].data))[word_i] >> 16)));
                }
                case BITS_16: { // 2 writes are required 
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint16_t *)(payload_elements[element_i].data))[word_i] >> 8)));
                }
                case BITS_8: { // 1 write is required
                    radio_write_fifo(fifo_i++, ((uint8_t *)(payload_elements[element_i].data))[word_i]);
                    break;
                }
                default:
                    println("Unknown word size: %d", payload_elements[element_i].size);
            }

            word_i++;
        }

        element_i++;
    }
}