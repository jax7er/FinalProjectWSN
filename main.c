/*
 * File:   main.c
 * Author: Jack
 *
 * Created on 18 February 2018, 10:33
 */

#include <stdio.h>
#include "xc.h"
#include "delay.h"
#include "utils.h"
#include "MRF24J40.h"
#include "mcc_generated_files/mcc.h"
    
int main(void) {
    SYSTEM_Initialize();
        
    printf("board init done\r\n");      
    
    mrf24j40_initialize();
    
    printf("radio init done\r\n");
        
    uint8_t rxMode = RX_TX_SELECT_GetValue(); // rx mode if the pin is high
        
    if (rxMode) {
        printf("RX mode\r\n");
    } else {
        printf("TX mode\r\n");
        TX_MODE_LED_SetHigh();
    }
    
    while (1) {
        delay_ms(1000);
    }
    
    return 0;
}
