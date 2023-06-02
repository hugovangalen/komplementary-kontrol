#include "konfigure_parser.h"
#include "linux/limits.h" 

// How much space to allocate for the line buffer when
// parsing configuration. 64 is probably more than enough.
#define LINE_BUFSZ          256


/*
 * This simply clears it out, making it unassigned and
 * clearing the label.
 */
void preset_clear( t_preset_button * button )
{
    memset( button, 0, sizeof(t_preset_button) );
    memset( button->label, 0x20, NAME_SZ ); // pad with spaces
}


/*
 * Parse a single line of definition, so the part after the = 
 * sign in "button0/0=label,type,etc"
 * 
 * returns -1 a parameter is out of range.
 */
int preset_parse( t_preset_button * button, const char * line, int verbose )
{
    const size_t len = strlen(line);
    preset_clear( button );
    
    if (len > 0)
    {
        size_t start = 0;
        size_t position = 0;
        size_t token_length;
        
        int phase = 0; // finding label
        
        // Note that we intentionally seek until index `len` so 
        // we encounter the NULL termination.
        while (position <= len)
        {
            int got_token = 0;
            
            token_length = position - start;
            switch (line[position])
            {
                case '\0':
                case ',': // separator or end of line, so anything before this
                    if (verbose) printf( "phase %d, debug %s\n", phase, line+start );
                    int to_int_value = atoi(line+start);
                    if (phase > 0 && to_int_value > 127)
                    {
                        printf( "A value is out of range.\n" );
                        return -1;
                    }
                    
                    switch(phase)
                    {
                        case 0: // the label ended before this comma
                            if (token_length > NAME_SZ) token_length = NAME_SZ;
                            memcpy( button->label, line+start, token_length );
                            got_token = 1;
                            break;
                            
                        case 1: // the type was before this comma
                            button->type = to_int_value;
                            got_token = 1;
                            break;
                            
                        case 2: // channel
                            button->channel = to_int_value;
                            got_token = 1;
                            break;
                            
                        case 3: // data1, data2, data3, data3
                        case 4:
                        case 5:
                        case 6:
                            button->data[ phase - 3 ] = to_int_value;
                            got_token = 1;                            
                            break;
                    }
                    break;
            }
            
            // next character
            position++;
            
            if (got_token)
            {
                // set up to read the next token
                start = position;
                
                if (phase == 6)
                {
                    phase = 0;
                }
                else
                {
                    phase++;
                }
            }            
        }

        button->assigned = 1;
        
        // Debug...
        if (verbose) 
        {
            printf( "Button LABEL=%s, TYPE=%d, CHANNEL=%d, DATA=%02x %02x %02x %02x\n",
                button->label,
                button->type,
                button->channel,
                button->data[0],
                button->data[1],
                button->data[2],
                button->data[3]
            );
        }            
    }
    
    return 1;
}


void preset_clear_config( t_preset_config * preset )
{
    memset( preset, 0, sizeof(t_preset_config) );
    memset( preset->label, 0x20, NAME_SZ );
}



/* Internally used to strip invalid characters of the 
 * end of an argument.
 */
static void __cleanup_text( char * righthand )
{
    size_t righthand_idx = strlen(righthand) - 1;
    
    // strip off any newlines, spaces, whatever
    int clean_righthand = 1;
    while (righthand_idx >= 0 && clean_righthand)
    {
        switch(righthand[ righthand_idx ])
        {
            case '\n':
            case ' ':
            case '\r':
                righthand[ righthand_idx ] = 0;
                righthand_idx--;
                break;
                
            default:
                // seems okay
                clean_righthand = 0;
                break;
        }
    }
}


/* 
 * Parses the configuration file and attempts to configure. If the file 
 * cannot be found in the path, it will check `PRESETS_PATH`.
 * 
 * Returns -1 on error, otherwise the number of buttons succesfully read.
 */
