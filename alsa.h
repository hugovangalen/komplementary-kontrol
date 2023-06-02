#ifndef _ALSA_STUFF_H_
#define _ALSA_STUFF_H_

#include "version.h"
#include "defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <alsa/asoundlib.h>

#define ALSA_CLIENT_NAME      "KOMPLEMENTARY"
#define ALSA_PORT_NAME        "MIDI OUT"

int alsa_open_client( char * client_name );
int alsa_send_mmc( unsigned char command, unsigned char channel );

void alsa_close_client();

#endif /* _ALSA_STUFF_H_ */
