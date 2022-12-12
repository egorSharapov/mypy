#include <stdio.h>
#include <malloc.h>
#include "..\include\reader.hpp"
#include "..\include\utilities.hpp"
#include "..\lex_tree\tree.hpp"
#include "..\include\read_st.hpp"

const char *assemble_file_name = "out\\assemble.txt";

static void print_tokens (Tree_tokens *tokens)
{
    printf ("%lld\n", tokens->capacity);
    for (size_t index = 0; index < tokens->capacity; index++)
    {
        printf ("tp: %2d ", tokens->array[index]->type);
        Tree_node *elem = tokens->array[index];
        switch (elem->type)
        {
        case OPERATOR:
        {
            printf ("| op: %s", convert_graph_op(elem->value.op));
            break;
        }
        case NUMBER:
        {
            printf ("| num:%02lg", elem->value.num);
            break;
        }
        case IDENTIFIER:
        {
            printf ("| var: %s", elem->value.id);
            break;
        }
        default:
            printf ("|non   ");
            break;
        }
        printf ("\n");
    }

}

static void Print_var_table (Var_table *variables)
{
    for (size_t index = 0; index < variables->size; index++)
    {
        printf ("variable: %s\n", variables->table[index].name);
    }
}

static void Print_tables (Translator *translator)
{
    printf ("\nglobal:\n");
    Print_var_table (translator->variables);

    for (size_t index = 0; index < translator->functions->size; index++)
    {
        printf ("function %s:\n", translator->functions->table[index].name);
        Print_var_table (translator->functions->table[index].variables);
    }
}

int main (int argc, const char *argv[])
{
    Text prog_code = {};
    Root tree_root = {};
    Translator translator = {};

    Translator_ctor (&translator);

    count_and_read (argc, argv, &prog_code);
    create_pointers (&prog_code);

    Tree_tokens *tokens = Tokenizer (&prog_code);

    print_tokens (tokens);

    tree_root.first_node =  Get_grammar (tokens, &translator);    
    Graph_print_tree (&tree_root);

    Translate_to_assembler (assemble_file_name, &tree_root, &translator);
    Print_tables (&translator);
}



