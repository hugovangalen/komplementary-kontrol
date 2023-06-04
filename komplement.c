#include "komplement.h"
//#define KEYS_DEBUG
//#define DUMP_KEYS_IN

#define PACKET_SZ    8
#define MAX_BUTTONS 40

#define KEY_PRESS     1
#define KEY_RELEASE   0
#define KEY_INITIAL  -1

// The stucture containing the tool configuration. 
static t_komplement_config cfg;

// Interrupt handling to catch SIGINT and abort gracefully.
static volatile int read_aborted = 0;
void handle_interrupt(int dummy) 
{
    read_aborted = 1;
}

static void print_usage( char * argv0 )
{
    printf(
        KOMPLEMENT_NAME " v" KOMPLEMENT_VERSION " by @hvangalen@mastodon.nl\n\n"
        "This utility converts the non-MIDI keys on the Komplete Kontrol A-series keyboards\n"
        "to keyboard presses, so they will become usable in your software of choice.\n\n"   
        "Usage: %s <options>\n\n"
        "Options:\n"
        " -o /path/to/uinput   The path to uinput device file (" DEFAULT_UINPUT_PATH ").\n"
        " -m /path/to/mapping  The path to mapping file (required to be useful).\n"
        " -a                   Do not create ALSA MIDI output port for MMC messages.\n"
        " -n                   Do not animate the buttons when starting/stopping.\n\n"
        " -q                   Be less verbose.\n"

        "Advanced options:\n"
        " -p <productId>       USB product ID (in case you want to try other hardware).\n"
        " -v <vendorId>        USB vendor ID (in case you want to try other hardware).\n\n"        
        RISK_DISCLAIMER,
        argv0 );
}


/*
 * Helper that sends all the key press or release events to the
 * appropriate destination.
 */
static void send_key_wrap( int fd_uinput, mapping_key_t send_key, int press )
{
    for(int ki = 0; ki < send_key.length; ki++)
    {
        if (send_key.keys[ki].type == MAPPING_TYPE_KEY)
        {
            if (press) 
            {
                //printf( "press %d\n", send_key.keys[ki].key );
                key_press( fd_uinput, send_key.keys[ki].key );
            } 
            else 
            {
                //printf( "release %d\n", send_key.keys[ki].key );
                key_release( fd_uinput, send_key.keys[ki].key );
            }
        }
        else if (press)
        {
            //printf( "mmc %d\n", send_key.keys[ki].key );
            alsa_send_mmc( send_key.keys[ki].key, 0 );
        }
    }
}


/*
 * This lights up only the buttons that have an actual action
 * mapped together with SHIFT 
 */
static void lightup_shifted()
{
    int light_it_up;
    for(int i=0; i<TOTAL_HID_BUTTONS; i++)
    {
        if (i == 0) light_it_up = LED_BRIGHT;
        else if (mapping_is_mapped(i, 1)) light_it_up = LED_ON;
        else light_it_up = LED_OFF;
            
        leds_update_led( i, light_it_up );        
    }
    
    leds_sync(cfg.vid, cfg.pid);
}



/*
 * This lights up only those buttons that have a mapping without
 * the SHIFT being pressed
 */
static void lightup_normal()
{
    int light_it_up;
    for(int i=0; i<TOTAL_HID_BUTTONS; i++)
    {
        // shift and octaves always lit
        light_it_up = (i == 0 
            || i == 19 
            || i == 20 
            || mapping_is_mapped(i, -1)) ? LED_ON : LED_OFF; 
            
        leds_update_led( i, light_it_up );
    }
    
    leds_sync(cfg.vid, cfg.pid);
}



/*
 * This lights up the buttons the first time, with a slight delay
 * because I think it's cool.
 * 
 * If cfg.animate is false it will be as good as instant.
 */
static int lightup_initial()
{
    for(int i=0; i<TOTAL_HID_BUTTONS; i++)
    {
        // shift and octaves always lit
        int light_it_up = (i == 0 || i == 19 || i == 20 || mapping_is_mapped(i, -1)) ? 1 : 0;
        leds_update_led( i, light_it_up ? LED_ON : LED_OFF );
        
        if (leds_sync(cfg.vid, cfg.pid) < 0)
        {
            return -1;
        }

        if (cfg.animate)
            usleep( 5000 );
    }
}


unsigned char reverse_bits_in_byte( unsigned char byte, char bits )
{
    unsigned char result = 0;
    for(int i=0; i<bits; i++)
    {
        if ((byte & (1 << i)))
            result |= 1 << ((bits - 1) - i);   
    }
    return result;
}

