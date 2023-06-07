#include "hid.h"
// #define HID_DEBUG 

static hid_device * device = NULL;

/* 
 * Initialise HID device.
 * 
 * Returns -1 on error, 0 if all is well.
 */
int hidstuff_init(int vid, int pid)
{
    hid_init();
    
    device =  hid_open( vid, pid, NULL );
    if (!device) return -1;
    
    // set the device to blocking while waiting so
    // we don't have to poll it.
    hid_set_nonblocking( device, 0 );
    
    return 0;
}


/* 
 * Cleans up HID device.
 */
void hidstuff_exit()
{
    if (device) hid_close(device);
    device = NULL;
    
    hid_exit();
}


/* 
 * Attempts to read data from HID device.
 * 
 * Returns -1 on error, the number of bytes otherwise (which could be 0).
 */
int hidstuff_read_raw( void* receive_buffer, size_t receive_buflen, int blocking )
{
    if (!device)
    {
        return -1;
    }
    
    if (blocking) {
        hid_set_nonblocking( device, 0 );
    } else {
        hid_set_nonblocking( device, 1 );
    }
    
    return hid_read( device, receive_buffer, receive_buflen );
}


/* 
 * Attempts to read data from HID device but times out after the requested
 * millis.
 * 
 * Returns -1 on error, the number of bytes otherwise (which could be 0).
 */
int hidstuff_read_raw_timeout( void * receive_buffer, size_t receive_buflen, int millis )
{
    if (!device) return -1;
    
    // always do this in blocking mode?
    hid_set_nonblocking( device, 0 );
    return hid_read_timeout( device, receive_buffer, receive_buflen, millis );
}



/* 
 * Send raw USB HID payload to the specified device and, if receive_buflen is non-zero
 * waits for a result.
 */
int hidstuff_send_raw( 
    unsigned char * buffer, size_t buflen,
    void* receive_buffer, size_t receive_buflen )
{
    if (!device)
    {
        return -1;
    }
    
#ifdef HID_DEBUG
    printf( "send_raw (write %d, read %d)\n", buflen, receive_buflen );
#endif
    int result = hid_write( device, buffer, buflen );

    if (receive_buffer && receive_buflen > 0) 
    {
#ifdef HID_DEBUG
        printf( "read from %d\n", __LINE__ );
#endif
        
        // always read blocking, I guess?
        hidstuff_read_raw( receive_buffer, receive_buflen, 1 );
    }
    
    return result;
}
 
