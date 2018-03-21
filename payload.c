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
#include "sensor.h"

#define _elementIdBits     8
#define _elementSizeBits   2
#define _elementLengthBits 6
#define _elementHeaderBits (_elementIdBits + _elementSizeBits + _elementLengthBits)

#define _maxLength (TXNFIFO_SIZE - 3)

#define _makeElement(i, s, w, d_p) (_element_t){.id = (i), .size = (s), .length = _numWordsToLength(w), .data_p = (d_p)}
#define _len(arr, bits) (sizeof(arr) / ((bits) / 8))
#define _numWordsToLength(w) ((w) - 1)
#define _lengthToNumWords(l) ((l) + 1)
#define _sizeToWordLength(s) ((s) == FLOAT ? 32 : 1 << ((s) + 3))
#define _printChar(c) do { if (payload_isValidChar(c)) printf("%c", (c)); else printf("<%.2X>", (c)); } while (0)

typedef enum payloadElementDataSize {
    BITS_8 = 0,
    BITS_16,
    BITS_32,
    FLOAT
} _elementDataSize_e;

// size in bits of payload element = 8 + 2 + 6 + (8 * 2^size * (length + 1)) = 16 + 2^(3 + size) * (length + 1)
typedef struct payloadElementFormat {
    uint8_t id; // up to 256 ids
    _elementDataSize_e size : _elementSizeBits; // number of bits per word
    unsigned int length : _elementLengthBits; // number of words per element - 1, up to length + 1 words
    void * data_p; // pointer to data
} _element_t;

typedef enum payloadElementIndex {
    SEQUENCE_NUM_INDEX = 0,
    ADC_VALUE_INDEX,
    RH_INDEX,
    TEMP_INDEX,
    DATA_32_BIT_INDEX,
    NUM_ELEMENTS
} _elementIndex_e;

union uint32Float_union {
    uint32_t uint32_value;
    float    float_value;
} uint32Float;

// externally visible
uint16_t payload_totalLength = 0; 
uint8_t payload_seqNum = 0;

// static
static uint16_t _length_bits = 0;

static _element_t _elements[NUM_ELEMENTS]; // allocate memory for elements
static uint16_t _adcValue = 0;
static uint8_t _rhValue = 0;
static float _tempValue = 0;
static uint32_t _data32[] = {
    0x12345678,
    0x23456781,
    0x34567812,
    0x45678123
};

void payload_init(void) {
    _length_bits = _elementHeaderBits * NUM_ELEMENTS;
    
    // build the payload array with elements, each element has an entry and new elements need to be added here
    // _makeElement(id, word length, number of words, pointer to data)
    for_range(e_i, NUM_ELEMENTS) {
        switch (e_i) {
            case SEQUENCE_NUM_INDEX: _elements[e_i] = _makeElement('n', BITS_8,  1,                    &payload_seqNum); break;       
            case ADC_VALUE_INDEX:    _elements[e_i] = _makeElement('a', BITS_16, 1,                    &_adcValue);      break; 
            case RH_INDEX:           _elements[e_i] = _makeElement('h', BITS_8,  1,                    &_rhValue);       break;
            case TEMP_INDEX:         _elements[e_i] = _makeElement('t', FLOAT,   1,                    &_tempValue);     break;  
            case DATA_32_BIT_INDEX:  _elements[e_i] = _makeElement('d', BITS_32, _len(_data32, 32), _data32);      break;
            default: println("Unknown payload element index: %u", e_i);
        }
        
        _length_bits += (_sizeToWordLength(_elements[e_i].size) * _lengthToNumWords(_elements[e_i].length));
    }
    
    payload_totalLength = mhrLength + (_length_bits / 8);
    
    println("mhr length = %d", mhrLength);
    println("payload length bits (bytes) = %d (%d)", _length_bits, _length_bits / 8);
    println("total length = %d", payload_totalLength);
    
    if (payload_totalLength > _maxLength) {
        println("Total length too big! Maximum is %d", _maxLength);

        utils_flashLedForever();
    }
}

void payload_update(void) {
    uint32_t temp;
    for_range(i, _len(_data32, 32)) {
        temp = _data32[i] << 28;
        _data32[i] = (_data32[i] >> 4) | temp;
    }
    
    _adcValue = sensor_readAdc();
    
    _rhValue = sensor_readRh();
    
    _tempValue = sensor_readTemp();
}

