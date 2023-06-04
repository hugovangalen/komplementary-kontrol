#ifndef _HID_STUFF_H_
#define _HID_STUFF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <hidapi/hidapi.h>

int hidstuff_init( int vid, int pid );
void hidstuff_exit();

int hidstuff_send_raw( 
    unsigned char * buffer, size_t buflen,
    void * receive_buffer, size_t receive_buflen );

int hidstuff_read_raw( void * receive_buffer, size_t receive_buflen, int blocking );
int hidstuff_read_raw_timeout( void * receive_buffer, size_t receive_buflen, int millis );

#endif /* _HID_STUFF_H_ */
