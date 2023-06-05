#ifndef _BUTTON_LEDS_H_
#define _BUTTON_LEDS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hid.h"

#define TOTAL_HID_BUTTONS     21

#define LED_BRIGHT          0x7e
#define LED_ON              0x7c
#define LED_OFF             0x00

void leds_init();
void leds_clear();
void leds_update_led( int index, int state );
int leds_sync();
void leds_animate_on();
void leds_animate_off();
void leds_off();

#endif /* _BUTTON_LEDS_H_ */
