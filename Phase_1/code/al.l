/**
 * HY-340 Project Phase 1 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define YY_DECL int alpha_yylex(void* yylval)

#define token_string_compare(val1, val2) strcmp((val1), (val2))

void print_unclosed_comment_error();
void push_string(int line);
int pop_string();
int isStringStackEmpty();
void print_unclosed_string_error();

typedef struct StackNode {
    int line;
    struct StackNode* next;
} StackNode;

typedef struct Stack {
    StackNode* top;
} Stack;

Stack commentStack;
Stack stringStack;

/* Token Structure */
typedef struct alpha_token_t {
    unsigned int line_number;
    unsigned int token_number;
    char *content;
    char *token_type;
    char *data_type;
    struct alpha_token_t *next_t;
} alpha_token_t;

alpha_token_t* list_head = NULL;
int counter_tk_number = 0;
int comment_line_start = 0;
int comment_line_end = 0;
int comment_nested = 0;
int string_line_start = 0;

/* Function to insert tokens into the list */
void insert_token(int line, const char* text, const char* type, const char* type_of_data) {
    alpha_token_t* new_token = (alpha_token_t*)malloc(sizeof(alpha_token_t));
    new_token->line_number = line;
    new_token->token_number = ++counter_tk_number;
    new_token->content = strdup(text);
    new_token->token_type = strdup(type);
    new_token->data_type = strdup(type_of_data);
    new_token->next_t = NULL;

    if (!list_head) {
        list_head = new_token;
    } else {
        alpha_token_t* temp = list_head;
        while (temp->next_t)
            temp = temp->next_t;
        temp->next_t = new_token;
    }
}

void initStack(Stack* stack) {
    stack->top = NULL;
}

void push(Stack* stack, int line) {
    StackNode* newNode = (StackNode*)malloc(sizeof(StackNode));
    if (!newNode) {
        fprintf(stderr, "Memory allocation error in push()\n");
        exit(EXIT_FAILURE);
    }
    newNode->line = line;
    newNode->next = stack->top;
    stack->top = newNode;
}

int pop(Stack* stack) {
    if (!stack->top) return -1;  // Stack empty
    StackNode* temp = stack->top;
    int line = temp->line;
    stack->top = stack->top->next;
    free(temp);
    return line;
}

int isStackEmpty(Stack* stack) {
    return stack->top == NULL;
}

void freeStack(Stack* stack) {
    while (!isStackEmpty(stack)) {
        pop(stack);
    }
}

/* Free Token List */
void free_token_list() {
    alpha_token_t* temp;
    while (list_head) {
        temp = list_head;
        list_head = list_head->next_t;
        free(temp->content);
        free(temp->token_type);
        free(temp->data_type);
        free(temp);
    }
}

%}

%option yylineno
%option noyywrap

/* for block comments, before %% */
%x COMMENT

/* for strings, before %% */
%x STRING_MODE

/* Regular Expressions */
id          [a-zA-Z][a-zA-Z_0-9]*
intconst    (0[xX][0-9A-Fa-f]+)|([0-9]+)
realconst   [0-9]+\.[0-9]+(e[+-]?[0-9]+)?
string      \"([^\\n"\\]|\\t|\\n|\\\"|\\\\|\\.)*\"
whitespace  [ \t\r\n]+
comment1    \/\/.*
operator    (\+|\-|\*|\/|=|%|\+\+|\-\-|==|!=|>|<|>=|<=)
keyword     (function|while|for|if|else|continue|break|return|and|or|not|local|true|false|nil)
punctuation (::|\.\.|:|\.|\{|\}|\[|\]|\(|\)|;|,)

