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

uint8_t seqNum = 0;
char sequenceNumberString[] = "SN 000 this is a test message";
char pressureSensorString[] = "Pressure data goes here";
extern uint32_t data32Bit[] = {
    0x12345678,
    0x23456781,
    0x34567812,
    0x45678123,    
    0x56781234,   
    0x67812345,   
    0x78123456,   
    0x81234567
};

payloadElement_t payload[NUM_ELEMENTS] = {
    (payloadElement_t){
        .id = SEQUENCE_NUM_ID, 
        .size = BITS_8, 
        .length = 29, 
        .data = sequenceNumberString
    },
    (payloadElement_t){
        .id = PRESSURE_SENSOR_ID, 
        .size = BITS_8,
        .length = 23, 
        .data = pressureSensorString
    },
    (payloadElement_t){
        .id = DATA_32_BIT_ID, 
        .size = BITS_32,
        .length = 8, 
        .data = data32Bit
    }
};

void payload_write(uint16_t * fifo_i_p) {
    uint16_t element_i = 0;
    while (element_i < NUM_ELEMENTS) {
        println("payload[%d]: num words = %d, size = %d", element_i, payload[element_i].length + 1, payload[element_i].size);

        // write the element header (total 2 bytes)
        mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, payload[element_i].id);
        mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, (((uint8_t)(payload[element_i].size)) << 6) | payload[element_i].length);

        // write the element payload (total ((length + 1) * 2^(size + 3) / 8) bytes)
        uint16_t const numWords = payload[element_i].length + 1;
        uint16_t word_i = 0;
        while (word_i < numWords) {
            switch (payload[element_i].size) { // allow fallthrough to save lines of code
                case BITS_64: { // 8 writes are required 
                    mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 56)));
                    mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 48)));
                    mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 40)));
                    mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, ((uint8_t)(((uint64_t *)(payload[element_i].data))[word_i] >> 32)));
                }
                case BITS_32: { // 4 writes are required 
                    mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, ((uint8_t)(((uint32_t *)(payload[element_i].data))[word_i] >> 24)));
                    mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, ((uint8_t)(((uint32_t *)(payload[element_i].data))[word_i] >> 16)));
                }
                case BITS_16: { // 2 writes are required 
                    mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, ((uint8_t)(((uint16_t *)(payload[element_i].data))[word_i] >> 8)));
                }
                case BITS_8: { // 1 write is required
                    mrf24j40_write_long_ctrl_reg((*fifo_i_p)++, ((uint8_t *)(payload[element_i].data))[word_i]);
                    break;
                }
                default:
                    println("Unknown word size: %d", payload[element_i].size);
            }

            word_i++;
        }

        element_i++;
    }
}