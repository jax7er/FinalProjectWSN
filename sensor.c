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

#define PRESSURE_SENSOR_ADDR      0x77
#define HUMIDITY_TEMP_SENSOR_ADDR 0x40

static void _i2c_write_command(uint8_t addr, uint8_t cmd) {    
    i2c_start(); 
    
    i2c_tx(i2c_addrWrite(addr));
    
    if (i2c_gotAck()) {
        i2c_tx(cmd);
    }
    
    i2c_stop();
}

void sensor_init(void) {
    _i2c_write_command(HUMIDITY_TEMP_SENSOR_ADDR, 0xFE); // 0xFE is the reset command
}

uint8_t sensor_readHumidity(void) {    
    // assert start condition
    i2c_start(); 
    
    // slave address with write
    i2c_tx(i2c_addrWrite(HUMIDITY_TEMP_SENSOR_ADDR));
    if (i2c_gotNack()) { // expecting ACK = 0
        println("slave addr nack");
        return 0xFF;
    }
    
    // command for humidity read
    i2c_tx(0xF5);  // 0xF5 = read humidity  
    if (i2c_gotNack()) { // expecting ACK = 0
        println("command nack");
        return 0xFF;
    }
    
    // when sensor reading is complete, sends an ACK
    do {
        delay_ms(5);
        i2c_repeatedStart();
        i2c_tx(i2c_addrRead(HUMIDITY_TEMP_SENSOR_ADDR));
    } while (i2c_gotNack()); // when complete ACK = 0
    
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
    
    return ((uint8_t)(((125 * rxValue) / 65536) - 6));
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
