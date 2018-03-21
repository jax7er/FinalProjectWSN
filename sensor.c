/*
 * File:   sensor.c
 * Author: jmm546
 *
 * Created on 14 March 2018, 09:57
 */

#include "sensor.h"
#include "xc.h"
#include "mcc_generated_files/i2c2.h"
#include "utils.h"
#include "delay.h"
#include "interface.h"

// I2C addresses
#define PRESSURE_ADDR 0x77
#define RH_TEMP_ADDR  0x40

// humidity/temperature sensor commands
#define RH_TEMP_RESET          0xFE
#define RH_TEMP_READ_RH        0xF5
#define RH_TEMP_READ_TEMP      0xF3
#define RH_TEMP_READ_LAST_TEMP 0xE0

// pressure sensor registers
#define PRESSURE_MEAS_CFG 0x08 // Sensor Operating Mode and Status
// MEAS_CFG<5> = Temperature measurement ready (TMP_RDY), 1 - New temperature measurement is ready. Cleared when temperature measurement is read.
// MEAS_CFG<4> = Pressure measurement ready (PRS_RDY), 1 - New pressure measurement is ready. Cleared when pressure measurement is read.
// MEAS_CFG<2:0> = Measurement mode and type (MEAS_CTRL), 000 - Idle / Stop background measurement, 001 - Pressure measurement, 010 - Temperature measurement
#define PRESSURE_PSR_B2        0x00 // pressure reading MSB
#define PRESSURE_PSR_B1        0x01
#define PRESSURE_PSR_B0        0x02 // pressure reading LSB
#define PRESSURE_TMP_B2        0x03 // temperature reading MSB
#define PRESSURE_TMP_B1        0x04
#define PRESSURE_TMP_B0        0x05 // temperature reading LSB

// pressure sensor commands
#define PRESSURE_READ_PRESSURE 0x01
#define PRESSURE_READ_TEMP     0x02

// pressure sensor read masks
#define PRESSURE_RESULT_PRESSURE_READY (1 << 4)
#define PRESSURE_RESULT_TEMP_READY     (1 << 5)

#define _readingToTemp(r) (((175.72 * ((float)(r))) / 65536.0) - 46.85)
#define _readingToRh(r)   ((uint8_t)(((125 * (r)) / 65536) - 6))
    
int32_t c00, c10, c01, c11, c20, c21, c30, pressureRaw, tempRaw;
int32_t const pressureScaleFactor = 524288; // from datasheet (Table 9) for single sample

//static void _rhTempSensorWriteCommand(uint8_t cmd) {    
//    i2c_start(); 
//    
//    i2c_tx(i2c_addrWrite(RH_TEMP_ADDR));
//    
//    if (i2c_gotAck()) {
//        i2c_tx(cmd);
//    }
//    
//    i2c_stop();
//}

static uint8_t _pressureSensorRead(uint8_t reg) {    
    uint8_t result = 0;
    
    i2c_start();
    
    i2c_tx(i2c_addrWrite(PRESSURE_ADDR));
    
    if (i2c_gotAck()) {
        i2c_tx(reg);
        
        if (i2c_gotAck()) {
            i2c_repeatedStart();
            
            i2c_tx(i2c_addrRead(PRESSURE_ADDR));
            
            if (i2c_gotAck()) {
                result = i2c_rx();
                
                i2c_sendNack();
            }
        }
    }
    
    i2c_stop();
    
    return result;
}

static void _pressureSensorWrite(uint8_t reg, uint8_t data) {    
    i2c_start();
    
    i2c_tx(i2c_addrWrite(PRESSURE_ADDR));
    
    if (i2c_gotAck()) {
        i2c_tx(reg);
        
        if (i2c_gotAck()) {
            i2c_tx(data);
        }
    }
    
    i2c_stop();
}

