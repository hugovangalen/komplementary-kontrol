#include "button_names.h"

/* The "nice" button names (Button0..Button39 would also still be supported
 * in the code).
 */
static const char * button_names[ REAL_BUTTON_TOTAL ] = {
    "Shift",            
    "Scale",            
    "Arp",              
    "Undo",
    "Quantize",
    "Ideas",
    "Loop",
    "Metro",
    "Tempo",
    "Play",
    "Record",
    "Stop",
    "Preset Up",
    "Preset Down",
    "Mute",
    "Solo",
    "Browser",
    "Plug-In",
    "Track",
    "Octave Down",
    "Octave Up",
    "4D Up",
    "4D Right",
    "4D Left",
    "4D Down",
    "Rotary1",
    "Rotary2",
    "Rotary3",
    "Rotary4",
    "Rotary5",
    "Rotary6",
    "Rotary7",
    "Rotary8",
    "4D Button",
    "Button34",
    "Button35",
    "Button36",
    "Button37",
    "Button38",
    "Button39",
    
    // Internally handled differently than the rest.
    "4D CW",        // clockwise rotation on 4D dial
    "4D CCW"        // counter-clockwise rotation
};

// const char * invalid_button = "**INVALID**";

const char * get_button_name( int index )
{
    if (index >= 0 && index < REAL_BUTTON_TOTAL)
    {
        return button_names[ index ];
    }
    
    return INVALID_KEY_OR_BUTTON;
}


/**
 * Attempts to resolve the button index depending
 * on the given string. This ignores case.
 * 
 * If the button cannot be interpreted, this returns -1.
 */
int get_button_index( char * name )
{
    size_t len = strlen( name );
    char tempname[ BUTTON_BUFFER_SZ ];
    
    for(int b=0; b<REAL_BUTTON_TOTAL; ++b)
    {
        // Alternative names are also supported.
        snprintf( tempname, BUTTON_BUFFER_SZ, "Button%d", b );

        if ((strncasecmp( name, button_names[ b ], len ) == 0) || 
            (strncasecmp( name, tempname, len ) == 0))
        {
            return b;
        }        
    }
    
    // not found...
    return -1;
}