%%
{keyword} {
    if (!token_string_compare(yytext, "if"))            { insert_token(yylineno, yytext, "KEYWORD IF", "enumerated");}
    else if (!token_string_compare(yytext, "else"))     { insert_token(yylineno, yytext, "KEYWORD ELSE", "enumerated"); }
    else if (!token_string_compare(yytext, "while"))    { insert_token(yylineno, yytext, "KEYWORD WHILE", "enumerated"); }
    else if (!token_string_compare(yytext, "for"))      { insert_token(yylineno, yytext, "KEYWORD FOR", "enumerated"); }
    else if (!token_string_compare(yytext, "function")) { insert_token(yylineno, yytext, "KEYWORD FUNCTION", "enumerated"); }
    else if (!token_string_compare(yytext, "return"))   { insert_token(yylineno, yytext, "KEYWORD RETURN", "enumerated"); }
    else if (!token_string_compare(yytext, "break"))    { insert_token(yylineno, yytext, "KEYWORD BREAK", "enumerated"); }
    else if (!token_string_compare(yytext, "continue")) { insert_token(yylineno, yytext, "KEYWORD CONTINUE", "enumerated"); }
    else if (!token_string_compare(yytext, "and"))      { insert_token(yylineno, yytext, "KEYWORD AND", "enumerated"); }
    else if (!token_string_compare(yytext, "not"))      { insert_token(yylineno, yytext, "KEYWORD NOT", "enumerated"); }
    else if (!token_string_compare(yytext, "or"))       { insert_token(yylineno, yytext, "KEYWORD OR", "enumerated"); }
    else if (!token_string_compare(yytext, "local"))    { insert_token(yylineno, yytext, "KEYWORD LOCAL", "enumerated"); }
    else if (!token_string_compare(yytext, "true"))     { insert_token(yylineno, yytext, "KEYWORD TRUE", "enumerated"); }
    else if (!token_string_compare(yytext, "false"))    { insert_token(yylineno, yytext, "KEYWORD FALSE", "enumerated"); }
    else if (!token_string_compare(yytext, "nil"))      { insert_token(yylineno, yytext, "KEYWORD NIL", "enumerated"); }
    else {
        insert_token(yylineno, yytext, "KEYWORD", "enumerated");
    }
    return 1;
}

{operator} {
    if (!token_string_compare(yytext, "+"))        { insert_token(yylineno, yytext, "OPERATOR PLUS", "enumerated"); }
    else if (!token_string_compare(yytext, "-"))   { insert_token(yylineno, yytext, "OPERATOR MINUS", "enumerated"); }
    else if (!token_string_compare(yytext, "*"))   { insert_token(yylineno, yytext, "OPERATOR MULT", "enumerated"); }
    else if (!token_string_compare(yytext, "/"))   { insert_token(yylineno, yytext, "OPERATOR DIVIDE", "enumerated"); }
    else if (!token_string_compare(yytext, "%"))   { insert_token(yylineno, yytext, "OPERATOR MODULO", "enumerated"); }
    else if (!token_string_compare(yytext, "++"))  { insert_token(yylineno, yytext, "OPERATOR PLUS_PLUS", "enumerated"); }
    else if (!token_string_compare(yytext, "--"))  { insert_token(yylineno, yytext, "OPERATOR MINUS_MINUS", "enumerated"); }
    else if (!token_string_compare(yytext, "="))   { insert_token(yylineno, yytext, "OPERATOR ASSIGNMENT", "enumerated"); }
    else if (!token_string_compare(yytext, "=="))  { insert_token(yylineno, yytext, "OPERATOR EQUAL_EQUAL", "enumerated"); }
    else if (!token_string_compare(yytext, "!="))  { insert_token(yylineno, yytext, "OPERATOR NOT_EQUAL", "enumerated"); }
    else if (!token_string_compare(yytext, ">"))   { insert_token(yylineno, yytext, "OPERATOR GREATER", "enumerated"); }
    else if (!token_string_compare(yytext, "<"))   { insert_token(yylineno, yytext, "OPERATOR LESS", "enumerated"); }
    else if (!token_string_compare(yytext, ">="))  { insert_token(yylineno, yytext, "OPERATOR GREATER_EQUAL", "enumerated"); }
    else if (!token_string_compare(yytext, "<="))  { insert_token(yylineno, yytext, "OPERATOR LESS_EQUAL", "enumerated"); }
    else {
        /* If none of the above operators doesnt match then just store it as a general OPERATOR. */
        insert_token(yylineno, yytext, "OPERATOR", "enumerated");
    }
    return 1;
}

{punctuation} {
    if (!token_string_compare(yytext, "("))       { insert_token(yylineno, yytext, "PUNCTUATION LEFT_PARENTHESIS", "enumerated"); }
    else if (!token_string_compare(yytext, ")"))  { insert_token(yylineno, yytext, "PUNCTUATION RIGHT_PARENTHESIS", "enumerated"); }
    else if (!token_string_compare(yytext, "["))  { insert_token(yylineno, yytext, "PUNCTUATION LEFT_BRACKET", "enumerated"); }
    else if (!token_string_compare(yytext, "]"))  { insert_token(yylineno, yytext, "PUNCTUATION RIGHT_BRACKET", "enumerated"); }
    else if (!token_string_compare(yytext, "{"))  { insert_token(yylineno, yytext, "PUNCTUATION LEFT_BRACE", "enumerated"); }
    else if (!token_string_compare(yytext, "}"))  { insert_token(yylineno, yytext, "PUNCTUATION RIGHT_BRACE", "enumerated"); }
    else if (!token_string_compare(yytext, ";"))  { insert_token(yylineno, yytext, "PUNCTUATION SEMICOLON", "enumerated"); }
    else if (!token_string_compare(yytext, "."))  { insert_token(yylineno, yytext, "PUNCTUATION DOT", "enumerated"); }
    else if (!token_string_compare(yytext, "..")) { insert_token(yylineno, yytext, "PUNCTUATION DOT_DOT", "enumerated"); }
    else if (!token_string_compare(yytext, "::")) { insert_token(yylineno, yytext, "PUNCTUATION COLON_COLON", "enumerated"); }
    else if (!token_string_compare(yytext, ":"))  { insert_token(yylineno, yytext, "PUNCTUATION COLON", "enumerated"); }
    else if (!token_string_compare(yytext, ","))  { insert_token(yylineno, yytext, "PUNCTUATION COMMA", "enumerated"); }
    else {
        insert_token(yylineno, yytext, "PUNCTUATION", "enumerated");
    }
    return 1;
}

