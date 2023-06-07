#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "button_names.h"
#include "uinput_stuff.h"
#include "mmc_stuff.h"
#include "mapping.h"

int config_read( char * filename, int verbose );