static uint16_t _rhTempSensorGetReading(uint8_t cmd) {
    // assert start condition
    i2c_start(); 
    
    // slave address with write
    i2c_tx(i2c_addrWrite(RH_TEMP_ADDR));
    if (i2c_gotNack()) { // expecting ACK = 0
        println("slave addr nack");
        return 0xFF;
    }
    
    // command for humidity read
    i2c_tx(cmd);
    if (i2c_gotNack()) { // expecting ACK = 0
        println("command nack");
        return 0xFF;
    }
    
    if (cmd == RH_TEMP_READ_LAST_TEMP) {
        i2c_repeatedStart();
        i2c_tx(i2c_addrRead(RH_TEMP_ADDR));
        if (i2c_gotNack()) { // expecting ACK = 0
            println("second addr nack");
            return 0xFF;
        }
    } else { // other readings take some time to produce a measurement
        do {
            delay_ms(5);
            i2c_repeatedStart();
            i2c_tx(i2c_addrRead(RH_TEMP_ADDR));
        } while (i2c_gotNack()); // when complete ACK = 0
    }
    
    // begin receiving data
    uint32_t rxValue = 0; // 16 bit value but make 32 bits for ease of calculating %RH
    
    // receive MSByte
    rxValue |= ((uint16_t)(i2c_rx())) << 8;
    i2c_sendAck();
    
    // receive LSByte
    rxValue |= ((uint16_t)(i2c_rx()));
    i2c_sendNack();
    
    // assert stop condition
    i2c_stop();
    
    return rxValue;
}

void sensor_init(void) {
    delay_ms(50); // ensure pressure sensor has definitely had time to start up (>40ms)
    
    // get pressure sensor coefficient values
    c00 = (((uint32_t)(_pressureSensorRead(0x13))) << 12) | (((uint32_t)(_pressureSensorRead(0x14))) << 4) | (((uint32_t)(_pressureSensorRead(0x15))) >> 4);
    c10 = (((uint32_t)(_pressureSensorRead(0x15) & 0x0F)) << 16) | (((uint32_t)(_pressureSensorRead(0x16))) << 8) | ((uint32_t)(_pressureSensorRead(0x17)));
    c01 = (((uint16_t)(_pressureSensorRead(0x18))) << 8) | ((uint16_t)(_pressureSensorRead(0x19)));
    c11 = (((uint16_t)(_pressureSensorRead(0x1A))) << 8) | ((uint16_t)(_pressureSensorRead(0x1B)));
    c20 = (((uint16_t)(_pressureSensorRead(0x1C))) << 8) | ((uint16_t)(_pressureSensorRead(0x1D)));
    c21 = (((uint16_t)(_pressureSensorRead(0x1E))) << 8) | ((uint16_t)(_pressureSensorRead(0x1F)));
    c30 = (((uint16_t)(_pressureSensorRead(0x20))) << 8) | ((uint16_t)(_pressureSensorRead(0x21)));  
    
//    println("pressure coeffs pre 2's complement: %ld, %ld, %ld, %ld, %ld, %ld, %ld", c00, c10, c01, c11, c20, c21, c30);
    
    // 20-bit 2's complement so if bit 19 is set, subtract 2^20
    c00 -= (c00 & (1UL << 19)) ? (1UL << 20) : 0;
    c10 -= (c10 & (1UL << 19)) ? (1UL << 20) : 0;
    
    // 16-bit 2's complement so if bit 15 is set, subtract 2^16
    c01 -= (c01 & (1UL << 15)) ? (1UL << 16) : 0;
    c11 -= (c11 & (1UL << 15)) ? (1UL << 16) : 0;
    c20 -= (c20 & (1UL << 15)) ? (1UL << 16) : 0;
    c21 -= (c21 & (1UL << 15)) ? (1UL << 16) : 0;
    c30 -= (c30 & (1UL << 15)) ? (1UL << 16) : 0;
    
//    println("pressure coeffs: %ld, %ld, %ld, %ld, %ld, %ld, %ld", c00, c10, c01, c11, c20, c21, c30);
    
//    _rhTempSensorWriteCommand(RH_TEMP_ADDR, RH_TEMP_RESET); 
}