int preset_parse_config( const char * path, t_preset_config * preset, int verbose )
{
    int result = -1;
    FILE *fp = NULL;
    
    // if (verbose) printf( "Resetting preset data.\n" );
    preset_clear_config( preset );
    
    // Put in a default name, so we can use that if it is missing.
    memcpy( preset->label, "Unnamed   ", NAME_SZ );
    
    if (access( path, F_OK ) != 0)
    {
    
#ifdef PRESETS_PATH
        // Try to seek the default presets path.
        char temp_path[ PATH_MAX ];
        memset( temp_path, 0, PATH_MAX );
        snprintf( temp_path, PATH_MAX, PRESETS_PATH "/%s", path );
        
        if (access( temp_path, F_OK ) != 0)
        {
            if (verbose) printf( "The file `%s` could not be found in the current path, nor in the presets folder " PRESETS_PATH ".\n", path );
            return -1;
        }
        
        if (verbose) printf( "Parsing `%s`...\n", temp_path );
        fp = fopen( temp_path, "r" );
#else
        printf( "The file `%s` could not be found.\n", path );        
#endif /* PRESETS_PATH */
        
    }
    else
    {
        if (verbose) printf( "Parsing `%s`...\n", path );
        fp = fopen( path, "r" );
    }
    
    if (fp == NULL)
    {
        if (verbose) printf( "The file `%s` could not be opened. Check the permissions.\n", path );
        return -1;
    }
    
    char buffer[ LINE_BUFSZ ];
    int line_counter = 0;
    
    // alright, we could at least parse *something*.
    result = 0;
    while (fgets( buffer, LINE_BUFSZ, fp ) != NULL)
    {
        line_counter++;
        
        // We have a buffer with a value like 'name=something' or 'button9/9=something',
        // so we need to figure out what.
        if (strncasecmp( "name=", buffer, 5 ) == 0)
        {
            // There is a name, so let's use that.
            char * name = buffer + 5;
            __cleanup_text( name );
            
            size_t name_length = strlen( name );
            if (name_length > NAME_SZ) name_length = NAME_SZ;
            
            memcpy( preset->label, name, name_length );
            
            if (verbose) printf( "Name `%s`\n", preset->label );   
        }
        else if (strncasecmp( "button", buffer, 6 ) == 0)
        {
            // This *could* be a Button0/0 assignment, so let's 
            // figure that out.
            char * equals = strstr( buffer+6, "=" );
            char * slash = strstr( buffer+6, "/" );
            if (slash != NULL && equals != NULL)
            {
                int page_index = atoi( buffer + 6 ); // before the /
                int button_index = atoi( slash + 1 ); // between / and =
                
                int index = (page_index * 8) + button_index;
                if (index > 31)
                {
                    printf( "Line #%d: The button %d / %d is out of range.\n", line_counter, page_index, button_index );
                    continue;
                }
                
                // printf( "Line #%d: Button %d / %d -> %d\n", line_counter, page_index, button_index, index );
                char * righthand = equals + 1;
                __cleanup_text( righthand );
                
                if (preset->button[ index ].assigned)
                {
                    printf( "Line #%d: Warning, the button %d / %d is being re-assigned.\n",
                            line_counter,
                            page_index,
                            button_index 
                    );
                }
                
                if (verbose)
                {
                    printf( "Page %d, button %d, index=%d, `%s`\n", page_index, button_index, index, righthand );
                }
                
                // so this is the actual length now.
                size_t righthand_len = strlen(righthand);
                if (righthand_len > 0)
                {
                    // Attempt to parse it.
                    int line_parse_result = preset_parse( &preset->button[index], righthand, verbose );
                    if (line_parse_result == 1)
                    {   
                        preset->button[ index ].assigned = 1;
                        result++;
                    }
                    else if (line_parse_result == -1)
                    {
                        printf( "Line #%d: A value is out of range.\n", line_counter );
                    }
                }
                else
                {
                    // this is not assigned.
                    if (verbose) printf( "Button%d/%d is not assigned.\n", page_index, button_index );
                    preset->button[ index ].assigned = 0;
                }
            }
        }        
    }
    
    
cleanup_and_return:    
    fclose(fp);
    fp = NULL;
    
    return result;
}
