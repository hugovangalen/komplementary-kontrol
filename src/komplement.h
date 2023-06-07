#ifndef _KOMPLEMENT_H_
#define _KOMPLEMENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <linux/hiddev.h>

#include "button_names.h"
#include "button_leds.h"
#include "uinput_stuff.h"
#include "mapping.h"
#include "config.h"
#include "version.h"
#include "alsa.h"

#define DEFAULT_HIDDEV_PATH     "/dev/usb/hiddev0"
#define DEFAULT_UINPUT_PATH     "/dev/uinput"

typedef struct t_komplement_config {
    
    // Output stuff
    bool quiet;
    bool animate;
    
    // create an output ALSA MIDI port that
    // can be used as input by other software
    bool midi_controller;
    
    // Path to mapping configuration.
    char * mapping_path;
    
    // The default uinput path (default /dev/uinput)
    char * uinput_path;
    
    // USB VendorId / ProductId
    int vid;
    int pid;
    
} t_komplement_config;

#endif /* _KOMPLEMENT_H_ */
