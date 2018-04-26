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
#ifndef UTILS_H
#define	UTILS_H

#include <stdio.h>
#include <stdint.h>

#define PRINT_EN 1
#if (PRINT_EN == 1)
#define println(...) do { printf(__VA_ARGS__); printf("\r\n"); } while (0)
#else
#define println(...) while (0);
#endif

#define range(counter, lessThan) ((counter) = 0; (counter) < (lessThan); (counter)++)
#define for_range(counter, lessThan) uint16_t counter; for range(counter, lessThan)

#define button_down (BUTTON_GetValue() == 0)
#define button_up (BUTTON_GetValue() == 1)

#define c(x) ((char)(x))
#define f(x) ((float)(x))
#define d(x) ((double)(x))
#define u8(x) ((uint8_t)(x))
#define u16(x) ((uint16_t)(x))
#define u32(x) ((uint32_t)(x))
#define u64(x) ((uint64_t)(x))
#define i8(x) ((int8_t)(x))
#define i16(x) ((int16_t)(x))
#define i32(x) ((int32_t)(x))
#define i64(x) ((int64_t)(x))
#define c_p(x) ((char *))(x))
#define f_p(x) ((float *)(x))
#define d_p(x) ((double *)(x))
#define u8_p(x) ((uint8_t *)(x))
#define u16_p(x) ((uint16_t *)(x))
#define u32_p(x) ((uint32_t *)(x))
#define u64_p(x) ((uint64_t *)(x))
#define i8_p(x) ((int8_t *)(x))
#define i16_p(x) ((int16_t *)(x))
#define i32_p(x) ((int32_t *)(x))
#define i64_p(x) ((int64_t *)(x))

void utils_uart1Print(const char * const str);
void utils_flashLed(uint16_t num);
void utils_flashLedForever(void);

#endif	/* UTILS_H */

