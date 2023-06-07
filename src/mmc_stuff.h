#ifndef _MMC_STUFF_H_
#define _MMC_STUFF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"

#define MMC_STOP            0x01
#define MMC_PLAY            0x02
#define MMC_FORWARD         0x04
#define MMC_REWIND          0x05

#define MMC_RECORD_STROBE   0x06
#define MMC_RECORD_EXIT     0x07
#define MMC_RECORD_PAUSE    0x08

int mmc_key_parse( char * key );
const char * mmc_key_name( int command );

#endif /* _MMC_STUFF_H_ */
