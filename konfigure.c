#include "konfigure.h"


static const unsigned char lead_in[] = LEAD_IN;
static const unsigned char pedal_lead_in[] = PEDAL_LEAD_IN;

static const unsigned char sysex_end[] = SYSEX_END;


/* 
 * Clears out the given buffer with all spaces (0x20)
 */
static void clear_buffer( char * buffer, size_t length )
{
    memset( buffer, 0x20, length );
}

static void copy_to_buffer( char * buffer, char * source, size_t length )
{
    clear_buffer( buffer, NAME_SZ );
    memcpy( buffer, source, length );
}

static void print_usage( char * argv0 )
{
    printf( 
        KONFIGURE_NAME " v" KONFIGURE_VERSION " by @hvangalen@mastodon.nl\n\n"
        "This utility generates SysEx to configure Komplete Kontrol A-series keyboards.\n\n"
        "Usage: %s <options> -i path/to/preset.pst\n"
        "\n"
        "Options:\n"
        " -i <path>    Path to preset definition.\n"
        " -o <path>    Write SysEx to this file in stead of standard out.\n"
        " -q           Be less verbose.\n\n"
        RISK_DISCLAIMER,
        argv0
    );
}

/*
 * This simply outputs SysEx data to stdout or
 * a file.
 */
#define OUTPUT_TO_STDOUT  0
#define OUTPUT_TO_FILE    1
#define OUTPUT_TO_MIDI    2 

static FILE * output = NULL;
static int output_type = 0;
static int output_errors = 0;

// For outputting to midi.
static snd_rawmidi_t *midi_output = NULL, **midi_outputp = NULL;

void wrap_fwrite( const unsigned char * buffer, size_t len )
{
    if (output_errors > 0) return;
    
    if (output_type == OUTPUT_TO_MIDI)
    {
        // send raw midi to the ALSA port...
        if (output_errors == 0)
        {
            if (snd_rawmidi_write( midi_output, buffer, len) < 0) 
            {
                output_errors++;
                printf( "Failed to send MIDI to device.\n" );
            }
        }
    }
    else
    {
        // write to file...
        if (fwrite( buffer, 1, len, output ) != len)
        {
            output_errors++;
            printf( "Failed to write data to file.\n" );
        }
    }
}

