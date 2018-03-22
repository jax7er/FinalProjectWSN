/*
 * File:   payload.c
 * Author: jmm546
 *
 * Created on March 6, 2018, 3:23 PM
 */

#include "payload.h"
#include "xc.h"
#include "utils.h"
#include "radio.h"
#include "sensor.h"

#define _elementIdBits     8
#define _elementSizeBits   2
#define _elementLengthBits 6
#define _elementHeaderBits (_elementIdBits + _elementSizeBits + _elementLengthBits)

#define _maxLength (TXNFIFO_SIZE - 3)

#define _makeElement(i, s, w, d_p) (_element_t){.id = (i), .size = (s), .length = _numWordsToLength(w), .data_p = (d_p)}
#define _len(arr) (sizeof(arr) / sizeof((arr)[0]))
#define _numWordsToLength(w) ((w) - 1)
#define _lengthToNumWords(l) ((l) + 1)
#define _sizeToWordLength(s) ((s) == FLOAT ? 32 : 1 << ((s) + 3))
#define _printChar(c) do { if (payload_isValidChar(c)) printf("%c", (c)); else printf("<%u>", (c)); } while (0)

typedef enum payloadElementId {
    SEQUENCE_NUM_ID = 'n',
    ADC_VALUE_ID = 'a',
    RH_ID = 'h',
    TEMP_ID = 't',
    PRESSURE_ID = 'p',
    TEST_STRING_ID = 's'
} _elementId_e;

typedef enum payloadElementDataSize {
    BITS_8 = 0,
    BITS_16,
    BITS_32,
    FLOAT
} _elementDataSize_e;

// size in bits of payload element = 8 + 2 + 6 + (8 * 2^size * (length + 1)) = 16 + 2^(3 + size) * (length + 1)
typedef struct payloadElementFormat {
    _elementId_e id : _elementIdBits; // up to 256 ids
    _elementDataSize_e size : _elementSizeBits; // number of bits per word
    unsigned int length : _elementLengthBits; // number of words per element - 1, up to length + 1 words
    void * data_p; // pointer to data
} _element_t;

typedef enum payloadElementIndex {
    SEQUENCE_NUM_INDEX = 0,
    ADC_VALUE_INDEX,
    RH_INDEX,
    TEMP_INDEX,
    PRESSURE_INDEX,
    TEST_STRING_INDEX,
    NUM_ELEMENTS
} _elementIndex_e;

typedef union uint32Float_union {
    uint32_t uint32_value;
    float    float_value;
} uint32Float_u;

// externally visible
uint16_t payload_totalLength = 0; 
uint8_t payload_seqNum = 0;

// static
static uint16_t _length_bits = 0;

static _element_t _elements[NUM_ELEMENTS]; // allocate memory for elements
static uint16_t _adcValue = 0;
static uint8_t _rhValue = 0;
static float _tempValue = 0;
static uint32_t _pressureValue = 0;
static uint8_t _testString[] = "Hello World...";

