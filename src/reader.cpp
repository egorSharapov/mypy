#include <malloc.h>
#include <assert.h>
#include <math.h>
#include <windows.h>
#include "..\include\reader.hpp"
#include "..\include\utilities.hpp"
#include "..\include\file_analize.hpp"


static Tree_tokens *Tokens_ctor (size_t size)
{
    Tree_tokens *tokens = (Tree_tokens *) Safety_calloc (sizeof (Tree_tokens));
    tokens->capacity = 0;
    tokens->size = 0;
    tokens->array = (Tree_node **) Safety_calloc (sizeof (Tree_node) * size); //fix it, invalid size

    return tokens;
}


static void Skip_spaces (const char **str)
{
    while (**str == ' ')
        *str += 1;
}

static int Add_delim (const char **code_string, int *str_len)
{
    int space_number = 4;
    int deep = 0;
    while (strncmp (*code_string, "    ", space_number) == 0)
    {
        *code_string += space_number;
        *str_len     -= space_number;
        deep++;
    }

    return deep;
}


static int Check_deep (Tree_tokens *tokens, int old_deep, int deep)
{
    if (old_deep > deep)
        tokens->array[tokens->capacity++] = New_operator (OP_ENDB, NULL, NULL);
    else if (old_deep < deep)
        tokens->array[tokens->capacity++] = New_operator (OP_BEGINB, NULL, NULL);
    return deep;
}

Tree_tokens *Tokenizer (Text *code)
{

    int old_deep = 0;
    Tree_tokens *tokens = Tokens_ctor (code->number_of_symbols);
    Tree_node   *node   = NULL;

    for (size_t index = 0; index < code->number_of_strings; index++)
    {
        const char *str_ptr = code->string[index].point;
        const char *str_tmp = str_ptr;
        int         str_len = code->string[index].len;

        int deep = Add_delim (&str_ptr, &str_len);
        old_deep = Check_deep (tokens, old_deep, deep);

        while (*str_ptr != '\0' and *str_ptr != '#')
        {
            Skip_spaces (&str_ptr);
 
            node = Is_num (&str_ptr, index + 1, str_ptr - str_tmp);

            if (node)
                tokens->array[tokens->capacity++] = node;
            else if (*str_ptr != '\0' and *str_ptr != ' ' and *str_ptr != '#')
            {
                printf ("lex error\n");
                break;
            }

        }
        if (str_len > 0 and *str_ptr != '#')
            tokens->array[tokens->capacity++] = New_operator (OP_EOS, NULL, NULL);
    }
    tokens->array[tokens->capacity++] = New_operator (OP_EOF, NULL, NULL);
    
    return tokens;
}






Tree_node *Is_num (const char **str, size_t str_num, size_t str_pos)
{
    double number = 0;
    const char *s_old = *str;

    number = strtod (*str, (char **) str);

    if (s_old != *str)
        return New_num (number, str_pos, str_num);
    
    return Is_operator (str, str_num, str_pos);
}

Tree_node *Is_variable (const char **str, size_t str_num, size_t str_pos)
{
    char *var_value = (char *) Safety_calloc (strlen (*str));

    if (sscanf (*str, "%[_A-Za-z0-9]", var_value) == 1)
    {
        *str += strlen (var_value);
        return New_var (var_value, str_pos, str_num);
    }

    free (var_value);
    return NULL;
}

#define DEF_OP(op_name, op_number, op_str)         \
    size = strlen (op_str);                        \
    if (strncmp (*str, op_str, size) == 0)         \
    {                                              \
        *str += size;                              \
        return New_operator (op_name, NULL, NULL); \
    }

Tree_node *Is_operator (const char **str, size_t str_num, size_t str_pos)
{
    size_t size = 0;
    char op_value[MAX_OP_SIZE] = {0};
    sscanf (*str, "%[a-z*+-/=\"]", op_value);
    // printf ("operator: %s\n", op_value);
    #include "..\include\operators_dsl.hpp"

    return Is_variable (str, str_num, str_pos);
}

#undef DEF_OP

/*
grammar -> Instruction EOF
Instruction -> fundeclaration | vardeclaration | assigment | choice_instruction | loop_instruction ?Instruction
assigment -> variable '=' expression | funcall
choice_instruction -> if expression ':' body_instruction
vardeclaration -> 'var' variable = expression
fundeclaration -> 'fun' variable '(' ')' ':' body_instruction 
funcall        -> variable '(' ')' 
jumpinstruction -> 'break' 'continue' 'return'
*/
//таблица имен таблица джампов

