#include "mapping.h"


static mapping_key_t mapping[ REAL_BUTTON_TOTAL ];
static mapping_key_t shifted_mapping[ REAL_BUTTON_TOTAL ];

void mapping_init()
{
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
    mapping[ index ] = key;
}

void mapping_set_shifted( int index, mapping_key_t key )
{
    shifted_mapping[ index ] = key;
}

mapping_key_t mapping_get( int index )
{
    return mapping[ index ];
}

mapping_key_t mapping_get_shifted( int index )
{
    return shifted_mapping[ index ];
}


/*
 * @returns 1 if it is in any way mapped, 0 otherwise.
 */
int mapping_is_mapped( int index )
{
    return (mapping[ index ].length > 0
        || shifted_mapping[ index ].length > 0) ? 1 : 0;
        
}