int main(int argc, char* argv[])
{
    // 
    int return_code = 0;
    
    // Tool configuration, set the defaults.
    memset( &cfg, 0, sizeof cfg );    
        
    // These can all be manipulated via command line options.
    cfg.vid = 0x17cc;
    cfg.pid = 0x1730;
    
    cfg.mapping_path = NULL; // "mappings/rosegarden";
    
    cfg.animate = true;
    cfg.quiet = false;
    cfg.midi_controller = true;
    
    
    int opt;
    int total_options_parsed = 0;
    while ((opt = getopt( argc, argv, "v:p:m:o:qnha" )) != -1)
    {
        total_options_parsed++;
        switch(opt)
        {
            case 'h':
                print_usage( argv[0] );
                return 0;
                
            case 'v': 
                cfg.vid = strtol(optarg, NULL, 16);
                break;
                
            case 'p': 
                cfg.pid = strtol(optarg, NULL, 16);
                // printf( "ProductID: %4x\n", cfg.pid );
                break;
                
            case 'm': 
                cfg.mapping_path = strdup(optarg);
                break;
            
            case 'o': 
                cfg.uinput_path = strdup(optarg);
                break;
                
            case 'q': 
                cfg.quiet = true;
                break;
                
            case 'n': 
                cfg.animate = false;
                break;
                
            case 'a': 
                cfg.midi_controller = false;
                break;
        }
    }
    
    //
    if (total_options_parsed == 0)
    {
        print_usage( argv[0] );
        return 1;
    }
    
    if (!cfg.uinput_path)
    {
        cfg.uinput_path = strdup( DEFAULT_UINPUT_PATH );
    }
    

    // Install signal handler now...
    signal( SIGINT, handle_interrupt );
    signal( SIGTERM, handle_interrupt );
    signal( SIGQUIT, handle_interrupt );
    

    // There are 54 packets of 8 bytes in size.
#define KEYPRESS_PACKETS_TOTAL  54
    const int packets_expected = KEYPRESS_PACKETS_TOTAL;

    // Buffer for a single packet.
    unsigned char packet[ PACKET_SZ ];
    
    // Set up the button mappings...
    mapping_init();

    
    // Read the configuration, and then only light up
    // those buttons that have an actual mapping.;
    if (cfg.mapping_path)
    {
        if (config_read( cfg.mapping_path, cfg.quiet ? 0 : 1 ) < 0)
        {
            printf( "The mapping file could not be read.\n" );
            return_code = 2;
            goto clean_up_and_exit;
        }
    }
    else
    {
        print_usage( argv[0] );
        printf( "ERROR: The -m <mapping> option is required.\n" );
        return_code = 1;
        
        goto clean_up_and_exit;
    }
   
    // Initialise HIDAPI:
    hidstuff_init( cfg.vid, cfg.pid );
    
    // These 3 bytes put the device into a certain mode where all the normal
    // operation ceases and it interfaces with the operating system, let's
    // call that "Interactive Mode".
    // unsigned char hid_packet_interactive[] = { 0xa0, 0x03, 0x04 };
    
    // This resets the HID device back to "MIDI Mode":
    unsigned char hid_packet_midi_mode[] = { 0xa0, 0x07, 0x00 };
    hidstuff_send_raw( hid_packet_midi_mode, 3, NULL, 0 );
    
    // Button led state initialise.
    leds_init();
    
    // Animates the button state, eventually this 
    // should hilite only the mapped buttons and
    // keep the rest dark.
    if (cfg.animate)
    {
        leds_animate_on(cfg.vid, cfg.pid);
    }
        
   
    // Initial LED state.
    if (lightup_initial() < 0)
    {
        printf( 
            "Opening the HID device %04x:%04x failed.\n"
            "Did you pass the right vendorId and productId? Does the current user have permissions?\n",
            cfg.vid, 
            cfg.pid
        );
            
        goto clean_up_and_exit;
    }
    

    
    // Set up MIDI output.
    if (cfg.midi_controller)
    {
        // Initialise ALSA
        if (alsa_open_client( NULL ) < 0)
        {
            printf( "Error opening ALSA port.\n" );
            return_code = 10;
            goto clean_up_and_exit;
        }
        else if (!cfg.quiet)
        {
            printf( "ALSA output port created.\n"  );
        }
    }
    
    
    // The `button_state` is to keep track of changes 
    // in button press state. 0: released, 1: pressed, -1: initial
    char button_state[ MAX_BUTTONS ]; // button 0..39
    memset( button_state, 0, MAX_BUTTONS );
    
    // for iterating
    int button_number;
    
    // Set up uinput device.
    int fd_uinput = uinput_open( cfg.uinput_path );
    if (fd_uinput < 0)
    {
        // close(fd);
        perror( "uinput open" );
        
        
        printf( 
            "Opening the device file at %s failed.\n"
            "Can it be read by the current user?\n",
            cfg.uinput_path
        );
        
        return_code = -2;
        goto clean_up_and_exit;
    }

    int read_errors = 0;
    
    char position_4d_dial = -1; // to determine the way the dial goes
    // int value_4d_dial = 0;           // the real dial value that can be mapped to something?
    
    bool shift_state_changed = false;
    bool shift_was_pressed = false;
    
    bool shift_outgoing_pressed = false;
    
    // read packets until interrupted
    while (read_aborted == 0)
    {
        /*
        // Set up for polling...
        FD_ZERO( &rfds );
        FD_SET( fd, &rfds );
        
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        
        select_result = select(fd+1, &rfds, NULL, NULL, &tv );
        if (select_result == -1) {
            perror( "select" );
            break;
        }
        else if (select_result == 0) 
        {
            // No data yet, so loop again.
            continue;
        }
        */
        
        size_t keypress_buffer_size = 30; //
        unsigned char keypress_buffer[ 30 ];      
//        printf( "trying to read...\n" );
        
        //int keypress_buffer_read = hidstuff_read_raw( &keypress_buffer, keypress_buffer_size, 1 );
        int keypress_buffer_read = hidstuff_read_raw_timeout( &keypress_buffer, keypress_buffer_size, 2000 );
        if (keypress_buffer_read == 0) continue;
        
#ifdef KEYS_DEBUG
        printf( "read %d\n", keypress_buffer_read );
#endif
        
#ifdef DUMP_KEYS_IN
        // 00000001
        // 00000000     // these bytes identify the pressed/released key
        // 00000000     // but the bit order is flipped.
        // 00000000
        // 00000000
        // dump the buffer as binary
        for(int b=0; b<keypress_buffer_read; b++)
        {
            printf( "%08b ", keypress_buffer[b] );
            if ((b+1) % 8 == 0) printf( "\n" );
        }
        printf( "\n" );
#endif 
        
        // Determine the pressed keys. Multiple keys can be pressed
        // at the same time.
        const long key_value = ((long)keypress_buffer[1])
                | ((long)keypress_buffer[2]) << 8
                | ((long)keypress_buffer[3]) << 16
                | ((long)keypress_buffer[4]) << 24;
            
#ifdef KEYS_DEBUG
        printf( "key value: %032b %ld\n", key_value, key_value  );
#endif
            
        const bool shift_is_pressed = key_value & 1;
        if (shift_is_pressed != shift_was_pressed) 
        {
#ifdef KEYS_DEBUG
            printf( "Shift state changed: %s\n", shift_is_pressed ? "PRESSED":"RELEASED" );
#endif
            
            if (shift_is_pressed) lightup_shifted();
            else lightup_normal();
            
            shift_was_pressed = shift_is_pressed;

            // If SHIFT is released before any other button is released, the wrong
            // key release event might be sent down the line, so in that case we 
            // just assume all buttons are released.
            if (!shift_is_pressed)
            {
                // Assume all have been released.
                memset( button_state, MAX_BUTTONS, 0 );
            }
        }
        
        
        
        unsigned char new_dial_position = keypress_buffer[ 28 ];
        int dial_change = 0;
        
        if (position_4d_dial == -1)
        {
            position_4d_dial = new_dial_position;
        } 
        else
        {        
            if (new_dial_position == 0xf && position_4d_dial == 0) {
                // It decremented and wrapped.
                dial_change = -1;
            } else if (new_dial_position == 0x0 && position_4d_dial == 0xf) {
                // It incremented and wrapped 
                dial_change = +1;
            } else {
                dial_change = new_dial_position - position_4d_dial;
            }
        
            if (dial_change != 0)
            {   
                // It actually changed, so we send a keydown / key up event.
                const mapping_key_t send_key = mapping_get( 
                    dial_change > 0 ? DIAL_CW_INDEX : DIAL_CCW_INDEX 
                );

                // We want to send this as a single keypress/release
                // event, so first this.
                send_key_wrap( fd_uinput, send_key, 1 );

#ifdef KEYS_DEBUG
                if (dial_change > 0) {
                    printf( "4D dial INC\n" );
                } else if (dial_change < 0) {
                    printf( "4D dial DEC\n" );
                }
#endif /* KEYS_DEBUG */

                // And release it...
                send_key_wrap( fd_uinput, send_key, 0 );
                position_4d_dial = new_dial_position;
            }
        }
        
        // So if any button is pressed, then the relevant bit would be set.
        // The shift is ignored, so we start at '1'.
        for(int button_number = 1; button_number < TOTAL_HID_BUTTONS; button_number++)
        {
            long test_value = 1 << button_number;   
            int  new_button_state = (key_value & test_value) ? 1 : 0; // pressed or released
            
            if (new_button_state != button_state[ button_number ])
            {
                // This key is pressed, so if there is any mapping, let's
                // do something with it.
                const mapping_key_t send_key = shift_is_pressed
                    ? mapping_get_shifted( button_number )
                    : mapping_get( button_number );

                send_key_wrap( fd_uinput, send_key, new_button_state );
                button_state[ button_number ] = new_button_state;
            }
        }
        
    } // while...
    
clean_up_and_exit:

    // clean-up stuff
    alsa_close_client();
        
    if (cfg.uinput_path) free(cfg.uinput_path);
    if (cfg.mapping_path) free(cfg.mapping_path);
    
    //if (fd>-1) close(fd);
    if (fd_uinput>-1) uinput_close(fd_uinput);
    
    if (cfg.animate) {
        leds_animate_off(cfg.vid, cfg.pid);
    } else {
        leds_off(cfg.vid, cfg.pid);
    }
    
    hidstuff_exit();
    
    return return_code;
}
