#include "button_leds.h"

/*
 * Buffer for the HID data that is sent to the device to 
 * change the state.
 */
static unsigned char button_hid_data[ 1 + TOTAL_HID_BUTTONS ]; // so 22 items, 21 max index



/*
 * Clear out all leds to 'off'
 */
void leds_clear()
{
    memset(button_hid_data + 1,0,TOTAL_HID_BUTTONS);
}



/* 
 * This initialises the internal buffer that is sent
 * over the wire as HID data.
 */
void leds_init()
{
    // Initialise the hid data packet.
    button_hid_data[ 0 ] = 0x80;
    leds_clear();
}



/*
 * Updates a single LED. State is either LED_OFF, LED_ON
 * or LED_BRIGHT.
 * 
 * This still requires a call to `leds_sync()` to actually
 * send it over to the device.
 */
void leds_update_led( int index, int state )
{
    if (index < TOTAL_HID_BUTTONS)
    {
        button_hid_data[ index + 1 ] = state;
    }
}


/*
 * Syncs the LED state.
 */
int leds_sync()
{
    char receive_buffer[ 22 ];
    return hidstuff_send_raw( 
        button_hid_data, 
        sizeof button_hid_data,
        receive_buffer,
        0
    );
}


/** 
 * The idea is that this animates button lights 0..20 (21 in total) in 
 * order with a slight delay.
 */
#define ANIMATE_COLUMNS     11
#define ANIMATE_DELAY       30000

#define ANIMWAIT            leds_sync(); usleep( ANIMATE_DELAY )

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


void leds_animate_on()
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
                leds_update_led( animation_sequence[i][j], LED_ON );
            }
            
            if (i < ANIMATE_COLUMNS - 1)
            {
                if (animation_sequence[i+1][j] >= 0)
                {
                    leds_update_led( animation_sequence[i+1][j], LED_OFF );
                }
            }
        }
        
        ANIMWAIT;
    }
    
#ifdef SIMPLE_ORDER
    for(int i=0; i<TOTAL_HID_BUTTONS; i++)
    {
        leds_update_led( i, LED_ON ); // turn on
        leds_sync();
        usleep( 155000 );
        
        leds_update_led( i, LED_OFF ); // turn off
        leds_sync();
    }
    
#endif

    // Finally turn them all off.
    leds_off();
}



/*
 * This turns all the buttons off, from index 0 to 21 with 
 * a slight delay in between.
 */
void leds_animate_off()
{
    for(int i=0; i < TOTAL_HID_BUTTONS; i++)
    {
        leds_update_led( i, LED_OFF );
        leds_sync(); 
        
        usleep( 5000 );
    }        
}

/*
 * Turns all LEDs off, without any delay.
 */
void leds_off()
{
    leds_clear();
    leds_sync();
}
