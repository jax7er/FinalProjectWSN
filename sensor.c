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
#define RH_TEMP_RESET  0xFE
#define RH_READ        0xF5
#define TEMP_READ      0xF3
#define LAST_TEMP_READ 0xE0

#define _readingToTemp(r) (((175.72 * ((float)(r))) / 65536.0) - 46.85)
#define _readingToRh(r)   ((uint8_t)(((125 * (r)) / 65536) - 6))

static void _i2c_write_command(uint8_t addr, uint8_t cmd) {    
    i2c_start(); 
    
    i2c_tx(i2c_addrWrite(addr));
    
    if (i2c_gotAck()) {
        i2c_tx(cmd);
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
    
    if (cmd == LAST_TEMP_READ) {
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
    _i2c_write_command(RH_TEMP_ADDR, RH_TEMP_RESET);
}

uint8_t sensor_readRh(void) { 
    uint32_t reading = _rhTempSensorGetReading(RH_READ); // 16 bit value but make 32 bits for ease of calculating %RH    
    return _readingToRh(reading);
}

float sensor_readTemp(void) {
    uint16_t reading = _rhTempSensorGetReading(TEMP_READ);
    return _readingToTemp(reading);
}

float sensor_readLastTemp(void) {
    uint16_t reading = _rhTempSensorGetReading(LAST_TEMP_READ);
    println("reading = %u", reading);
    return _readingToTemp(reading);
}

uint16_t sensor_readPressure(void) {
    return 0;
}

uint16_t sensor_readAdc(void) {
    AD1CON1bits.DONE = 0;
    AD1CON1bits.SAMP = 1; // start sampling

    while (!AD1CON1bits.DONE); // wait for conversion to complete

    return ADC1BUF0; // read value
}
