#include <linux/uinput.h>

#ifndef _MAPPING_H_
#define _MAPPING_H_

// BUTTON_TOTAL is in here.
#include "button_names.h"

// The maximum keys a button can trigger.
#define MAX_KEYS 4

#define MAPPING_TYPE_KEY        0
#define MAPPING_TYPE_MMC        1

typedef struct mapped_key_t {
    unsigned char type;
    int key;
} mapped_key_t;

typedef struct mapping_key_t {
    int type;
    int length;
    mapped_key_t keys[ MAX_KEYS ];
} mapping_key_t;

#define MAP_MMC_KEY(code)   (mapped_key_t){.type=MAPPING_TYPE_MMC, .key=code}
#define MAP_KEY(code)       (mapped_key_t){.type=MAPPING_TYPE_KEY, .key=code}

void mapping_init();

void mapping_set( int index, mapping_key_t key );
void mapping_set_shifted( int index, mapping_key_t key );

mapping_key_t mapping_get( int index );
mapping_key_t mapping_get_shifted( int index );

int mapping_is_mapped( int index, int shifted );

#endif /* _MAPPING_H_*/
