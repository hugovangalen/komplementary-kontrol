#include "button_leds.h"

static unsigned char button_hid_data[ 1 + TOTAL_HID_BUTTONS ]; // so 22 items, 21 max index

static int vid = 0; // vendor id
static int pid = 0; // product id

void clear_button_leds()
{
    memset(button_hid_data + 1,0,TOTAL_HID_BUTTONS);
}

int init_button_leds( int vendorid, int productid )
{
    vid = vendorid;
    pid = productid;
    
    // Initialise the hid data packet.
    button_hid_data[ 0 ] = 0x80;
    clear_button_leds();
    
    // Initialise HIDAPI.
    return hid_init();
}


int fini_button_leds()
{
    // Cleanup HIDAPI.
    return hid_exit();
}

void update_button_led_real( int index, char intensity )
{
    if (index < TOTAL_HID_BUTTONS)
    {
        button_hid_data[ index + 1 ] = intensity;
    }
}

void update_button_led( int index, int state )
{
    update_button_led_real( index, state ? 0x7c : 0x00 );
}


int sync_button_leds()
{
    if (!vid || !pid)
    {
        printf( "No USB device configured.\n" ); // %04x:%04x is in
        return -1;
    }
    
    // Attempt to open things...
    hid_device * device = hid_open( vid, pid, NULL );
    if (!device)
    {
        // printf( "hid_open() failed\n" );
        return -2;
    }
    
    // 
    int result = hid_write( device, button_hid_data, sizeof button_hid_data );
    hid_close( device );
}



/** 
 * The idea is that this animates button lights 0..20 (21 in total) in 
 * order with a slight delay.
 */
#define ANIMATE_COLUMNS     11
#define ANIMATE_DELAY       30000

#define ANIMWAIT            sync_button_leds(); usleep( ANIMATE_DELAY )

static char animation_sequence[ANIMATE_COLUMNS][5] = {
    {  0,  3,  6,  9, 19 },
    {  1,  4,  7, 10, 20 },
    {  2,  5,  8, 11, -1 },
    { -1, -1, -1, -1, -1 }, // pause between these columns
    { 12, 13, 14, -1, -1 },
    { 12, 13, 15, -1, -1 },
    { -1, -1, -1, -1, -1 }, // pause between these columns
    { -1, -1, -1, -1, -1 }, // pause between these columns
    { 16, -1, -1, -1, -1 },
    { 17, -1, -1, -1, -1 },
    { 18, -1, -1, -1, -1 }
};


void fancy_button_leds()
{   
/*
    // Wipe from Left to Right
    for(int i=0; i<ANIMATE_COLUMNS; i++)
    {
        for(int j=0; j<5; j++)
        {
            if (animation_sequence[i][j] >= 0)
            {
                update_button_led( animation_sequence[i][j], 1 );
            }
            
            if (i > 0)
            {
                if (animation_sequence[i-1][j] >= 0)
                {
                    update_button_led( animation_sequence[i-1][j], 0 );
                }
            }
        }
        
        ANIMWAIT;
    }
*/

    // Wipe from Right to Left
    for(int i=ANIMATE_COLUMNS-1; i>=0; i--)
    {
        for(int j=0; j<5; j++)
        {
            if (animation_sequence[i][j] >= 0)
            {
                update_button_led( animation_sequence[i][j], 1 );
            }
            
            if (i < ANIMATE_COLUMNS - 1)
            {
                if (animation_sequence[i+1][j] >= 0)
                {
                    update_button_led( animation_sequence[i+1][j], 0 );
                }
            }
        }
        
        ANIMWAIT;
    }
    
#ifdef SIMPLE_ORDER
    for(int i=0; i<TOTAL_HID_BUTTONS; i++)
    {
        update_button_led( i, 1 ); // turn on
        sync_button_leds();
        usleep( 155000 );
        
        update_button_led( i, 0 ); // turn off
        sync_button_leds();
    }
    
#endif

    // Finally turn them all off.
    clear_button_leds();
    sync_button_leds();
}



/*
 * This turns all the buttons off, from index 0 to 21.
 */
void fancy_button_leds_off()
{
    for(int i=0; i < TOTAL_HID_BUTTONS; i++)
    {
        update_button_led( i, 0 );
        
        sync_button_leds(); 
        usleep( 5000 );
    }        
}
