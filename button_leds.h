#ifndef _BUTTON_LEDS_H_
#define _BUTTON_LEDS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <hidapi/hidapi.h>

#define TOTAL_HID_BUTTONS  21

int init_button_leds( int vid, int pid );
int fini_button_leds();

void update_button_led( int index, int state );
void clear_button_leds();
int sync_button_leds();

void fancy_button_leds();
void fancy_button_leds_off();

#endif /* _BUTTON_LEDS_H_ */
