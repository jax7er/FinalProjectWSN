/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef PAYLOAD_H
#define	PAYLOAD_H

#include <xc.h> // include processor files - each processor file is guarded.  

#define isValidPayloadChar(value) (((value) >= '0' && (value) <= '9') \
        || ((value) >= 'a' && (value) <= 'z') \
        || ((value) >= 'A' && (value) <= 'Z') \
        || (value) == ' ')
#define numWordsToLength(w) ((w) - 1)
#define lengthToNumWords(l) ((l) + 1)
#define sizeToWordLength(s) (1 << ((s) + 3))

#define payload_printChar(c) do { if (isValidPayloadChar(c)) printf("%c", (c)); else printf("<%.2X>", (c)); } while (0)

#define payloadElementIdBits     8
#define payloadElementSizeBits   2
#define payloadElementLengthBits 6
#define payloadElementHeaderBits (payloadElementIdBits + payloadElementSizeBits + payloadElementLengthBits)

#define payload_maxLength (TXNFIFO_SIZE - 3)

typedef enum payloadElementDataSize {
    BITS_8 = 0,
    BITS_16,
    BITS_32,
    BITS_64
} payloadElementDataSize_e;

// size in bits of payload element = 8 + 2 + 6 + (8 * 2^size * (length + 1)) = 16 + 2^(3 + size) * (length + 1)
typedef struct payloadElementFormat {
    uint8_t id; // up to 256 ids
    payloadElementDataSize_e size : payloadElementSizeBits; // number of bits per word
    unsigned int length : payloadElementLengthBits; // number of words per element - 1, up to length + 1 words
    void * data_p; // pointer to data
} payloadElement_t;

extern uint16_t payload_totalLength;
extern uint8_t payload_seqNum;

void payload_init(void);
void payload_update(void);
void payload_write(void);
void payload_read(void);

#endif	/* PAYLOAD_H */

