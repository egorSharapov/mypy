#include <string.h>
#include <assert.h>
#include "..\include\translator.hpp"
#include "..\include\utilities.hpp"
#include "..\lex_tree\tree.hpp"



const char *Convert_to_assemble_op (OPERATORS op_value)
{
    if (op_value == OP_ADD)
        return "ADD";
    if (op_value == OP_SUB)
        return "SUB";
    if (op_value == OP_MUL)
        return "MUL";
    if (op_value == OP_DIV)
        return "DIV";
    
    return NULL;
}

Var_table *Var_table_ctor (void)
{
    Var_table *vars = (Var_table *) calloc (1, sizeof (Var_table));
    vars->size = 0;
    vars->capacity = 1;
    vars->table = (Variable *) calloc (1, sizeof (Variable));

    return vars;

}

void Var_table_push (Var_table *vars, const char *name)
{  
    Var_table_resize (vars); 
    vars->table[vars->size++].name = name;
}

void Var_table_resize (Var_table *variables)
{
    if (variables->size == variables->capacity)
    {
        variables->capacity *= 2;

        variables->table = (Variable *) recalloc (variables->table, variables->capacity, sizeof (Variable));
    }
}

void Fun_table_resize (Fun_table *functions)
{
    if (functions->size == functions->capacity)
    {
        functions->capacity *= 2;

        functions->table = (Function *) recalloc (functions->table, functions->capacity, sizeof (Function));
    }
}

bool Is_in_fun_table (Fun_table *functions, const char *func_name)
{
    for (size_t index = 0; index < functions->size; index++)
    {
        if (strcmp (functions->table[index].name, func_name) == 0)
            return 1;
    }
    return 0;
}

bool Is_in_var_table (Var_table *variables, const char *var_name)
{
    for (size_t index = 0; index < variables->size; index++)
        if (strcmp (variables->table[index].name, var_name) == 0)
            return 1;
    return 0;
}

static int Find_var_index (Var_table *variables, const char *var_name)
{
    for (int index = 0; index < (int) variables->capacity; index++)
        if (strcmp (variables->table[index].name, var_name) == 0)
            return index;

    return -1;
}


void Translator_ctor (Translator *translator)
{
    translator->variables = (Var_table *) calloc (1, sizeof (Var_table));
    translator->variables->size = 0;
    translator->variables->capacity = 1;
    translator->variables->table = (Variable *) calloc (1, sizeof (Variable));

    translator->functions = (Fun_table *) calloc (1, sizeof (Fun_table));
    translator->functions->size = 0;
    translator->functions->capacity = 1;
    translator->functions->table = (Function *) calloc (1, sizeof (Function));

}

static int if_number = 0;
static int loop_number = 0;
static int local_level = 1;
static int var_mem_size = 50;

void Translate_to_assembler (const char *assemble_file_name, Root *tree_root, Translator *translator)
{
    FILE *assemble_file = fopen (assemble_file_name, "wb");

    if (!assemble_file)
        printf ("open assemble file error");
    
    fprintf (assemble_file, "PUSH 0\nPOP ax\n\n");
    Translate_statement (assemble_file, tree_root->first_node, translator->variables);
    fprintf (assemble_file, "HLT\n");

    Translate_functions (assemble_file, translator);
    fclose (assemble_file);

}

void Translate_statement (FILE *assemble_file, Tree_node *node, Var_table *variables)
{
    if (IS_OP (OP_IF))
        Translate_if (assemble_file, node, variables);
    else if (IS_OP (OP_ST))
    {
        Translate_statement (assemble_file, node->left, variables);
        Translate_statement (assemble_file, node->right, variables);
    }
    else if (IS_OP (OP_ASSGN) or IS_OP (OP_ADD_ASSIGN) or IS_OP (OP_SUB_ASSIGN))
        Translate_assigment (assemble_file, node, variables);
    
    else if (IS_OP (OP_WHILE))
        Translate_loop (assemble_file, node, variables);
    
    else if (IS_OP (OP_CALL))
        Translate_call (assemble_file, node, variables);
    
    else if (IS_OP (OP_PRINT))
        Translate_print (assemble_file, node, variables);
    
    else if (IS_OP (OP_FUNC))
        return;
    
    else if (IS_OP (OP_RETURN))
    {
        Translate_expression (assemble_file, node->left, variables);
        fprintf (assemble_file, "RET\n\n");
    }
    else
        Translate_expression (assemble_file, node, variables);

}


static void Translate_string_const (FILE *assemble_file, Tree_node *node)
{
    const char *temp_string = node->value.id;
    printf ("%s\n", temp_string);
    while (*temp_string != '\0')
    {
        fprintf (assemble_file, "PUSH %d\n", *temp_string);
        fprintf (assemble_file, "COUT\n");
        temp_string += 1;
    }
}

void Translate_print (FILE *assemble_file, Tree_node *node, Var_table *variables)
{
    if (node->right)
        Translate_string_const (assemble_file, node->right);

    if (node->left)
        Translate_expression (assemble_file, node->left, variables);

    fprintf (assemble_file, "OUT\n");
}

void Translate_functions (FILE *assemble_file, Translator *trans_table)
{
    Fun_table *functions = trans_table->functions;
    local_level = 2;
    for (size_t index = 0; index < functions->size; index++)
    {
        Translate_function (assemble_file, functions->table[index].fun_pointer, functions->table[index].variables);
    }
}