int main( int argc, char * argv[] )
{
    // this is what we return at the bottom
    int result_code = 0;
    int verbose = 1;
    
    // This is the initial list of presets, which are all empty.
    t_preset_config preset;
    
    // The command line options could be parsed to determine
    // an output file.
    output = stdout;
    output_type = OUTPUT_TO_STDOUT;
    
    char * preset_path = NULL;
    char * output_path = NULL;
    char * alsa_midi_port = NULL;
    
    // Buffer for template name, button labels.
    char buffer[ NAME_SZ ];
    
    int opt;
    
    while ((opt = getopt( argc, argv, "i:o:p:hq" )) != -1)
    {
        switch(opt)
        {
            case 'h':
                print_usage( argv[0] );
                return 0;
                
            case 'i':
                // input file, presets file.
                preset_path = optarg;
                break;
                
            case 'o':
                // this sets the output path, so let's open the file.
                output_path = optarg;
                break;
                
            case 'p':
                // this sets the ALSA MIDI output port to 
                // send the bytes to.
                alsa_midi_port = optarg;
                break;
                
            case 'q':
                verbose = 0;
                break;
        }
    }

    // We *must* have a preset_path now, otherwise
    // we cannot do anything.
    if (preset_path == NULL)
    {
        print_usage( argv[0] );
        printf( "ERROR: The -i <preset> option is required.\n" );
        return 1;        
    }
    
    // Attempt to read the presets...
    int parse_result = preset_parse_config( preset_path, &preset, verbose );
    if (parse_result < 0)
    {
        printf( "ERROR: The preset file could not be parsed.\n" );
        return 2;
    }
    
    if (output_path != NULL)
    {
        output = fopen( output_path, "w" );
        output_type = OUTPUT_TO_FILE;
    }
    
    if (alsa_midi_port != NULL)
    {
        output_type = OUTPUT_TO_MIDI;
        midi_outputp = &midi_output;
        
        int err = snd_rawmidi_open( NULL, 
                midi_outputp, 
                alsa_midi_port, 
                SND_RAWMIDI_NONBLOCK 
        );

        if (err < 0)
        {
            printf( "ERROR: The MIDI port %s could not be opened: %s\n", alsa_midi_port, snd_strerror(err) );
            result_code = 10;
            goto cleanup;
        }
        
        err = snd_rawmidi_nonblock(midi_output, 0);
        if (err < 0)
        {
            printf( "ERROR: The MIDI port blocking mode could not be set: %s\n", snd_strerror(err) );
            result_code = 11;
            goto cleanup;
        }
        
        
    }
    

    // Okay, write stuff to the file:
    //
    //  LEAD_IN
    //  <template name>
    //  <button assignments>
    //  <button labels>
    //  LEAD_OUT
    
    // LEAD_IN
    wrap_fwrite( lead_in, LEAD_IN_SZ );
    
    // template name
    wrap_fwrite( preset.label, NAME_SZ );
    
    // Button assignments. 
    //
    // Note that these *are* send in the proper order,
    // for each page, send the button definitions.
    //
    // The button labels below are sent in a different
    // order which is probably a bug.
    int cc_index = 10;
    for(int page=0; page<4; page++)
    {
        for(int button=0; button<8; button++)
        {
            
            // The index in preset.button[] (due to the two nested
            // for loops, this cannot exceed the actual MAX_PRESET_BUTTONS).
            const int preset_button_index = (page * 8) + button;

            // Copy the values from that button to the buffer.
            const t_preset_button button = preset.button[ preset_button_index ];
printf( "Button %d: %s type=%d chan=%2x %02x %02x %02x %02x\n", 
        preset_button_index, 
        button.label,
        button.type,
        button.channel,
        button.data[0],
        button.data[1],
        button.data[2],
        button.data[3]
);

            wrap_fwrite( &button.type, 1 );
            wrap_fwrite( &button.channel, 1 );
            wrap_fwrite( button.data, 4 );
            
#ifdef HARDCODED_ASSIGNEMENTS
            // Button format is like "0x00 0x00 CC 0x00 0x00 0x7f"
            //
            // 1st and 2nd bytes probably indicate type,
            // 3rd byte the CC (depends on type I guess)
            // 4th type of CC I guess
            // 5th range from
            // 6th range to
            buffer[ 0 ] = 0x00;         // CC = 0x00, PGM CHANGE = 0x01
            buffer[ 1 ] = 0x00;         // channel (0..15 for 1 to 16)
            
            // PGM CHANGE has range 0..126
            // buffer[ 2 ] = 0x00
            // buffer[ 3 ] = 0x00
            // buffer[ 4 ] = 0x00
            // buffer[ 5 ] = 0x7f
            
            buffer[ 2 ] = cc_index; // button + 10; // CC 
            cc_index++;

            buffer[ 3 ] = 0x00; // button type 0x00 = ABS, 0x01 = REL, 0x01 REL OFFSET
            
            // ABS type:
            buffer[ 4 ] = 0x00; // range from
            buffer[ 5 ] = 0x7f; // range to
            
            wrap_fwrite( buffer, 1, 6, output );

            // REL type:
            // buffer[ 4 ] = 0x00; // probably the step value
            // buffer[ 5 ] = 0x00; // probably the step value
            
            // REL OFFSET type:
            // buffer[ 4 ] = 0x3f;
            // buffer[ 5 ] = 0x41
#endif /* HARDCODED_ASSIGNEMENTS */
            
        }
    }
    
    // The button labels. 
    //
    // THESE ARE SENT IN A DIFFERENT ORDER THAN THE 
    // ITEMS ABOVE. In stead of iterating each page and
    // sending each button, like a logical person would do,
    // these are iterating the buttons and then sending them
    // for each page. 
    //
    // Weird.
    
#ifdef HARDCODED_ASSIGNEMENTS    
    copy_to_buffer( buffer, "BUTTON", 6 );
#endif
    
    for (int button=0; button<8; button++)
    {
        // for some unexplained reason the 2nd button on all 
        // pages are *always* garbled on the device, though 
        // the data sent to it is fine.
        //
        // this also happens from the windows configuration utility,
        // so I am not quite sure what causes this
        for(int page=0; page<4; page++)
        {   
            // 
            const int preset_button_index = button + (page * 4);
            const t_preset_button button = preset.button[ preset_button_index ];
            
            wrap_fwrite( button.label, NAME_SZ );
            
#ifdef HARDCODED_ASSIGNEMENTS                
            buffer[7] = 0x31 + page; // 1..4
            buffer[8] = '/';
            buffer[9] = 0x31 + button; // 1..8
            
            wrap_fwrite( buffer, 1, NAME_SZ, output );
#endif
            
        }
    }
 
    // LEAD_OUT
    wrap_fwrite( sysex_end, SYSEX_END_SZ );

#ifdef PEDAL_TOO
    
    // 4 Pedal BUTTONS unsure why 4 but I guess this somehow makes 
    // sense.
    //
    // 0000   f0 00 21 09 30 17 4d 43 01 00 03 18 00 00 00 40   ..!.0.MC.......@
    // 0010   03 00 7f 00 00 41 02 00 7f 00 00 00 00 00 00 00   .....A..........
    // 0020   01 00 03 00 01 f7                                 ......
    
    // LEAD_IN
    wrap_fwrite( pedal_lead_in, 1, PEDAL_LEAD_IN_SZ, output );
    
    for (int button=0; button<4; button++)
    {
        // This just turns the buttons off, I guess.
        buffer[ 0 ] = 0x00;
        buffer[ 1 ] = 0x00;
        buffer[ 2 ] = 0x00;
        buffer[ 3 ] = 0x00;
        buffer[ 4 ] = 0x00;
        buffer[ 5 ] = 0x00;
        
        wrap_fwrite( buffer, 1, 6, output );
    }
 
    // LEAD_OUT
    wrap_fwrite( sysex_end, 1, SYSEX_END_SZ, output );   
#endif /* PEDAL_TOO */

cleanup:
    if (midi_outputp)
    {
		snd_rawmidi_close(midi_output);
        midi_outputp = NULL;
    }
    
    // If we have opened an actual file, then we 
    // want to close it.
    if (output_type == OUTPUT_TO_FILE)
    {
        fclose( output );
    }
    
    return result_code;
}
