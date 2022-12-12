#include <ctype.h>
#include <sys\stat.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include "..\include\file_analize.hpp"

//--------------------------------------------------------------------------------------------------------------------

static size_t file_size_count (const char * file_name)
{
    assert (file_name);

    struct stat buff = {};
    buff.st_size = 0;

    stat(file_name, &buff);

    return buff.st_size;
}

//--------------------------------------------------------------------------------------------------------------------

void counting_strings (Text * text)
{
    assert (text);

    text->source [text->number_of_symbols] = '\0';
    char * point = text->source;
    
    while ((point = strchr (point, '\n')) != NULL)
    {
        if (*(point - 1) == 13)
            *(point - 1) = ' ';
        
        *point = '\0';
        point++;
        text->number_of_strings++;
    }
    text->number_of_strings++;

}

//--------------------------------------------------------------------------------------------------------------------

ERRORS_CODE count_and_read (int argc, const char *argv[], Text * text)
{
    assert (text);
    assert (argv);
    const char *file_name = argv[argc - 1];

    FILE * input_file = fopen (file_name, "rb");

    if (!input_file)
        return FALL;

    size_t file_size = file_size_count (file_name);
    
    text->source = (char *) calloc ((file_size + 2), sizeof (char));

    if (text->source == NULL)
        return NO_MEM_ERROR;
    
    text->number_of_symbols = fread (text->source, sizeof(char), file_size, input_file);

    if (file_size != text->number_of_symbols)
        return FALL;

    counting_strings (text);
    
    if (fclose (input_file) != 0)
    {
        printf ("close file %s error", file_name);
        return FALL;
    }

    return SUCCESS;
}

//--------------------------------------------------------------------------------------------------------------------

void create_pointers (Text * text)
{
    assert (text);

    text->string = (String *) calloc (text->number_of_strings + 1, sizeof (String));
    size_t str_index = 0;
    char * point = text->source;

    text->string [str_index].point = point;
    while (str_index++ < text->number_of_strings)
    {
        point += strlen (point) + 1;
        text->string [str_index].point = point;
        text->string [str_index - 1].len = point - text->string [str_index - 1].point - 2;
    }
}