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
uint16_t payload_adcValue = 0;
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
        .id = 'n', 
        .size = BITS_8, 
        .length = numWordsToLength(sizeof(payload_seqNumString) / sizeof(uint8_t)),
        .data_p = payload_seqNumString
    };
    
    payload_elements[ADC_VALUE_INDEX] = (payloadElement_t){
        .id = 'a', 
        .size = BITS_16,
        .length = numWordsToLength(1),
        .data_p = &payload_adcValue
    };
    
    payload_elements[DATA_32_BIT_INDEX] = (payloadElement_t){
        .id = 'd', 
        .size = BITS_32,
        .length = numWordsToLength(sizeof(payload_data32Bit) / sizeof(uint32_t)),
        .data_p = payload_data32Bit
    };
    
    payload_elements[LAST_ELEMENT_INDEX] = (payloadElement_t){
        .id = 'l', 
        .size = BITS_8,
        .length = numWordsToLength(sizeof(payload_lastElementString) / sizeof(uint8_t)),
        .data_p = payload_lastElementString
    };
    
    payload_length_bits = payloadElementHeaderBits * NUM_ELEMENTS;
    uint16_t element_i;
    for (element_i = 0; element_i < NUM_ELEMENTS; element_i++) {
        payload_length_bits += (sizeToWordLength(payload_elements[element_i].size) * lengthToNumWords(payload_elements[element_i].length));
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

void payload_update(void) {
    payload_seqNumString[3] = '0' + (payload_seqNum / 100) % 10;
    payload_seqNumString[4] = '0' + (payload_seqNum / 10) % 10;
    payload_seqNumString[5] = '0' + payload_seqNum % 10;
    
    payload_adcValue = readAdc();
}

void payload_write() {
    uint16_t fifo_i = TXNFIFO;
    mrf24f40_mhr_write(&fifo_i);
    
    println("Start writing payload at address %d", fifo_i);
    
    uint16_t element_i = 0;
    while (element_i < NUM_ELEMENTS) {
        uint16_t const numWords = lengthToNumWords(payload_elements[element_i].length);
        
        println("payload[%d]: num words = %d, word length = %d bits", element_i, numWords, 1 << (payload_elements[element_i].size + 3));

        // write the element header (total 2 bytes)
        radio_write_fifo(fifo_i++, payload_elements[element_i].id);
        radio_write_fifo(fifo_i++, (((uint8_t)(payload_elements[element_i].size)) << 6) | payload_elements[element_i].length);

        // write the element payload (total ((length + 1) * 2^(size + 3) / 8) bytes)
        uint16_t word_i = 0;
        while (word_i < numWords) {
            switch (payload_elements[element_i].size) { // allow fallthrough to save lines of code
                case BITS_64: { // 8 writes are required 
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint64_t *)(payload_elements[element_i].data_p))[word_i] >> 56)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint64_t *)(payload_elements[element_i].data_p))[word_i] >> 48)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint64_t *)(payload_elements[element_i].data_p))[word_i] >> 40)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint64_t *)(payload_elements[element_i].data_p))[word_i] >> 32)));
                }
                case BITS_32: { // 4 writes are required 
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint32_t *)(payload_elements[element_i].data_p))[word_i] >> 24)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint32_t *)(payload_elements[element_i].data_p))[word_i] >> 16)));
                }
                case BITS_16: { // 2 writes are required 
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint16_t *)(payload_elements[element_i].data_p))[word_i] >> 8)));
                }
                case BITS_8: { // 1 write is required
                    radio_write_fifo(fifo_i++, ((uint8_t *)(payload_elements[element_i].data_p))[word_i]);
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

void payload_read(void) {
    radio_set_bit(BBREG1, 2); // RXDECINV = 1, disable receiving packets off air.

    uint16_t fifo_i = RXFIFO;
    uint8_t frameLength = radio_read_fifo(fifo_i++);
    uint16_t rxPayloadLength = frameLength - 2; // -2 for the FCS bytes

    uint16_t const fifoEnd = fifo_i + rxPayloadLength;
    uint16_t buf_i = 0;
    while (fifo_i < fifoEnd) {
        rxBuffer[buf_i] = radio_read_fifo(fifo_i);
        
        buf_i++;
        fifo_i++;
    }

    uint8_t const fcsL = radio_read_fifo(fifo_i++);
    uint8_t const fcsH = radio_read_fifo(fifo_i++);
    uint8_t const lqi = radio_read_fifo(fifo_i++);
    uint8_t const rssi = radio_read_fifo(fifo_i++);  

    radio_set_bit(RXFLUSH, 0); // reset the RXFIFO pointer, RXFLUSH = 1

    radio_clear_bit(BBREG1, 2); // RXDECINV = 0, enable receiving packets off air.
    
    printf("RX payload: mhr = [");    
    buf_i = 0; // reset buffer index
    while (buf_i < mhrLength) {
        if (buf_i < mhrLength - 1) { // mhr
            printf("0x%.2X, ", rxBuffer[buf_i++]);
        } else { // last mhr value
            println("0x%.2X]", rxBuffer[buf_i++]);
        }
    }
    uint16_t element_i = 0;
    while (buf_i < rxPayloadLength) {
        println("- payload element %u -", element_i);
        
        println("id = %c", rxBuffer[buf_i++]);
        
        uint8_t sizeAndLength = rxBuffer[buf_i++];
        payloadElementDataSize_e size = sizeAndLength >> 6;
        uint8_t numWords = lengthToNumWords(sizeAndLength & 0x3F);
        println("word length = %u", sizeToWordLength(size));
        println("number of words = %u", numWords);
        
        uint16_t word_i = 0;
        if (size == BITS_8) {
            uint8_t data_8[numWords];
            
            printf("data = ");
            
            while (word_i < numWords) {
                data_8[word_i] = rxBuffer[buf_i];
                
                payload_printChar(data_8[word_i]);
                
                word_i++;
                buf_i++;
            }
            
            printf("\r\n");
        } else if (size == BITS_16) {
            uint16_t data_16[numWords];
            
            while (word_i < numWords) {
                data_16[word_i] = 
                        (((uint16_t)(rxBuffer[buf_i])) << 8) | 
                        ((uint16_t)(rxBuffer[buf_i + 1]));
                                
                println("data %u = %u", word_i, data_16[word_i]);
                
                word_i++;
                buf_i += 2;
            }
        } else if (size == BITS_32) {
            uint32_t data_32[numWords];
            
            while (word_i < numWords) {
                data_32[word_i] = 
                        (((uint32_t)(rxBuffer[buf_i])) << 24) | 
                        (((uint32_t)(rxBuffer[buf_i + 1])) << 16) | 
                        (((uint32_t)(rxBuffer[buf_i + 2])) << 8) | 
                        ((uint32_t)(rxBuffer[buf_i + 3]));
                
                println("data %u = %lu", word_i, data_32[word_i]);
                
                word_i++;
                buf_i += 4;
            }
        } else if (size == BITS_64) {
            uint64_t data_64[numWords];
            
            while (word_i < numWords) {
                data_64[word_i] = 
                        (((uint64_t)(rxBuffer[buf_i])) << 56) | 
                        (((uint64_t)(rxBuffer[buf_i + 1])) << 48) | 
                        (((uint64_t)(rxBuffer[buf_i + 2])) << 40) | 
                        (((uint64_t)(rxBuffer[buf_i + 3])) << 32) | 
                        (((uint64_t)(rxBuffer[buf_i + 4])) << 24) | 
                        (((uint64_t)(rxBuffer[buf_i + 5])) << 16) | 
                        (((uint64_t)(rxBuffer[buf_i + 6])) << 8) | 
                        ((uint64_t)(rxBuffer[buf_i + 7]));
                
                println("data %u = %llu", word_i, data_64[word_i]);
                
                word_i++;
                buf_i += 8;
            }
        }
        
        element_i++;
    }

    println("FCSH = 0x%.2X", fcsH);
    println("FCSL = 0x%.2X", fcsL);
    println("LQI = %d", lqi);
    println("RSSI = %d", rssi);
}