{id}          { insert_token(yylineno, yytext, "IDENTIFIER", "char*"); return 1; }
{intconst}    { insert_token(yylineno, yytext, "INTCONST", "integer"); return 1; }
{realconst}   { insert_token(yylineno, yytext, "REALCONST", "float"); return 1; }
{string} {
    char *processed_str = strdup(yytext);
    
    // Remove outer quotes
    processed_str[strlen(processed_str) - 1] = '\0';
    memmove(processed_str, processed_str + 1, strlen(processed_str));

    // Replace escape sequences
    for (int i = 0; processed_str[i]; i++) {
        if (processed_str[i] == '\\' && processed_str[i + 1]) {
            char replacement_regex;
            if (processed_str[i + 1] == 't') {
                replacement_regex = '\t';
            } else if (processed_str[i + 1] == 'n') {
                replacement_regex = '\n';
            } else if (processed_str[i + 1] == '"') {
                replacement_regex = '"';
            } else if (processed_str[i + 1] == '\\') {
                replacement_regex = '\\';
            } else {
                // If the escape sequence is unrecognized, leave it as is.
                continue;
            }
            processed_str[i] = replacement_regex;
            memmove(&processed_str[i + 1], &processed_str[i + 2],
                    strlen(&processed_str[i + 2]) + 1);
        }
    }

    insert_token(yylineno, processed_str, "STRING", "char*");
    free(processed_str);
    return 1;
}

{comment1}    {insert_token(yylineno, yytext, "COMMENT LINE_COMMENT", "enumerated"); }

"/*" {	
    push(&commentStack, yylineno);
    comment_nested++;
    BEGIN(COMMENT);
}

<COMMENT>"/*" {
    push(&commentStack, yylineno);
    comment_nested++;
}

<COMMENT>. {
}

<COMMENT>"*/" {
    if (comment_nested > 0) {
        int startLine = pop(&commentStack);
        comment_nested--;
        if (isStackEmpty(&commentStack)) {
            comment_line_end = yylineno;
            int msg_size = snprintf(NULL, 0, "Nested Comment %d - %d", startLine, comment_line_end) + 1;
            char *comment_msg = (char *)malloc(msg_size);
            if (comment_msg) {
                snprintf(comment_msg, msg_size, "Nested Comment %d - %d", startLine, comment_line_end);
                insert_token(startLine, comment_msg, "COMMENT BLOCK_COMMENT", "enumerated");
                free(comment_msg);
            }
            BEGIN(INITIAL);
        }
    }
}           

<COMMENT>\n { 
    comment_line_end = yylineno; 
}

<COMMENT><<EOF>> {
    print_unclosed_comment_error();
    yyterminate();
}

\" {
    push_string(yylineno);
    BEGIN(STRING_MODE);
}

<STRING_MODE>[^"\\\n]+ { yymore(); }
<STRING_MODE>\\.       { yymore(); }

<STRING_MODE>\" {
    int startLine = pop_string();
    if (startLine != -1) {
        insert_token(startLine, yytext, "STRING", "char*");
    }
    BEGIN(INITIAL);
}

<STRING_MODE>\n {
    int startLine = pop_string();
    if (startLine != -1) {
        int msg_size = snprintf(NULL, 0, "ERROR UNCLOSED STRING AT LINE %d", startLine) + 1;
        char *error_msg = (char *)malloc(msg_size);
        if (error_msg) {
            snprintf(error_msg, msg_size, "ERROR UNCLOSED STRING AT LINE %d", startLine);
            insert_token(startLine, error_msg, "ERROR", "enumerated");
            free(error_msg);
        }
    }
    BEGIN(INITIAL);

    while (input() != '\n' && input() != EOF) {}
}

