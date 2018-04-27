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

#define _ELEMENT_ID_BITS     8
#define _ELEMENT_SIZE_BITS   2
#define _ELEMENT_LENGTH_BITS 6
#define _ELEMENT_HEADER_BITS (_ELEMENT_ID_BITS + _ELEMENT_SIZE_BITS + _ELEMENT_LENGTH_BITS)

#define _MAX_LENGTH (TXNFIFO_SIZE - 3)

#define _makeElement(i, s, w, d_p) (_element_t){.id = (i), .size = (s), .length = _numWordsToLength(w), .data_p = (d_p)}
#define _len(arr) (sizeof(arr) / sizeof((arr)[0]))
#define _numWordsToLength(w) ((w) - 1)
#define _lengthToNumWords(l) ((l) + 1)
#define _sizeToWordLength(s) ((s) == FLOAT ? 32 : 1 << ((s) + 3))
#if (PRINT_EN == 1)
#define _printChar(c) do { if (payload_isValidChar(c)) printf("%c", (c)); else printf("<%u>", (c)); } while (0)
#else 
#define _printChar(c) while (0)
#endif

typedef enum payloadElementId {
    REQUEST_READINGS_ID = 'r',
    SEQUENCE_NUM_ID =     'n',
    ADC_VALUE_ID =        'a',
    RH_ID =               'h',
    TEMP_ID =             't',
    PRESSURE_ID =         'p',
    TEST_STRING_ID =      's'
} _elementId_e;

typedef enum payloadElementDataSize {
    BITS_8 = 0,
    BITS_16,
    BITS_32,
    FLOAT
} _elementDataSize_e;

// size in bits of payload element = 8 + 2 + 6 + (8 * 2^size * (length + 1)) = 16 + 2^(3 + size) * (length + 1)
typedef struct payloadElementFormat {
    _elementId_e       id :     _ELEMENT_ID_BITS; // up to 256 ids
    _elementDataSize_e size :   _ELEMENT_SIZE_BITS; // number of bits per word
    unsigned int       length : _ELEMENT_LENGTH_BITS; // number of words per element - 1, up to length + 1 words
    void *             data_p; // pointer to data
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
    uint32_t u32;
    float    f;
} uint32Float_u;

// externally visible
payload_t payload = (payload_t){
    .totalLength = 0,
    .seqNum = 0
};

// static
static uint16_t _length_bits = 0;
static _element_t _elements[NUM_ELEMENTS]; // allocate memory for elements
static uint8_t _testString[] = "Hello World...";

static void _printMinimal(uint16_t rxPayloadLength) {
    uint16_t buf_i = MHR_LENGTH;
        
    while (buf_i < rxPayloadLength) {
//        println("- payload element %u -", element_i);
        
//        if (buf_i > mhrLength) printf("|");
        _elementId_e id = radio.rxBuffer[buf_i++];
        printf("|%c:", id);
        
        uint8_t sizeAndLength = radio.rxBuffer[buf_i++];
        uint8_t numWords = _lengthToNumWords(sizeAndLength & 0x3F);
        
        uint16_t word_i = 0;
        uint32Float_u intFloat;
        while (word_i < numWords) {            
            switch (id) {
                case REQUEST_READINGS_ID:
                    println("Reading request");
                    return;
                case SEQUENCE_NUM_ID:
                case RH_ID:
                    printf("%u", radio.rxBuffer[buf_i]);
                    buf_i++;
                    break;
                case ADC_VALUE_ID:
                    printf("%u", 
                        (u16(radio.rxBuffer[buf_i]) << 8) | 
                        u16(radio.rxBuffer[buf_i + 1]));
                        buf_i += 2;
                    break;
                case TEMP_ID:
                    intFloat.u32 = 
                        (u32(radio.rxBuffer[buf_i]) << 24) | 
                        (u32(radio.rxBuffer[buf_i + 1]) << 16) | 
                        (u32(radio.rxBuffer[buf_i + 2]) << 8) | 
                        u32(radio.rxBuffer[buf_i + 3]);
                    printf("%.2f", d(intFloat.f));
                    buf_i += 4;
                    break;
                case PRESSURE_ID: 
                    printf("%lu",  
                        (u32(radio.rxBuffer[buf_i]) << 24) | 
                        (u32(radio.rxBuffer[buf_i + 1]) << 16) | 
                        (u32(radio.rxBuffer[buf_i + 2]) << 8) | 
                        u32(radio.rxBuffer[buf_i + 3]));
                    buf_i += 4;
                    break;
                case TEST_STRING_ID:
                    _printChar(radio.rxBuffer[buf_i]);
                    buf_i++;
                    break;
                default:
                    printf("?");
                    buf_i++;
                    break;
            }

            word_i++;
        }
    }
    
//    printf("\r\n");
}

