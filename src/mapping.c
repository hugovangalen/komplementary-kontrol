#include "mapping.h"

// The null key is sent when invalid indices are used, so we can 
// return a 0-length mapping.
static mapping_key_t null_key;

static mapping_key_t mapping[ REAL_BUTTON_TOTAL ];
static mapping_key_t shifted_mapping[ REAL_BUTTON_TOTAL ];

void mapping_init()
{
    memset( &null_key, 0, sizeof null_key );
    
    for(int i=0; i<REAL_BUTTON_TOTAL; i++)
    {
        mapping[i].length = 0;
        shifted_mapping[i].length = 0;
    }
}

/* Interestingly, the button index seems to correlate with the button lights 
 * order (at least for 0..21) so that allows us to light only those buttons
 * with actual mappings.
*/
void mapping_set( int index, mapping_key_t key )
{
    if (index >= 0 && index < REAL_BUTTON_TOTAL)
        mapping[ index ] = key;
}

void mapping_set_shifted( int index, mapping_key_t key )
{
    if (index >= 0 && index < REAL_BUTTON_TOTAL)
        shifted_mapping[ index ] = key;
}

mapping_key_t mapping_get( int index )
{
    if (index >= 0 && index < REAL_BUTTON_TOTAL)
        return mapping[ index ];
    
    return null_key;
}

mapping_key_t mapping_get_shifted( int index )
{
    if (index >= 0 && index < REAL_BUTTON_TOTAL)
        return shifted_mapping[ index ];
    
    return null_key;
}


/*
 * @returns 1 if it is in any way mapped, 0 otherwise.
 */
int mapping_is_mapped( int index, int shifted )
{
    if (index >= 0 && index < REAL_BUTTON_TOTAL)
    {
        if (shifted == 1) {
            // mappings with shift only
            return (shifted_mapping[ index ].length > 0) ? 1 : 0;
            
        } else if (shifted == -1) {
            // only normal mappings without shift
            return (mapping[ index ].length > 0) ? 1 : 0;
        }

        // any mapping at all
        return (mapping[ index ].length > 0
                || shifted_mapping[ index ].length > 0) ? 1 : 0;
    }
    
    return 0;
}