void payload_init(void) {
    _length_bits = _elementHeaderBits * NUM_ELEMENTS;
    
    // build the payload array with elements, each element has an entry and new elements need to be added here
    // _makeElement(id, word length, number of words, pointer to data)
    for_range(e_i, NUM_ELEMENTS) {
        switch (e_i) {
            case SEQUENCE_NUM_INDEX: _elements[e_i] = _makeElement(SEQUENCE_NUM_ID, BITS_8,  1,                 &payload_seqNum); break;       
            case ADC_VALUE_INDEX:    _elements[e_i] = _makeElement(ADC_VALUE_ID,    BITS_16, 1,                 &_adcValue);      break; 
            case RH_INDEX:           _elements[e_i] = _makeElement(RH_ID,           BITS_8,  1,                 &_rhValue);       break;
            case TEMP_INDEX:         _elements[e_i] = _makeElement(TEMP_ID,         FLOAT,   1,                 &_tempValue);     break;  
            case PRESSURE_INDEX:     _elements[e_i] = _makeElement(PRESSURE_ID,     BITS_32, 1,                 &_pressureValue); break;  
            case TEST_STRING_INDEX:  _elements[e_i] = _makeElement(TEST_STRING_ID,  BITS_8,  _len(_testString), _testString);     break;
            default: println("Unknown payload element index: %u", e_i); continue;
        }
        
        _length_bits += _sizeToWordLength(_elements[e_i].size) * _lengthToNumWords(_elements[e_i].length);
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
    _adcValue = sensor_readAdc();    
    _rhValue = sensor_readRh();    
    _tempValue = sensor_readTemp();    
    _pressureValue = sensor_readPressure();
    
    println("adc = %u, rh = %u, temp = %f, pressure = %lu", _adcValue, _rhValue, d(_tempValue), _pressureValue);
}

void payload_write() {
    uint16_t fifo_i = TXNFIFO;
    radio_mhr_write(&fifo_i);
    
//    println("Start writing payload at address %d", fifo_i);
    
    uint16_t element_i = 0;
    while (element_i < NUM_ELEMENTS) {
        uint16_t const numWords = _lengthToNumWords(_elements[element_i].length);
        
//        println("payload[%d]: num words = %d, word length = %d bits", element_i, numWords, 1 << (payload_elements[element_i].size + 3));

        // write the element header (total 2 bytes)
        radio_write_fifo(fifo_i++, _elements[element_i].id);
        radio_write_fifo(fifo_i++, (u8(_elements[element_i].size) << 6) | _elements[element_i].length);

        // write the element payload (total ((length + 1) * 2^(size + 3) / 8) bytes)
        uint16_t word_i = 0;
        void * data_p = _elements[element_i].data_p;
        uint32Float_u uint32Float;
        while (word_i < numWords) {
            switch (_elements[element_i].size) {
                case FLOAT: { // floats are also 32 bits
                    uint32Float.float_value = f_p(data_p)[word_i]; // use union to get the correct bit pattern as a uint32
                    radio_write_fifo(fifo_i++, u8(uint32Float.uint32_value >> 24));
                    radio_write_fifo(fifo_i++, u8(uint32Float.uint32_value >> 16));
                    radio_write_fifo(fifo_i++, u8(uint32Float.uint32_value >> 8));
                    radio_write_fifo(fifo_i++, u8(uint32Float.uint32_value));
                    break;
                }
                case BITS_32: { // 4 writes are required, allow fallthrough to save lines of code
                    radio_write_fifo(fifo_i++, u8(u32_p(data_p)[word_i] >> 24));
                    radio_write_fifo(fifo_i++, u8(u32_p(data_p)[word_i] >> 16));
                }
                case BITS_16: { // 2 writes are required, allow fallthrough to save lines of code
                    radio_write_fifo(fifo_i++, u8(u16_p(data_p)[word_i] >> 8));
                }
                case BITS_8: { // 1 write is required
                    radio_write_fifo(fifo_i++, u8_p(data_p)[word_i]);
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
//    printf("RX payload: mhr = [");    
//    buf_i = 0; // reset buffer index
//    while (buf_i < mhrLength) {
//        if (buf_i < mhrLength - 1) { // mhr
//            printf("0x%.2X, ", rxBuffer[buf_i++]);
//        } else { // last mhr value
//            println("0x%.2X]", rxBuffer[buf_i++]);
//        }
//    }
    buf_i = mhrLength;
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
                        (u16(rxBuffer[buf_i]) << 8) | 
                        u16(rxBuffer[buf_i + 1]);
                                
                println("%u data %u = %u", element_i, word_i, data_16[word_i]);
                
                word_i++;
                buf_i += 2; // 16 bits = 2 bytes
            }
        } else if (size == BITS_32) {
            uint32_t data_32[numWords];
            
            while (word_i < numWords) {
                data_32[word_i] = 
                        (u32(rxBuffer[buf_i]) << 24) | 
                        (u32(rxBuffer[buf_i + 1]) << 16) | 
                        (u32(rxBuffer[buf_i + 2]) << 8) | 
                        u32(rxBuffer[buf_i + 3]);
                
                println("%u data %u = %lu", element_i, word_i, data_32[word_i]);
                
                word_i++;
                buf_i += 4; // 32 bits = 4 bytes
            }
        } else if (size == FLOAT) {
            uint32Float_u data_f[numWords];
            
            while (word_i < numWords) {
                data_f[word_i].uint32_value = 
                        (u32(rxBuffer[buf_i]) << 24) | 
                        (u32(rxBuffer[buf_i + 1]) << 16) | 
                        (u32(rxBuffer[buf_i + 2]) << 8) | 
                        u32(rxBuffer[buf_i + 3]);
                
                println("%u data %u = %f", element_i, word_i, d(data_f[word_i].float_value));
                
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