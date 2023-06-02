#ifndef _KONFIGURE_PARSER_H_
#define _KONFIGURE_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// Button types that can be assigned to rotary dials.
#define TYPE_CC         0x00
#define TYPE_PGM        0x01

// Button label (and preset name) size.
#define NAME_SZ           10

// The total pages times the number of buttons:
#define MAX_PRESET_BUTTONS 4 * 8


/* 
 * The structure that holds the button configuration. 
 */
typedef struct t_preset_button {
    unsigned char assigned; 
    
    char label[ NAME_SZ + 1]; // + 1 for NULL for convencience when printing the values.    
    unsigned char type;
    unsigned char channel;
    unsigned char data[ 4 ];

} t_preset_button;


/*
 * The structure that holds the entire preset.
 */
typedef struct t_preset_config {
    char label[ NAME_SZ + 1 ]; // + 1 for terminator for convenience when printing value.
    t_preset_button button[ MAX_PRESET_BUTTONS ];
} t_preset_config;



void preset_free( t_preset_button * button );
void preset_clear( t_preset_button * button );

int preset_parse( t_preset_button * button, const char * line, int verbose );

int preset_parse_config( const char * path, t_preset_config * preset, int verbose );
void preset_clear_config( t_preset_config * preset );

#endif /* _KONFIGURE_PARSER_H_*/
