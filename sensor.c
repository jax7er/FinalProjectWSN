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

#define PRESSURE_SENSOR_ADDR      0x77
#define HUMIDITY_TEMP_SENSOR_ADDR 0x40

//typedef enum {
//    I2C2_MESSAGE_FAIL,
//    I2C2_MESSAGE_PENDING,
//    I2C2_MESSAGE_COMPLETE,
//    I2C2_STUCK_START, 
//    I2C2_MESSAGE_ADDRESS_NO_ACK,
//    I2C2_DATA_NO_ACK,
//    I2C2_LOST_STATE
//} I2C2_MESSAGE_STATUS;
static I2C2_MESSAGE_STATUS _status = I2C2_MESSAGE_FAIL;
static uint8_t _command = 0;

void sensor_init(void) {
    
    //*** http://www.microchip.com/forums/m985492.aspx ***//
    
    // dummy write to initialise I2C driver
    I2C2_MasterWrite(&_command, 1, 0, &_status);
    
    //println("status = %u", _status);
    
    _command = 0xFE; // 0xFE = reset
    I2C2_MasterWrite(&_command, 1, HUMIDITY_TEMP_SENSOR_ADDR, &_status);
    
    println("status = %u", _status);
    
    while (_status == I2C2_MESSAGE_PENDING); // wait for message to be sent
    
    println("status = %u", _status);
}

uint16_t sensor_readHumidity(void) {    
    _command = 0xF5; // 0xF5 = read humidity
    I2C2_MasterWrite(&_command, 1, HUMIDITY_TEMP_SENSOR_ADDR, &_status);
    
    uint16_t attempts = 0;
    uint8_t result[2] = {0};        
    do {
        I2C2_MasterRead(result, 2, HUMIDITY_TEMP_SENSOR_ADDR, &_status);
        
        attempts++;
        
        while (_status == I2C2_MESSAGE_PENDING);
    } while (_status == I2C2_DATA_NO_ACK);
    
    println("post status = %u, attempts = %u", _status, attempts);
    
    return (((uint16_t)(result[0])) << 8) | ((uint16_t)(result[1]));
}

uint16_t sensor_readTemperature(void) {
    return 0;
}

uint16_t sensor_readLastTemperature(void) {
    return 0;
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
