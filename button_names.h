#ifndef _BUTTON_NAMES_H_
#define _BUTTON_NAMES_H_

#include "defs.h"

#include <stdio.h>
#include <string.h>


#define BUTTON_BUFFER_SZ         16
#define TOGGLE_BUTTON_TOTAL      40

// These are 'virtual' buttons to handle the turning 
// of the 4D dial.
#define DIAL_CW_INDEX       40
#define DIAL_CCW_INDEX      41

// This is the button total + the extra buttons for the turning
// of the dial.
#define REAL_BUTTON_TOTAL   TOGGLE_BUTTON_TOTAL + 2

const char * get_button_name(int);
int get_button_index( char * );

#endif /* _BUTTON_NAMES_H_ */
