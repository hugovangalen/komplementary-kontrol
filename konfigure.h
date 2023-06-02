#ifndef _KONFIGURE_H_
#define _KONFIGURE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>

#include "konfigure_parser.h"
#include "defs.h"
#include "version.h"

// SysEx fixed values.
#define LEAD_IN         "\xf0\x00\x21\x09\x30\x17\x4d\x43\x01\x00\x01\x0a\x04"
#define LEAD_IN_SZ      13

#define PEDAL_LEAD_IN       "\xf0\x00\x21\x09\x30\x17\x4d\x43\x01\x00\x03\x18\x00"
#define PEDAL_LEAD_IN_SZ    13

// This is just an end-of-sysex packet, really.
#define SYSEX_END       "\xf7"
#define SYSEX_END_SZ    1

#endif /* _KONFIGURE_H_ */