Tree_node* Get_grammar (Tree_tokens *tokens, Translator *translator)
{
    assert (tokens);

    Tree_node *left_node = Get_instruction (tokens, translator);

    if (IS_TOKEN_OP (OP_EOF))
        printf ("succes syntax analize\n");

    return left_node;
}

Tree_node *Get_instruction (Tree_tokens *tokens, Translator *translator)
{
    Tree_node* right_node = NULL;
    Tree_node* left_node = NULL;

    if ((IS_TYPE (IDENTIFIER) or IS_TYPE (OPERATOR)) and !(IS_TOKEN_OP (OP_EOF) or IS_TOKEN_OP (OP_ENDB)))
    {
        if (IS_TYPE (IDENTIFIER))
            left_node = Get_assigment (tokens, translator);
        else if (IS_TOKEN_OP (OP_VAR))
            left_node = Get_var_declaration (tokens, translator);
        else if (IS_TOKEN_OP (OP_IF))
            left_node = Get_choice_instruction (tokens, translator);
        else if (IS_TOKEN_OP (OP_WHILE))
            left_node = Get_loop_instruction (tokens, translator);
        else if (IS_TOKEN_OP (OP_FUNC))
            left_node = Get_fun_declaration (tokens, translator);
        else if (IS_TOKEN_OP (OP_PRINT))
            left_node = Get_fun_call (tokens, translator);
        else if (IS_TOKEN_OP (OP_RETURN))
            left_node = Get_jump_instruction (tokens, translator);
        else
            return left_node;

        if ((right_node = Get_instruction (tokens, translator)))
            left_node = New_operator (OP_ST, left_node, right_node);
 
    }

    return left_node;
}

Tree_node *Get_jump_instruction (Tree_tokens *tokens, Translator *translator)
{
    if (IS_TOKEN_OP (OP_RETURN))
    {
        Tree_node *node_left = TOKEN;
        tokens->size += 1;
        node_left->left = Get_expression (tokens, translator);
        CHECK_OP (OP_EOS)
        return node_left;
    }
    return NULL;
}

Tree_node *Get_fun_call (Tree_tokens *tokens, Translator *translator)
{
    if (IS_TOKEN_OP (OP_INPUT))
    {
        Tree_node *fun_call = tokens->array[tokens->size++];
        CHECK_OP (OP_LBRC)
        CHECK_OP (OP_RBRC)

        return fun_call;
    }
    if (IS_TOKEN_OP (OP_PRINT))
    {
        Tree_node *fun_call = tokens->array[tokens->size++];
        CHECK_OP (OP_LBRC)
        if (IS_TOKEN_OP (OP_QUOTE))
        {
            tokens->size += 1;
            fun_call->right = TOKEN;
            tokens->size += 1;
            CHECK_OP (OP_QUOTE)
            CHECK_OP (OP_COMMA)
        }
        fun_call->left = Get_expression (tokens, translator);
        CHECK_OP (OP_RBRC)
        CHECK_OP (OP_EOS)
        return fun_call;
    }
    if (IS_TYPE (IDENTIFIER))
    {
        Tree_node *fun_call = New_operator (OP_CALL, NULL, NULL);

        fun_call->left = tokens->array[tokens->size++];
        if (!Is_in_fun_table (translator->functions, fun_call->left->value.id))
            printf ("no declaration function: %s\n", fun_call->left->value.id);

        CHECK_OP (OP_LBRC)

        Tree_node *params = Get_expression (tokens, translator);

        while (IS_TOKEN_OP (OP_COMMA))
        {
            tokens->size += 1;
            params = New_operator (OP_ST, params, NULL);
            params->right = Get_expression (tokens, translator);
        }
        if (params)
            fun_call->left->left = params;


        CHECK_OP (OP_RBRC)

        return fun_call;
    }
    return NULL;
}

static Tree_node *Get_instruction_body(Tree_tokens *tokens, Translator *translator)
{
        CHECK_OP (OP_COLON)
        CHECK_OP (OP_EOS)
        CHECK_OP (OP_BEGINB)

        Tree_node *body = Get_instruction (tokens, translator);

        CHECK_OP (OP_ENDB) 

        return body;
}


