#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <linux/uinput.h>

#include "defs.h"

#ifndef _UINPUT_STUFF_H_
#define _UINPUT_STUFF_H_

int uinput_open(char *path);
void uinput_close();

void key_press( int fd, int code );
void key_release( int fd, int code );

int key_parse( char * );
const char * key_name( int code );

#endif /* _UINPUT_STUFF_H_ */