void payload_write() {
    uint16_t fifo_i = TXNFIFO;
    mrf24f40_mhr_write(&fifo_i);
    
//    println("Start writing payload at address %d", fifo_i);
    
    uint16_t element_i = 0;
    while (element_i < NUM_ELEMENTS) {
        uint16_t const numWords = _lengthToNumWords(_elements[element_i].length);
        
//        println("payload[%d]: num words = %d, word length = %d bits", element_i, numWords, 1 << (payload_elements[element_i].size + 3));

        // write the element header (total 2 bytes)
        radio_write_fifo(fifo_i++, _elements[element_i].id);
        radio_write_fifo(fifo_i++, (((uint8_t)(_elements[element_i].size)) << 6) | _elements[element_i].length);

        // write the element payload (total ((length + 1) * 2^(size + 3) / 8) bytes)
        uint16_t word_i = 0;
        void * data_p = _elements[element_i].data_p;
        while (word_i < numWords) {
            switch (_elements[element_i].size) {
                case FLOAT: { // floats are also 32 bits
                    uint32Float.float_value = ((float *)(data_p))[word_i]; // use union to get the correct bit pattern as a uint32
                    radio_write_fifo(fifo_i++, ((uint8_t)(uint32Float.uint32_value >> 24)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(uint32Float.uint32_value >> 16)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(uint32Float.uint32_value >> 8)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(uint32Float.uint32_value)));
                    break;
                }
                case BITS_32: { // 4 writes are required, allow fallthrough to save lines of code
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint32_t *)(data_p))[word_i] >> 24)));
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint32_t *)(data_p))[word_i] >> 16)));
                }
                case BITS_16: { // 2 writes are required, allow fallthrough to save lines of code
                    radio_write_fifo(fifo_i++, ((uint8_t)(((uint16_t *)(data_p))[word_i] >> 8)));
                }
                case BITS_8: { // 1 write is required
                    radio_write_fifo(fifo_i++, ((uint8_t *)(data_p))[word_i]);
                    break;
                }
                default:
                    println("Unknown word size: %d", _elements[element_i].size);
            }

            word_i++;
        }

        element_i++;
    }
    
    println("%u TX", payload_seqNum);
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
    
    println("%u RX", rxBuffer[2]);
    
    // cast data to proper type and print
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
//        println("- payload element %u -", element_i);
        
        println("%u id = %c", element_i, rxBuffer[buf_i++]);
        
        uint8_t sizeAndLength = rxBuffer[buf_i++];
        _elementDataSize_e size = sizeAndLength >> 6;
        uint8_t numWords = _lengthToNumWords(sizeAndLength & 0x3F);
        println("%u word length = %u", element_i, _sizeToWordLength(size));
        println("%u number of words = %u", element_i, numWords);
        
        uint16_t word_i = 0;
        if (size == BITS_8) {
            uint8_t data_8[numWords];
            
            printf("%u data = ", element_i);
            
            while (word_i < numWords) {
                data_8[word_i] = rxBuffer[buf_i];
                
                _printChar(data_8[word_i]);
                
                word_i++;
                buf_i++; // 8 bits = 1 byte
            }
            
            printf("\r\n");
        } else if (size == BITS_16) {
            uint16_t data_16[numWords];
            
            while (word_i < numWords) {
                data_16[word_i] = 
                        (((uint16_t)(rxBuffer[buf_i])) << 8) | 
                        ((uint16_t)(rxBuffer[buf_i + 1]));
                                
                println("%u data %u = <%.2X>", element_i, word_i, data_16[word_i]);
                
                word_i++;
                buf_i += 2; // 16 bits = 2 bytes
            }
        } else if (size == BITS_32) {
            uint32_t data_32[numWords];
            
            while (word_i < numWords) {
                data_32[word_i] = 
                        (((uint32_t)(rxBuffer[buf_i])) << 24) | 
                        (((uint32_t)(rxBuffer[buf_i + 1])) << 16) | 
                        (((uint32_t)(rxBuffer[buf_i + 2])) << 8) | 
                        ((uint32_t)(rxBuffer[buf_i + 3]));
                
                println("%u data %u = <%.4X>", element_i, word_i, data_32[word_i]);
                
                word_i++;
                buf_i += 4; // 32 bits = 4 bytes
            }
        } else if (size == FLOAT) {
            float data_f[numWords];
            
            while (word_i < numWords) {
                uint32Float.uint32_value = 
                        (((uint32_t)(rxBuffer[buf_i])) << 24) | 
                        (((uint32_t)(rxBuffer[buf_i + 1])) << 16) | 
                        (((uint32_t)(rxBuffer[buf_i + 2])) << 8) | 
                        ((uint32_t)(rxBuffer[buf_i + 3]));
                data_f[word_i] = uint32Float.float_value;
                
                println("%u data %u = %f", element_i, word_i, data_f[word_i]);
                
                word_i++;
                buf_i += 4; // 32 bits = 4 bytes
            }
        }
        
        element_i++;
    }

    println("FCSH = 0x%.2X", fcsH);
    println("FCSL = 0x%.2X", fcsL);
    println("LQI = %d", lqi);
    println("RSSI = %d", rssi);

    radio_set_bit(RXFLUSH, 0); // reset the RXFIFO pointer, RXFLUSH = 1

    radio_clear_bit(BBREG1, 2); // RXDECINV = 0, enable receiving packets off air.
}