static void _printVerbose(uint16_t rxPayloadLength) {
    uint16_t buf_i = MHR_LENGTH;
    uint16_t element_i = 0;
    while (buf_i < rxPayloadLength) {
//        println("- payload element %u -", element_i);
        
        println("%u id = %c", element_i, radio.rxBuffer[buf_i++]);
        
        uint8_t sizeAndLength = radio.rxBuffer[buf_i++];
        _elementDataSize_e size = sizeAndLength >> 6;
        uint8_t numWords = _lengthToNumWords(sizeAndLength & 0x3F);
        println("%u word length = %u", element_i, _sizeToWordLength(size));
        println("%u number of words = %u", element_i, numWords);
        
        uint16_t word_i = 0;
        if (size == BITS_8) {
            uint8_t data_8[numWords];
            
            printf("%u data = ", element_i);
            
            while (word_i < numWords) {
                data_8[word_i] = radio.rxBuffer[buf_i];
                
                _printChar(data_8[word_i]);
                
                word_i++;
                buf_i++; // 8 bits = 1 byte
            }
            
            printf("\r\n");
        } else if (size == BITS_16) {
            uint16_t data_16[numWords];
            
            while (word_i < numWords) {
                data_16[word_i] = 
                        (u16(radio.rxBuffer[buf_i]) << 8) | 
                        u16(radio.rxBuffer[buf_i + 1]);
                                
                println("%u data %u = %u", element_i, word_i, data_16[word_i]);
                
                word_i++;
                buf_i += 2; // 16 bits = 2 bytes
            }
        } else if (size == BITS_32) {
            uint32_t data_32[numWords];
            
            while (word_i < numWords) {
                data_32[word_i] = 
                        (u32(radio.rxBuffer[buf_i]) << 24) | 
                        (u32(radio.rxBuffer[buf_i + 1]) << 16) | 
                        (u32(radio.rxBuffer[buf_i + 2]) << 8) | 
                        u32(radio.rxBuffer[buf_i + 3]);
                
                println("%u data %u = %lu", element_i, word_i, data_32[word_i]);
                
                word_i++;
                buf_i += 4; // 32 bits = 4 bytes
            }
        } else if (size == FLOAT) {
            uint32Float_u data_f[numWords];
            
            while (word_i < numWords) {
                data_f[word_i].u32 = 
                        (u32(radio.rxBuffer[buf_i]) << 24) | 
                        (u32(radio.rxBuffer[buf_i + 1]) << 16) | 
                        (u32(radio.rxBuffer[buf_i + 2]) << 8) | 
                        u32(radio.rxBuffer[buf_i + 3]);
                
                println("%u data %u = %f", element_i, word_i, d(data_f[word_i].f));
                
                word_i++;
                buf_i += 4; // 32 bits = 4 bytes
            }
        }
        
        element_i++;
    }

    println("FCSH = 0x%.2X", radio.fcsH);
    println("FCSL = 0x%.2X", radio.fcsL);
    println("LQI = %d", radio.lqi);
    println("RSSI = %d", radio.rssi);
}

void payload_init() {
    _length_bits = _ELEMENT_HEADER_BITS * NUM_ELEMENTS;

    // build the payload array with elements, each element has an entry and new elements need to be added here
    // _makeElement(id, word length, number of words, pointer to data)
    for_range(e_i, NUM_ELEMENTS) {
        switch (e_i) {
            case SEQUENCE_NUM_INDEX: _elements[e_i] = _makeElement(SEQUENCE_NUM_ID, BITS_8,  1,                 &(payload.seqNum));       break;       
            case ADC_VALUE_INDEX:    _elements[e_i] = _makeElement(ADC_VALUE_ID,    BITS_16, 1,                 &(sensor.adcValue));      break; 
            case RH_INDEX:           _elements[e_i] = _makeElement(RH_ID,           BITS_8,  1,                 &(sensor.rhValue));       break;
            case TEMP_INDEX:         _elements[e_i] = _makeElement(TEMP_ID,         FLOAT,   1,                 &(sensor.tempValue));     break;  
            case PRESSURE_INDEX:     _elements[e_i] = _makeElement(PRESSURE_ID,     BITS_32, 1,                 &(sensor.pressureValue)); break;  
            case TEST_STRING_INDEX:  _elements[e_i] = _makeElement(TEST_STRING_ID,  BITS_8,  _len(_testString), _testString);             break;
            default:                 println("Unknown payload element index: %u", e_i);                                                   continue;
        }

        _length_bits += _sizeToWordLength(_elements[e_i].size) * _lengthToNumWords(_elements[e_i].length);
    }

    payload.totalLength = MHR_LENGTH + (_length_bits / 8);

//    println("mhr length = %d", mhrLength);
//    println("payload length bits (bytes) = %d (%d)", _length_bits, _length_bits / 8);
//    println("total length = %d", payload_totalLength);

    if (payload.totalLength > _MAX_LENGTH) {
        println("Total length too big! Maximum is %d", _MAX_LENGTH);

        utils_flashLedForever();
    }
}

void payload_write() {
    uint16_t fifo_i = TXNFIFO;
    radio_mhr_write(&fifo_i);
    
//    println("Start writing payload at address %d", fifo_i);
    
    uint16_t element_i = 0;
    while (element_i < NUM_ELEMENTS) {
        const uint16_t numWords = _lengthToNumWords(_elements[element_i].length);
        
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
                    uint32Float.f = f_p(data_p)[word_i]; // use union to get the correct bit pattern as a uint32
                    radio_write_fifo(fifo_i++, u8(uint32Float.u32 >> 24));
                    radio_write_fifo(fifo_i++, u8(uint32Float.u32 >> 16));
                    radio_write_fifo(fifo_i++, u8(uint32Float.u32 >> 8));
                    radio_write_fifo(fifo_i++, u8(uint32Float.u32));
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
    
    println("%u TX", payload.seqNum);
}

void payload_writeReadingsRequest(void) {
    payload.totalLength = MHR_LENGTH + _ELEMENT_HEADER_BITS + 8;
    uint16_t fifo_i = TXNFIFO;
    radio_mhr_write(&fifo_i);
    
    // write the element header (total 2 bytes)
    radio_write_fifo(fifo_i++, REQUEST_READINGS_ID);
    radio_write_fifo(fifo_i++, (u8(BITS_8) << 6) | 0);
    
    // write data, 0 for all readings
    radio_write_fifo(fifo_i++, 0); 
}

void payload_read(void) {
    const uint16_t rxPayloadLength = radio_read_rx();
    
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
    _printMinimal(rxPayloadLength);
}

uint8_t payload_isReadingsRequest(void) {
    return radio.rxBuffer[MHR_LENGTH] == REQUEST_READINGS_ID;
}