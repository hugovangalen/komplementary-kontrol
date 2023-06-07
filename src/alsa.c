#include "alsa.h"

// The handle to the output port.
static snd_seq_t * handle = NULL;
static int output_port = -1;

/*
 * Creates the output port.
 */
int alsa_create_output_port()
{
    return snd_seq_create_simple_port( 
        handle, 
        ALSA_CLIENT_NAME " " ALSA_PORT_NAME,
        SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_APPLICATION
    );
}

/* 
 * Create ALSA client and initialises the output port. 
 */
int alsa_open_client(char * client_name)
{
    int err;
    
    err = snd_seq_open( &handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if (err < 0) return err;
    
    if (client_name == NULL) client_name = (char*)&ALSA_CLIENT_NAME;
    snd_seq_set_client_name(handle, client_name );
 
    // set up output port.
    output_port = alsa_create_output_port();
    return output_port;
}


/* 
 * Cleanup ALSA client 
 */
void alsa_close_client()
{
    if (handle != NULL)
    {
        snd_seq_close( handle );
        handle = NULL;
    }
    
    output_port = -1;
}



/*
 * Sends the MMC message in `command` down the wire.
 * 
 * Returns negative values on error, 0 otherwise.
 */
int alsa_send_mmc( unsigned char command, unsigned char channel )
{
    if (!handle || output_port == -1)
    {
        printf( "ALSA not initialised.\n" );
        return -1;
    }

    // from https://en.wikipedia.org/wiki/MIDI_Machine_Control
    //
    // F0 7F <Device-ID> <Sub-ID#1> [<Sub-ID#2> [<parameters>]] F7
    //
    // where Device-ID is:
    // MMC device's ID#; value 00-7F (7F = all devices); AKA "channel number"
    
    // So this becomes the default buffer, where we change
    // the 5th byte (index 4) for the command.
    //
    // Right now, we're ignoring the channel parameter, 
    // so the "Device-ID" remains 0x7f.
    unsigned char mmc_buffer[] = "\xf0\x7f\x7f\x06\x00\xf7";
    mmc_buffer[ 4 ] = command;
    
    // printf( "Send MMC command %02x\n", command );
    
    // Now we have an MMC message to send to the output port.
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);

    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    snd_seq_ev_set_source(&ev, output_port );

    // Just set SYSEX stuff and send it out..
    ev.type = SND_SEQ_EVENT_SYSEX;
    
    snd_seq_ev_set_sysex(&ev, 6, mmc_buffer);
    return snd_seq_event_output_direct( handle, &ev );
}