Tree_node *Get_fun_declaration (Tree_tokens *tokens, Translator *translator)
{
    if (translator->local_level != GLOBAL_LEVEL)
    {
        printf ("declaration function in function\n");
        abort ();
    }

    if (IS_TOKEN_OP (OP_FUNC))
    {
        //создание таблички перееменных функции
        Var_table *variables = Var_table_ctor ();

        Tree_node *func = tokens->array[tokens->size];

        tokens->size += 1;
        Fun_table_resize (translator->functions);

        func->left = tokens->array[tokens->size++];

        function.fun_pointer = func;
        function.name = func->left->value.id;
        function.variables = variables;

        CHECK_OP(OP_LBRC);

        Tree_node *params = func->left->left;

        if (IS_TOKEN_OP (OP_VAR))
        {
            tokens->size += 1;
            params = tokens->array[tokens->size++];
            Var_table_push (variables, params->value.id);
            func->left->left = params;
            while (IS_TOKEN_OP (OP_COMMA))
            {
                tokens->size += 1;
                CHECK_OP (OP_VAR);
                params->left = tokens->array[tokens->size++];
                Var_table_push (variables, params->left->value.id);
                params = params->left;
            }
        }
        CHECK_OP (OP_RBRC);
        
        translator->local_level = FUNC_LEVEL;
        func->right = Get_instruction_body (tokens, translator);
        translator->local_level = GLOBAL_LEVEL;

        translator->functions->size += 1;

        return func;
    }
    return NULL;
}


Tree_node *Get_var_declaration (Tree_tokens *tokens, Translator *translator)
{

    if (IS_TYPE (OPERATOR) and IS_TOKEN_OP (OP_VAR))
    {
        Var_table *vars = NULL;

        tokens->size += 1;

        const char *temp_name = tokens->array[tokens->size]->value.id;

        if (translator->local_level == GLOBAL_LEVEL)
            vars = translator->variables;
        if (translator->local_level == FUNC_LEVEL)
            vars = translator->functions->table[translator->functions->size].variables;

        if (Is_in_var_table (vars, temp_name))
            printf ("warning: redeclaration var %s\n", temp_name);
    
        Var_table_push (vars, temp_name);
        Tree_node *left_node = Get_assigment (tokens, translator);

        return left_node;

    }
    return NULL;
}


static Tree_node *Get_choice_comparsion (Tree_tokens *tokens, Translator *translator)
{
    Tree_node *compound_op = Get_expression (tokens, translator);
    
    if (IS_TOKEN_OP (OP_LESS)   or IS_TOKEN_OP (OP_MORE) or
        IS_TOKEN_OP (OP_NEQUAL) or IS_TOKEN_OP (OP_EQUAL))
    {
        Tree_node *compare_op = TOKEN;
        tokens->size += 1; 
        compare_op->left = compound_op;
        compare_op->right = Get_expression (tokens, translator);
        compound_op = compare_op;
    }
    return compound_op;
}


Tree_node *Get_loop_instruction (Tree_tokens *tokens, Translator *translator)
{
    if (IS_TYPE (OPERATOR) and IS_TOKEN_OP (OP_WHILE))
    {
        Tree_node *parent = tokens->array[tokens->size++];

        parent->left = Get_choice_comparsion (tokens, translator);

        // printf ("operator: %d\n", tokens->array[tokens->size]->op_value);

        CHECK_OP (OP_COLON)
        CHECK_OP (OP_EOS)
        CHECK_OP (OP_BEGINB)

        parent->right = Get_instruction (tokens, translator);

        CHECK_OP (OP_ENDB)

        return parent;

    }
    return NULL;

}


Tree_node *Get_choice_instruction (Tree_tokens *tokens, Translator *translator)
{
    if (IS_TYPE (OPERATOR) and IS_TOKEN_OP (OP_IF))
    {
        Tree_node *choice_op = TOKEN;
        Tree_node *tmp_op = choice_op;
        tokens->size += 1;

        choice_op->left  = Get_choice_comparsion (tokens, translator);
        choice_op->right = Get_instruction_body (tokens, translator);

        while (IS_TOKEN_OP (OP_ELIF))
        {
            free(TOKEN);
            tokens->size += 1;
            Tree_node * elif_op = New_operator (OP_ELSE, NULL, NULL);
            elif_op->left  = choice_op->right;
            elif_op->right = New_operator      (OP_IF, NULL, NULL);

            elif_op->right->left  = Get_choice_comparsion (tokens, translator);
            elif_op->right->right = Get_instruction_body  (tokens, translator);

            choice_op->right = elif_op;
            choice_op = elif_op->right;
        }
        if (IS_TOKEN_OP (OP_ELSE))
        {
            Tree_node *else_op = TOKEN;
            tokens->size += 1;
            else_op->right = Get_instruction_body (tokens, translator);
            
            else_op->left    = choice_op->right;
            choice_op->right = else_op;
        }
        return tmp_op;
    }
    return NULL;
}