uint8_t sensor_readRh(void) { 
    uint32_t reading = _rhTempSensorGetReading(RH_TEMP_READ_RH); // 16 bit value but make 32 bits for ease of calculating %RH    
    return _readingToRh(reading);
}

float sensor_readTemp(void) {
    uint16_t reading = _rhTempSensorGetReading(PRESSURE_READ_TEMP);
    return _readingToTemp(reading);
}

float sensor_readLastTemp(void) {
    uint16_t reading = _rhTempSensorGetReading(RH_TEMP_READ_LAST_TEMP);
    return _readingToTemp(reading);
}

uint32_t sensor_readPressure(void) {
    // 3.6 ms per reading at 5 Pa precision and 1 sample (default)
    // 27.6 ms per reading at 1.2 Pa precision and 16 samples
        
    _pressureSensorWrite(PRESSURE_MEAS_CFG, PRESSURE_READ_PRESSURE);
    while (!(_pressureSensorRead(PRESSURE_MEAS_CFG) | PRESSURE_RESULT_PRESSURE_READY)) {
        delay_ms(2);
    }
    uint32_t pressureReading = (((uint32_t)(_pressureSensorRead(PRESSURE_PSR_B2))) << 16) | (((uint32_t)(_pressureSensorRead(PRESSURE_PSR_B1))) << 8) | ((uint32_t)(_pressureSensorRead(PRESSURE_PSR_B0)));
//    println("pressure reading = %lu", pressureReading);
    pressureRaw = (pressureReading & 0x00FFFFFF);    
//    println("pressure raw pre 2's complement = %ld", pressureRaw);
    // 24-bit 2's complement so if bit 23 is set, subtract 2^24
    pressureRaw -= (pressureReading & (1UL << 23)) ? (1UL << 24) : 0;    
//    println("pressure raw = %ld", pressureRaw);
    
    _pressureSensorWrite(PRESSURE_MEAS_CFG, PRESSURE_READ_TEMP);
    while (!(_pressureSensorRead(PRESSURE_MEAS_CFG) | PRESSURE_RESULT_TEMP_READY)) {
        delay_ms(2);
    }
    uint32_t tempReading = (((uint32_t)(_pressureSensorRead(PRESSURE_TMP_B2))) << 16) | (((uint32_t)(_pressureSensorRead(PRESSURE_TMP_B1))) << 8) | ((uint32_t)(_pressureSensorRead(PRESSURE_TMP_B0)));
//    println("temp reading = %lu", tempReading);
    tempRaw = (tempReading & 0x00FFFFFF);   
//    println("temp raw pre 2's complement = %ld", tempRaw);
    // 24-bit 2's complement so if bit 23 is set, subtract 2^24
    tempRaw -= (tempRaw & (1UL << 23)) ? (1UL << 24) : 0;
    
//    println("temp raw = %ld", tempRaw);
    
    float pressureScaled = ((float)(pressureRaw)) / ((float)(pressureScaleFactor));
    float tempScaled = ((float)(tempRaw)) / ((float)(pressureScaleFactor));
    
//    println("pressure scaled = %f", pressureScaled);
//    println("temp scaled = %f", tempScaled);
    
    return ((uint32_t)(((float)(c00))
            + pressureScaled * (((float)(c10)) + pressureScaled * (((float)(c20)) + pressureScaled * ((float)(c30))))
            + tempScaled * ((float)(c01))
            + tempScaled * pressureScaled * (((float)(c11)) + pressureScaled * ((float)(c21)))));
}

uint16_t sensor_readAdc(void) {
    AD1CON1bits.DONE = 0;
    AD1CON1bits.SAMP = 1; // start sampling

    while (!AD1CON1bits.DONE); // wait for conversion to complete

    return ADC1BUF0; // read value
}