void Translate_call (FILE *assemble_file, Tree_node *node, Var_table *variables)
{
    Tree_node *params = node->left->left;
    if (params)
    {
        Translate_statement (assemble_file, params, variables);
    }
    if (local_level == 2)
        fprintf (assemble_file, "\nPUSH ax\nPUSH %lld\nADD\nPOP ax\n", variables->size);
    else 
        fprintf (assemble_file, "PUSH %d\nPOP ax\n", var_mem_size);
    fprintf (assemble_file, "CALL func_%s\n", node->left->value.id);
    if (local_level == 2)
        fprintf (assemble_file, "PUSH %lld\nPUSH ax\nSUB\nPOP ax\n\n", variables->size);
    else
        fprintf (assemble_file, "PUSH 0\nPOP ax\n");

}

void Translate_function (FILE *assemble_file, Tree_node *node, Var_table *function_vars)
{
    assert (node);
    fprintf (assemble_file, "\nfunc_%s:\n", node->left->value.id);
    Tree_node *params = node->left->left;

    while (params)
    {
        int arg_ind = Find_var_index (function_vars, params->value.id);
        fprintf (assemble_file, "POP [%d+ax]\n\n", arg_ind);
        params = params->left;
    }

    if (node->right)
        Translate_statement (assemble_file, node->right, function_vars);
}

static const char *compare_op (Tree_node *node)
{
    if (IS_OP (OP_LESS))
        return "JBE";
    if (IS_OP (OP_MORE))
        return "JAE";
    if (IS_OP (OP_EQUAL))
        return "JNE";   
    if (IS_OP (OP_NEQUAL))
        return "JE";

    return NULL; 
}

static void Translate_comparsion (FILE *assemble_file, Tree_node *node, Var_table *variables, const char *command, int *counter)
{
    if (IS_COMPARE_OPERATORS(node))
    {
        Translate_expression (assemble_file, node->left, variables);
        Translate_expression (assemble_file, node->right, variables);
        fprintf (assemble_file, "%s end_%s%d\n", compare_op (node), command, *counter);
    }
    else
    {
        Translate_expression (assemble_file, node->left, variables);
        fprintf (assemble_file, "PUSH 0 \nJE end_%s%d\n", command, *counter);
    }
    *counter += 1;
}

void Translate_if (FILE *assemble_file, Tree_node *node, Var_table *variables)
{
    int temp_if_number = if_number;
    fprintf (assemble_file, "\n");

    Translate_comparsion (assemble_file, node->left, variables, "if", &if_number);

    if (node->right->value.op== OP_ELSE)
    {
        Translate_statement (assemble_file, node->right->left, variables);
        fprintf (assemble_file, "JMP end_else%d\n\n", temp_if_number);
        fprintf (assemble_file, "end_if%d:\n\n", temp_if_number);
        Translate_statement (assemble_file, node->right->right, variables);
        fprintf (assemble_file, "end_else%d:\n\n", temp_if_number);
    }
    else
    {
        Translate_statement (assemble_file, node->right, variables);
        fprintf (assemble_file, "end_if%d:\n\n", temp_if_number);
    }

}

void Translate_expression (FILE *assemble_file, Tree_node *node, Var_table *variables)
{
    if (!node)
        return;

    if (IS_OP (OP_SIN))
    {
        Translate_expression (assemble_file, node->left, variables);
        fprintf (assemble_file, "SIN\n");
    }
    else if (IS_OP (OP_SQRT))
    {
        Translate_expression (assemble_file, node->left, variables);
        fprintf (assemble_file, "SQRT\n");
    }
        
    else if (IS_OPERATORS(node))
    {
        Translate_expression (assemble_file, node->right, variables);
        Translate_expression (assemble_file, node->left, variables);
        fprintf (assemble_file, "%s\n", Convert_to_assemble_op (node->value.op));
    }

    else if (node->type == IDENTIFIER)
    {
        int var_index = Find_var_index (variables, node->value.id);
        fprintf (assemble_file, "PUSH [%d+ax]\n", var_index);
    }
    else if (node->type == NUMBER)
    {
        fprintf (assemble_file, "PUSH %lg\n", node->value.num);
    }
    else if (IS_OP (OP_CALL))
    {
        Translate_call (assemble_file, node, variables);
    }
}

void Translate_loop (FILE *assemble_file, Tree_node *node, Var_table *variables)
{
    int temp_loop_number = loop_number;
    fprintf (assemble_file, "start_while%d:\n", loop_number);
    Translate_comparsion (assemble_file, node->left, variables, "while", &loop_number);
    Translate_statement (assemble_file, node->right, variables);
    fprintf (assemble_file, "JMP start_while%d\n", temp_loop_number);
    fprintf (assemble_file, "end_while%d:\n", temp_loop_number);

}


void Translate_assigment (FILE *assemble_file, Tree_node *node, Var_table *variables)
{
    size_t left_var_index = Find_var_index (variables, node->left->value.id);    
    
    if (node->right->type == NUMBER)
        fprintf (assemble_file, "PUSH %lg\n", node->right->value.num);
    else if ((IS_OPERATORS (node->right)) or node->right->type == IDENTIFIER)
    {
        Translate_expression (assemble_file, node->right, variables);
    }
    else if (node->right->type == OPERATOR and node->right->value.op == OP_CALL)
        Translate_call (assemble_file, node->right, variables);
    else 
        fprintf (assemble_file, "IN\n");
    if (IS_OP (OP_ADD_ASSIGN))
    {
        fprintf (assemble_file, "PUSH [%lld+ax]\n", left_var_index);
        fprintf (assemble_file, "ADD\n");
    }
    if (IS_OP (OP_SUB_ASSIGN))
    {
        fprintf (assemble_file, "PUSH [%lld+ax]\n", left_var_index);
        fprintf (assemble_file, "SUB\n");
    }

    fprintf (assemble_file, "POP [%lld+ax]\n",left_var_index);
}
