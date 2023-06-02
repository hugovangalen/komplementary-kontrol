#include "config.h"
#include <linux/limits.h>

// The line size in the configuration file.
#define BUFFER_SZ 256

/*
 * Read configuration file. The file consists of simple
 * mapping:
 * 
 * Button0=LeftCtrl,Z
 * Button39=LeftCtrl,LeftShift,Z
 */
int config_read( char * filename, int verbose )
{
    int config_read_result = 0;
    
    char buffer[ BUFFER_SZ ];    
    int  keys[ 4 ];
    
    int  buflen = 0;
    int  keylen = 0;

    char c;
    
    int fd;
    
    if (access( filename, F_OK ) != 0)
    {    
#ifdef MAPPINGS_PATH
        // A mappings path folder has been defined, so let's
        // search for it there.
        char temp_path[ PATH_MAX ];
        memset( temp_path, 0, PATH_MAX );
        snprintf( temp_path, PATH_MAX, MAPPINGS_PATH "/%s", filename );
        
        if (access( temp_path, F_OK ) == -1)        
        {
            if (verbose) printf( "The file `%s` could not be found in the current path, nor in the mappings folder " MAPPINGS_PATH ".\n", filename );
            return -1;

            return -1;
        }
        
        // This file seems accessible, so let's use that.
        fd = open( temp_path, O_RDONLY );
#else
        // The file is not accessible. 
        return -1;
        
#endif /* MAPPINGS_PATH */
        
    }
    else
    {
        // This file is accessible...
        fd = open( filename, O_RDONLY );
    }
    
    if (fd < 0) return -2;

    // The lines are in no specific order (Button39 could be defined before Button0),
    // so we just read each line.
    int end_of_file = 0;
    
    memset( buffer, 0, sizeof buffer );
    memset( keys,   0, sizeof keys );
    
    int phase = 0; // 0 = reading button name, 1 = reading first key, 2 = reading second, etc.
    
    int button_index = -1;
    
    // The "Shift+Mute" can also be used as input.
    int shifted = 0; // keyboard button is shifted
    int comment = 0; // line is a comment
    
    int line_counter = 1;
    
    // For parsing / storing the single mapping configuration.
    mapping_key_t mapping;
    mapping.length = 0;
    
    while (!end_of_file)
    {
        // We just read 1 character at a time.
        int read_result = read( fd, &c, 1 );
        if (read_result == -1)
        {
            // This is an error situation.
            config_read_result = -1;
            goto cleanup_and_return;
        }
        else if (read_result == 0)
        {
            // The end of file has been encountered, so bail, though
            // if the last line is an assignment and we do have a button_index,
            // we need to parse the last key for that.
            
            if (button_index > -1) 
            {
                end_of_file = 1;
                
                // This is checked in parse_key to know this is the last item 
                // in the list of keys.
                c = '\n';
                goto parse_key;
            }
            else
            {
                // Not reading a button, so bail.
                break;
            }
        } 
        
        // We have read a character.
        
        // If `button_index` is still less than zero,
        // then we are still waiting to parse a button
        // identifier.
        if (button_index == -1)
        {
            if (c == '\n')
            {
                // This is an empty line, or the end of a comment.
                line_counter++;
                
                comment = 0;
                shifted = 0;
                
                goto restart_buffer;
            }
            else if (!comment && buflen == 0 && c == ' ')
            {
                // This line starts wrong, assume comment.
                printf( "Bad formatting on line %d\n", line_counter );
                // line_counter++;
                comment = 1;
                continue;
            }
            else if (buflen == 0 && c == '#')
            {
                // This is a comment, keep on reading and ignoring until
                // end of line.
                // line_counter++;
                // printf( "#comment on line %d\n", line_counter );
                comment = 1;
                continue;
            }
            
            if (!comment)
            {
                if (c == '=')
                {
                    // An assignment, so everything before this was the
                    // keyboard button number (Button0, or one of the alternative
                    // names, Undo, Plug-In, etc.).
                    //
                    // So we need to find a match for this.                
                    button_index = get_button_index( buffer );
                    
    //                printf( "buffer = `%s`, button_index = %d\n", buffer, button_index );
                    
                    // An assignment, so what comes after are the button names.
                    goto restart_buffer;
                }     
                else if (c == '+')
                {
                    // This could be "Shift+Mono" which can have
                    // a different mapping.
                    size_t len = strlen(buffer);
                    if (strncasecmp( "Shift", buffer, len ) == 0)
                    {
                        shifted = 1;
                        goto restart_buffer;
                    }
                }
            }
        }
        else
        {
            if (c == '+' || c == ',' || c == '\n')
            {
                // We have found a separator, so the button 
                // description is in the buffer. 
                goto parse_key;
            }
        }
            
        // Add to the buffer and continue.
        buffer[ buflen ] = c;
        buflen++;
        
        continue;
        
parse_key:
        // First attempt to parse normal keys, and only attempt to 
        // match MMC key if we didn't find it.
        const int key_code = key_parse( buffer );
        const int mmc_code = key_code == -1 ? mmc_key_parse( buffer ) : -1;
        
        if (key_code == -1 && mmc_code == -1)
        {
            printf( "Failed to parse key `%s` at line %d\n", buffer, line_counter );
        }
        else if (mmc_code > -1)
        {
            // mapping.type = MAPPING_TYPE_MMC;
            mapping.keys[ mapping.length ] = MAP_MMC_KEY(mmc_code);
            mapping.length++;
        }
        else
        {
            //mapping.type = MAPPING_TYPE_KEY;
            mapping.keys[ mapping.length ] = MAP_KEY(key_code);
            mapping.length++;
        }
        
        if (c == '\n')
        {   
            // The end of the line. If we have parsed a mapping, 
            // we should assign it.
            if (shifted) 
            {
                mapping_set_shifted( button_index, mapping );
            }
            else
            {
                mapping_set( button_index, mapping );
            }
            
            // Verbosity.
            if (verbose)
            {
                printf( "Button `%s%s` mapped to ", (shifted ? "Shift+" : ""), get_button_name( button_index ) );
                for(int ki=0; ki<mapping.length; ki++)
                {
                    if (mapping.keys[ki].type == MAPPING_TYPE_KEY)
                    {
                        printf( "%s", key_name( mapping.keys[ki].key ) );
                    }
                    else
                    {
                        printf( "%s", mmc_key_name( mapping.keys[ki].key ) );
                    }
                    if (ki < mapping.length-1)
                    {
                        printf( "+" );
                    }
                }
                
                printf( "\n" );
            }
            
            // Next line...
            line_counter++;
            
            // Scan for the next button.
            mapping.length = 0;
            button_index = -1;
            
            shifted = 0;
            comment = 0;
        }
    
restart_buffer:
        // Special handling to clear the buffer and 
        // read the next token.
        memset( buffer, 0, sizeof buffer );
        buflen = 0;        
    }
    
cleanup_and_return:
    // Clean up and return the collected value.
    close(fd);
    return config_read_result;
    
}
