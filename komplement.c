#include "komplement.h"

// #define DUMP_KEYS_IN 


// Interrupt handling to catch SIGINT and abort gracefully.
static volatile int read_aborted = 0;
void handle_interrupt(int dummy) 
{
    read_aborted = 1;
}

#define PACKET_SZ    8
#define MAX_BUTTONS 40

#define KEY_PRESS     1
#define KEY_RELEASE   0
#define KEY_INITIAL  -1


static void print_usage( char * argv0 )
{
    printf(
        KOMPLEMENT_NAME " v" KOMPLEMENT_VERSION " by @hvangalen@mastodon.nl\n\n"
        "This utility converts the non-MIDI keys on the Komplete Kontrol A-series keyboards\n"
        "to keyboard presses, so they will become usable in your software of choice.\n\n"   
        "Usage: %s <options>\n\n"
        "Options:\n"
        " -i /path/to/hiddev   The path to relevant hiddev device (" DEFAULT_HIDDEV_PATH ").\n"
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
            if (press) {
                key_press( fd_uinput, send_key.keys[ki].key );
            } else {
                key_release( fd_uinput, send_key.keys[ki].key );
            }
        }
        else if (press)
        {
            alsa_send_mmc( send_key.keys[ki].key, 0 );
        }
    }
}