<STRING_MODE><<EOF>> {
    while (!isStringStackEmpty()) {
        int startLine = pop_string();
        int msg_size = snprintf(NULL, 0, "ERROR UNCLOSED STRING AT LINE %d", startLine) + 1;
        char *error_msg = (char *)malloc(msg_size);
        if (error_msg) {
            snprintf(error_msg, msg_size, "ERROR UNCLOSED STRING AT LINE %d", startLine);
            insert_token(startLine, error_msg, "ERROR", "enumerated");
            free(error_msg);
        }
    }
    BEGIN(INITIAL);
}

{whitespace}  {  }

.   { 
    int msg_size = snprintf(NULL, 0, "ERROR UNDEFINED CHARACTER AT LINE %d", yylineno) + 1;
    char *error_msg = (char *)malloc(msg_size);
    if (error_msg) {
        snprintf(error_msg, msg_size, "ERROR UNDEFINED CHARACTER AT LINE %d", yylineno);
        insert_token(yylineno, yytext, error_msg, "enumerated");
        free(error_msg);
    }
}

%%

/* Print error for unclosed nested comment */
void print_unclosed_comment_error() {
    while (!isStackEmpty(&commentStack)) {
        int startLine = pop(&commentStack);
        int msg_size = snprintf(NULL, 0, "ERROR UNCLOSED COMMENT STARTED AT %d", startLine) + 1;
        char *error_msg = (char *)malloc(msg_size);
        if (error_msg) {
            snprintf(error_msg, msg_size, "ERROR UNCLOSED COMMENT STARTED AT %d", startLine);
            insert_token(startLine, error_msg, "ERROR", "enumerated");
            free(error_msg);
        }
    }
}

void push_string(int line) {
    push(&stringStack, line);
}

int pop_string() {
    return pop(&stringStack);
}

int isStringStackEmpty() {
    return isStackEmpty(&stringStack);
}

/* Print error for unclosed string */
void print_unclosed_string_error() {
    while (!isStackEmpty(&stringStack)) {
        int startLine = pop(&stringStack);
        int msg_size = snprintf(NULL, 0, "ERROR UNCLOSED STRING AT LINE %d", startLine) + 1;
        char *error_msg = (char *)malloc(msg_size);
        if (error_msg) {
            snprintf(error_msg, msg_size, "ERROR UNCLOSED STRING AT LINE %d", startLine);
            insert_token(startLine, error_msg, "ERROR", "enumerated");
            free(error_msg);
        }
    }
}

/* Function to print tokens */
void print_output() {
    alpha_token_t* temp = list_head;
    printf("---------------- Lexical Analysis ----------------\n");
    
    while (temp) {
        int extra_size = 0;
        
        if (!token_string_compare(temp->token_type, "INTCONST") || !token_string_compare(temp->token_type, "REALCONST")) {
            extra_size = snprintf(NULL, 0, " %s", temp->content) + 1;
        } else if (!token_string_compare(temp->token_type, "IDENTIFIER") || !token_string_compare(temp->token_type, "STRING")) {
            extra_size = snprintf(NULL, 0, " \"%s\"", temp->content) + 1;
        }

        char *extra_var = (char *)malloc(extra_size);
        if (extra_var) {
            if (extra_size > 1) {
                if (!token_string_compare(temp->token_type, "INTCONST") || 
                    !token_string_compare(temp->token_type, "REALCONST")) {
                    snprintf(extra_var, extra_size, " %s", temp->content);
                } else if (!token_string_compare(temp->token_type, "IDENTIFIER") || 
                           !token_string_compare(temp->token_type, "STRING")) {
                    snprintf(extra_var, extra_size, " \"%s\"", temp->content);
                }
            } else {
                extra_var[0] = '\0';
            }
        } else {
            extra_var = "";
        }
        
        printf("%d: #%d \"%s\" %s%s <-%s\n", 
               temp->line_number, 
               temp->token_number, 
               temp->content, 
               temp->token_type,
               extra_var,
               temp->data_type);

        if (extra_size > 1) {
            free(extra_var);
        }

        temp = temp->next_t;
    }
}

int main(int argc, char *argv[]) {
    initStack(&commentStack);
    initStack(&stringStack);

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <input file> [output file]\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *input_file = fopen(argv[1], "r");
    if (!input_file) {
        perror("Error opening input file");
        return EXIT_FAILURE;
    }
    yyin = input_file;

    while (alpha_yylex(NULL));
    fclose(input_file);

    if (argc == 3) {
        FILE *output_file = fopen(argv[2], "w");
        if (!output_file) {
            perror("Error opening output file");
            return EXIT_FAILURE;
        }
        freopen(argv[2], "w", stdout);
    }

    print_output();

    free_token_list();
    freeStack(&commentStack);
    freeStack(&stringStack);

    return EXIT_SUCCESS;
}