Tree_node *Get_assigment (Tree_tokens *tokens, Translator *translator)
{
    Tree_node* node_left = Get_variable (tokens, translator);

    if (IS_TYPE (OPERATOR) and IS_ASSIGN_OP())
    {
        Tree_node *assign = tokens->array[tokens->size++];

        assign->right = Get_expression (tokens, translator);
        assign->left = node_left;

        node_left = assign;
    }
    CHECK_OP (OP_EOS)

    return node_left;
}

Tree_node* Get_expression (Tree_tokens *tokens, Translator *translator)
{
    Tree_node* node_left = Get_muldiv_expression (tokens, translator);

    while (IS_TYPE (OPERATOR) and (IS_TOKEN_OP (OP_ADD) or IS_TOKEN_OP (OP_SUB)))
    {
        Tree_node *addsub_node = tokens->array[tokens->size++];

        addsub_node->right = Get_muldiv_expression (tokens, translator);
        addsub_node->left = node_left;

        node_left = addsub_node;
    }
    return node_left;
}


Tree_node* Get_muldiv_expression (Tree_tokens *tokens, Translator *translator)
{
    Tree_node* node_left = Get_power (tokens, translator);

    while (IS_TYPE (OPERATOR) and (IS_TOKEN_OP (OP_MUL) or IS_TOKEN_OP (OP_DIV)))
    {
        Tree_node *muldiv_node  = tokens->array[tokens->size++];
        muldiv_node->right = Get_power (tokens, translator);

        muldiv_node->left = node_left;
        node_left = muldiv_node;
    }
    return node_left;
}


Tree_node* Get_major_expression (Tree_tokens *tokens, Translator *translator)
{
    Tree_node* val = NULL;

    if (IS_TYPE (OPERATOR) and IS_TOKEN_OP (OP_LBRC))
    {
        free (TOKEN);
        tokens->size += 1;
        val = Get_expression (tokens, translator);
        CHECK_OP (OP_RBRC)
    }
    else if (IS_TYPE (NUMBER))
        val = Get_number (tokens);
    else if (IS_TYPE (IDENTIFIER))
    {
        if (IS_NEXT_OP (OP_LBRC))
            val = Get_fun_call (tokens, translator);
        else
            val = Get_variable (tokens, translator);
    }
    else if (IS_TOKEN_OP (OP_INPUT))
        val = Get_fun_call (tokens, translator);
    else
        printf ("get P error\n");

    return val;
}


Tree_node* Get_number (Tree_tokens *tokens)
{
    if (IS_TYPE (NUMBER))
        return tokens->array[tokens->size++];
    else
        printf ("get number error");

    return NULL;
}

Tree_node* Get_variable (Tree_tokens *tokens, Translator *translator)
{
    if (IS_TYPE (IDENTIFIER))
    {
        const char *var_name = tokens->array[tokens->size]->value.id;
        Var_table * vars = NULL;
        if (translator->local_level == GLOBAL_LEVEL)
            vars = translator->variables;

        if (translator->local_level == FUNC_LEVEL)
            vars = translator->functions->table[translator->functions->size].variables;
        
        if (Is_in_var_table (vars, var_name))
            return tokens->array[tokens->size++];
        else
        {
            printf ("no declaration var: %s\n", var_name);
            exit (0);
        }
    }
    else
        printf ("get variable error");
    
    return NULL;
}


Tree_node* Get_power (Tree_tokens *tokens, Translator *translator)
{
    Tree_node* node_left = Get_function_expression (tokens, translator);
    
    if (IS_TYPE (OPERATOR) and IS_TOKEN_OP (OP_PWR))
    {
        Tree_node *pwr = tokens->array[tokens->size++];

        pwr->right = Get_function_expression (tokens, translator);
        pwr->left = node_left;

        return pwr;
    }
    return node_left;
}


Tree_node *Get_function_expression (Tree_tokens *tokens, Translator *translator)
{
    if (IS_TYPE (OPERATOR) and (IS_UNARY()))
    {
        Tree_node  *func_operator = tokens->array[tokens->size++];
        CHECK_OP (OP_LBRC);

        func_operator->left = Get_expression (tokens, translator);
        CHECK_OP (OP_RBRC);
        return func_operator;
    }

    return Get_major_expression (tokens, translator);
}


