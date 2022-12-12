#include <string.h>
#include <assert.h>
#include "..\include\file_analize.hpp"
#include "..\lex_tree\tree.hpp"
#include "..\include\utilities.hpp"



static int clear_scanf (const char* source, char *value)
{
    return sscanf (source, " %s", value);
}

//--------------------------------------------------------------------------------------------------------------------

void Read_tree_from_file (const char *t_file_name, Root *tree_root)
{
    Text text = {};

    count_and_read (1, &t_file_name, &text);
    const char *temp_source = text.source;
    if (strchr (text.source, '{'))
    {
        temp_source = strchr (text.source, '{');

        if (!temp_source)
            printf ("syntax error");
        
        char *value = (char *) Safety_calloc (sizeof (char) * 5);

        if (!value)
            printf ("no memory");

        clear_scanf (temp_source + 1, value);
        tree_root->first_node = New_var (value, 0, 0);
        Read_node (temp_source + 1, tree_root->first_node);
    }
}

//--------------------------------------------------------------------------------------------------------------------

static const char* Read_leather (const char* source, Tree_node *parent, Tree_node** son)
{
    assert (source);
    assert (parent);
    assert (son);
    
    source = strchr (source, '{') + 1;

    char *value = (char *) Safety_calloc (sizeof (char) * 5);
    
    if (!value)
        printf ("no memory");

    clear_scanf (source, value);
    *son = New_var (value, 0, 0);
    if (strchr (source, '{') < strchr (source, '}'))
        source = Read_node (source, *son);

    return source;
}

//--------------------------------------------------------------------------------------------------------------------

const char *Read_node (const char *source, Tree_node* parent)
{
    assert (source);
    assert (parent);

    if (strchr (source, '{'))
        source = Read_leather (source, parent, &parent->right);

    if (strchr (source, '{') > strchr (source, '}'))
        source = Read_leather (source, parent, &parent->left);

    return source;

}