int main(int argc, char* argv[])
{
    // 
    int return_code = 0;
    
    // Tool configuration, set the defaults.
    t_komplement_config cfg;
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
    while ((opt = getopt( argc, argv, "v:p:m:o:i:qnha" )) != -1)
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
            
            case 'i': 
                cfg.hiddev_path = strdup(optarg);
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
    
    if (!cfg.hiddev_path)
    {
        cfg.hiddev_path = strdup( DEFAULT_HIDDEV_PATH );
    }
    
    if (!cfg.uinput_path)
    {
        cfg.uinput_path = strdup( DEFAULT_UINPUT_PATH );
    }
    

    // Install signal handler now...
    signal( SIGINT, handle_interrupt );
    

    // There are 54 packets of 8 bytes in size.
    const int packets_expected = 54;

    // Buffer for a single packet.
    unsigned char packet[ PACKET_SZ ];
    
    // Set up the button mappings...
    mapping_init();

    // Initialise button led stuff via HIDAPI:
    init_button_leds( cfg.vid, cfg.pid );
    
    // Animates the button state, eventually this 
    // should hilite only the mapped buttons and
    // keep the rest dark.
    if (cfg.animate)
    {
        fancy_button_leds();
    }
    
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
    
    // shift and octaves always lit
    for(int i=0; i<TOTAL_HID_BUTTONS; i++)
    {
        int light_it_up = (i == 0 || i == 19 || i == 20 || mapping_is_mapped(i)) ? 1 : 0;
        update_button_led( i, light_it_up );
        
        if (sync_button_leds() < 0)
        {
            printf( 
                "Opening the HID device %04x:%04x failed.\n"
                "Did you pass the right vendorId and productId? Does the current user have permissions?\n",
                cfg.vid, 
                cfg.pid
            );
            
            goto clean_up_and_exit;
        }
        
        usleep( 5000 );
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
    // in button press state. 0: down, 1: up, -1: initial
    char button_state[ MAX_BUTTONS ]; // button 0..39
    for(int b=0; b<MAX_BUTTONS; ++b)
    {
        button_state[ b ] = -1;
    }
    
    // for iterating
    int button_number;
    
    // For polling stuff...
    fd_set rfds;
    struct timeval tv;    
    int select_result;
    
    //
    int fd = open( cfg.hiddev_path, O_RDONLY );
    if (fd < 0)
    {
        perror( "hiddev open" );
        
        printf( 
            "Opening the device file at %s failed.\n"
            "Can it be read by the current user?\n",
            cfg.hiddev_path
        );
        
        return_code = -1;
        goto clean_up_and_exit;
    }
    
    // Set up uinput device.
    int fd_uinput = uinput_open( cfg.uinput_path );
    if (fd_uinput < 0)
    {
        close(fd);
        perror( "uinput open" );
        
        
        printf( 
            "Opening the device file at %s failed.\n"
            "Can it be read by the current user?\n",
            cfg.hiddev_path
        );
        
        return_code = -2;
        goto clean_up_and_exit;
    }

    int read_errors = 0;
    
    unsigned char position_4d_dial = 0; // to determine the way the dial goes
    // int value_4d_dial = 0;              // the real dial value that can be mapped to something?
    
    // read packets until interrupted
    while (read_aborted == 0)
    {
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
        
        // We appear to have data, so let's read
        // all the packets
        button_number = 0;
        for(int p=0; p < packets_expected; p++)
        {
            int bytes_read = read( fd, &packet, PACKET_SZ );
            if (bytes_read != PACKET_SZ)
            {
                printf( "read() did not return enough bytes.\n" );
                
                read_errors++;
                if (read_errors > 128)
                {
                    printf( "Too many errors, aborting.\n" );
                    read_aborted = 1;
                }
                
                break;
            }
            
#ifdef DUMP_KEYS_IN

            // Each packet has the format
            //  "0? 00 01 ff ?? ?? ?? ??"
            //
            //  "02 00 01 ff" Button state
            printf( "%4d: ", p );
            
            printf( "%02x ", packet[0] );
            printf( "%02x ", packet[1] );
            printf( "%02x ", packet[2] );
            printf( "%02x   ", packet[3] );
            
            printf( "%02x ", packet[4] );
            printf( "%02x ", packet[5] );
            printf( "%02x ", packet[6] );
            printf( "%02x ", packet[7] );
            
            printf( "\n" );
            
#endif /* DUMP_KEYS_IN */
            
            if (packet[1] == 0x00 
                && packet[2] == 0x01
                && packet[3] == 0xff)
            {
                if (packet[0] == 0x03 && p == 51)
                {
                    // This is the 4D dial that gets turned. This runs from 0x00 to 0x0F
                    // and we can only determine the direction by comparing to a previous
                    // value.
                    unsigned char new_dial_position = packet[4];
                    int dial_change = 0;
                    
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

#ifdef DUMP_KEYS_IN
                        if (dial_change > 0) {
                            printf( "4D dial INCREASE\n" );
                        } else if (dial_change < 0) {
                            printf( "4D dial DECREASE\n" );
                        }
#endif

                        // And release it...
                        send_key_wrap( fd_uinput, send_key, 0 );
                        
                        // 
                        position_4d_dial = new_dial_position;
                    }
                }
                else if (packet[0] == 0x02)
                {                    
                    const char previous_button_state = button_state[ button_number ];
                    const char new_button_state = (packet[4] == 0 
                        ? KEY_RELEASE 
                        : KEY_PRESS );
                    
                    if (new_button_state != previous_button_state)
                    {
                        button_state[ button_number ] = new_button_state;
                        
                        // We only print the button state if it has actually 
                        // been pressed before.
                        if (previous_button_state != KEY_INITIAL 
                            || new_button_state == KEY_PRESS)
                        {
                        
                            // BUTTON STATE
                            if (!cfg.quiet) 
                            {
                                printf( "%s (button %d): %s\n", 
                                        get_button_name(button_number), 
                                        button_number,  
                                        new_button_state ? "PRESS" : "RELEASE" 
                                );
                            }
                            
                            const mapping_key_t send_key = button_state[0] 
                                ? mapping_get_shifted( button_number )
                                : mapping_get( button_number );
                            
                            send_key_wrap( fd_uinput, send_key, new_button_state );   
                        }
                    }
                    
                    button_number++;
                }
            }
        }
    }
    
clean_up_and_exit:

    // clean-up stuff
    alsa_close_client();
    fancy_button_leds_off();
    
    if (cfg.uinput_path) free(cfg.uinput_path);
    if (cfg.hiddev_path) free(cfg.hiddev_path);
    if (cfg.mapping_path) free(cfg.mapping_path);
    
    if (fd>-1) close(fd);
    if (fd_uinput>-1) uinput_close(fd_uinput);
    
    fini_button_leds();
    
    return return_code;
}
