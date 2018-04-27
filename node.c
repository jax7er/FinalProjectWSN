/*
 * File:   node.c
 * Author: jmm546
 *
 * Created on 26 April 2018, 11:32
 */


#include "xc.h"
#include "node.h"
#include "utils.h"
#include "delay.h"
#include "payload.h"
#include "radio.h"
#include "sensor.h"
#include "mcc_generated_files/pin_manager.h"

static void _baseLoop(void);
static void _moteLoop(void);
static void _cycleMode(void);

node_t node = (node_t){
    .type = BASE, // default to base
    .id = 0, // default to 0 (base)
    .mode = TIMED,
    .requestMode = MANUAL,
    .requestDelay_ms = 5000,
    .radioTimedSleep_ms = 1000
};

void node_run(void) {
    if (node.type == BASE) {
        _baseLoop();
    } else {
        _moteLoop();
    }
}

static void _baseLoop(void) {
    if (node.mode == REQUEST && node.requestMode == AUTO) {
        timer_restart();
    } else {
        timer_stop();
    }
    
    while (1) {
        if (!(radio.ifs.event)) {
//            if (mode == REQUEST) {
                while (!(button_down || radio.ifs.event)) {
                    if ((node.requestMode == AUTO) && (timer_getTime_ms() > node.requestDelay_ms)) {
                        timer_restart();
                        break;
                    } else {
                        delay_ms(10);
                    }
                }
//            } else {
//                Sleep();
//            }
        }

        if (radio.ifs.event) {
            radio_getIntFlags();

            if (radio.ifs.rx) {
                radio.ifs.rx = 0;

                LED_Toggle();

                payload_read();

                LED_Toggle();
            }
        } else { // button_down || (timer_getTime_us() > numDelay_ms)
            while (button_down); // wait for button to be released
            radio_request_readings();
        }
    }
}

static void _moteLoop(void) {
    while (1) {
        while (button_down) {
            _cycleMode();
            delay_ms(500);
        }
//        while (button_down); // wait if pressing the button
//        while (button_up) { // wait for button press or request   
        while (node.mode != STREAM) {
            if (node.mode == TIMED) {
                radio_sleep_timed(node.radioTimedSleep_ms);
            }

            if (!(radio.ifs.event)) { // if already an event don't sleep as radio will not generate any more interrupts until INTSTAT is read
                Sleep(); // sleep until interrupt from radio
            }

            if (radio.ifs.event) {
                radio_getIntFlags();

                if (radio.ifs.rx) {
                    radio.ifs.rx = 0;

                    if (node.mode == REQUEST) {
                        radio_read_rx();

                        if (payload_isReadingsRequest()) {
                            //TODO work out which readings have been requested, 0 is all readings

                            break;
                        }
                    }
                } else if (radio.ifs.wake) {
                    radio.ifs.wake = 0;

                    if (node.mode == TIMED) {
                        break;
                    }
                }
            }
        }
//        }
//        while (button_down); // wait for button to be released if previously pressed

        sensor_update();          

        payload_write();

        // read back TXNFIFO
//        radio_printTxFifo();
//        delay_ms(100); // allow printing to finish

        LED_Toggle();

        radio_trigger_tx(); // trigger transmit

        do {            
            while (!(radio.ifs.event)); // wait for interrupt from radio

            radio_getIntFlags();
        } while (!(radio.ifs.tx));
        radio.ifs.tx = 0; // reset TX flag

        LED_Toggle();

//        radio_check_txstat();

//        delay_ms(2500);
    }
}

static void _cycleMode(void) {
    switch (node.mode) {
        case TIMED:
            node.mode = REQUEST;
            utils_flashLed(2);
            break;
        case REQUEST:
            node.mode = STREAM;
            utils_flashLed(3);
            break;
        case STREAM:
            node.mode = TIMED;
            utils_flashLed(1);
            break;
        default:
            node.mode = TIMED;
    }
}