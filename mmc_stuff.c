#include "mmc_stuff.h"


//#define MMC_STOP            0x01
//#define MMC_PLAY            0x02
//#define MMC_FORWARD         0x04
//#define MMC_REWIND          0x05
//#define MMC_RECORD_STROBE   0x06
//#define MMC_RECORD_EXIT     0x07
//#define MMC_RECORD_PAUSE    0x08

static size_t KEY_MAX = 9;
static const char * KEY_STRINGS[] = {
    NULL,
    "MMC_Stop",
    "MMC_Play",
    NULL,
    "MMC_Forward",
    "MMC_Rewind",
    "MMC_Record_Strobe",
    "MMC_Record_Exit",
    "MMC_Record_Pause"
};

/*
 * Returns the MMC command value, or -1 if
 * it isn't found.
 */
int mmc_key_parse( char * key )
{
    size_t len = strlen(key);    
    for(int i=0; i < KEY_MAX; i++)
    {
        if (KEY_STRINGS[i] != NULL)
        {
            if (strncasecmp( key, KEY_STRINGS[i], len ) == 0)
            {
                return i;
            }
        }
    }
    
    return -1;
}

const char * mmc_key_name( int command )
{
    if (command > 0 
        && command < KEY_MAX
        && KEY_STRINGS[ command ] != NULL) 
    {
        return KEY_STRINGS[command];
    }
    
    return INVALID_KEY_OR_BUTTON;
}
