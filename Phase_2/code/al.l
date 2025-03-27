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
#include "parser.tab.h"
extern int yylineno;

#define YY_DECL int alpha_yylex(void* yylval)

#define token_string_compare(val1, val2) strcmp((val1), (val2))

void print_unclosed_comment_error();
void push_string(int line);
int pop_string();
int isStringStackEmpty();
void print_unclosed_string_error();

typedef struct StackNode {
    int line;
    int has_nested;
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
    newNode->has_nested = 0;	// in the begining default is that these is no nested comment 
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
    

{keyword} {
    if (!strcmp(yytext, "if"))            { return IF; }
    else if (!strcmp(yytext, "else"))     { return ELSE; }
    else if (!strcmp(yytext, "while"))    { return WHILE; }
    else if (!strcmp(yytext, "for"))      { return FOR; }
    else if (!strcmp(yytext, "function")) { return FUNCTION; }
    else if (!strcmp(yytext, "return"))   { return RETURN; }
    else if (!strcmp(yytext, "break"))    { return BREAK; }
    else if (!strcmp(yytext, "continue")) { return CONTINUE; }
    else if (!strcmp(yytext, "and"))      { return AND; }
    else if (!strcmp(yytext, "not"))      { return NOT; }
    else if (!strcmp(yytext, "or"))       { return OR; }
    else if (!strcmp(yytext, "local"))    { return LOCAL; }
    else if (!strcmp(yytext, "true"))     { return TRUE; }
    else if (!strcmp(yytext, "false"))    { return FALSE; }
    else if (!strcmp(yytext, "nil"))      { return NIL; }
    else {
        return IDENTIFIER;
    }
}

{operator} {
    if (!token_string_compare(yytext, "+"))        { return PLUS; }
    else if (!token_string_compare(yytext, "-"))   { return MINUS; }
    else if (!token_string_compare(yytext, "*"))   { return MULTIPLY; }
    else if (!token_string_compare(yytext, "/"))   { return DIVIDE; }
    else if (!token_string_compare(yytext, "%"))   { return MODULO; }
    else if (!token_string_compare(yytext, "++"))  { return PLUS_PLUS; }
    else if (!token_string_compare(yytext, "--"))  { return MINUS_MINUS; }
    else if (!token_string_compare(yytext, "="))   { return ASSIGNMENT; }
    else if (!token_string_compare(yytext, "=="))  { return EQUAL_EQUAL; }
    else if (!token_string_compare(yytext, "!="))  { return NOT_EQUAL; }
    else if (!token_string_compare(yytext, ">"))   { return GREATER; }
    else if (!token_string_compare(yytext, "<"))   { return LESS; }
    else if (!token_string_compare(yytext, ">="))  { return GREATER_EQUAL; }
    else if (!token_string_compare(yytext, "<="))  { return LESS_EQUAL; }
    else {
        /* Return a generic operator token if no specific match is found */
        return OPERATOR;
    }
}

{punctuation} {
    if (!token_string_compare(yytext, "("))       { return LEFT_PARENTHESIS; }
    else if (!token_string_compare(yytext, ")"))  { return RIGHT_PARENTHESIS; }
    else if (!token_string_compare(yytext, "["))  { return LEFT_BRACKET; }
    else if (!token_string_compare(yytext, "]"))  { return RIGHT_BRACKET; }
    else if (!token_string_compare(yytext, "{"))  { return LEFT_BRACE; }
    else if (!token_string_compare(yytext, "}"))  { return RIGHT_BRACE; }
    else if (!token_string_compare(yytext, ";"))  { return SEMICOLON; }
    else if (!token_string_compare(yytext, "."))  { return DOT; }
    else if (!token_string_compare(yytext, "..")) { return DOT_DOT; }
    else if (!token_string_compare(yytext, "::")) { return COLON_COLON; }
    else if (!token_string_compare(yytext, ":"))  { return COLON; }
    else if (!token_string_compare(yytext, ","))  { return COMMA; }
    else {
        /* Return a generic punctuation token if no specific match is found */
        return PUNCTUATION;
    }
}

{id}          { yylval.stringValue = strdup(yytext); return IDENTIFIER; }
{intconst}    { yylval.intValue = atoi(yytext); return INTCONST; }
{realconst}   { yylval.realValue = atof(yytext); return REALCONST; }

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
    if (commentStack.top != NULL) {
        commentStack.top->has_nested = 1;
    }
    push(&commentStack, yylineno);
    comment_nested++;
}

<COMMENT>. {
}

<COMMENT>"*/" {
    if (comment_nested > 0) {
        int startLine = commentStack.top->line;
        int was_nested = commentStack.top->has_nested;
        pop(&commentStack);
        comment_nested--;

        comment_line_end = yylineno;
        const char* label = (commentStack.top != NULL) ? "Nested Comment" : "Comment";
        int msg_size = snprintf(NULL, 0, "%s %d - %d", label, startLine, comment_line_end) + 1;
        char *comment_msg = (char *)malloc(msg_size);
        if (comment_msg) {
            snprintf(comment_msg, msg_size, "%s %d - %d", label, startLine, comment_line_end);
            insert_token(startLine, comment_msg, "COMMENT BLOCK_COMMENT", "enumerated");
            free(comment_msg);
        }
        if (isStackEmpty(&commentStack)){
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
        char *processed_str = strdup(yytext);
        int len = strlen(processed_str);

        if (len > 0 && processed_str[len - 1] == '"') {
            processed_str[len - 1] = '\0';
        }

        if (processed_str[0] == '"') {
            memmove(processed_str, processed_str + 1, strlen(processed_str));
        }

        for (int i = 0; processed_str[i]; i++) {
            if (processed_str[i] == '\\' && processed_str[i + 1]) {
                char replacement_char;
                if (processed_str[i + 1] == 't') {
                    replacement_char = '\t';
                } else if (processed_str[i + 1] == 'n') {
                    replacement_char = '\n';
                } else if (processed_str[i + 1] == '"') {
                    replacement_char = '"';
                } else if (processed_str[i + 1] == '\\') {
                    replacement_char = '\\';
                } else {
                    continue;
                }
                processed_str[i] = replacement_char;
                memmove(&processed_str[i + 1],
                        &processed_str[i + 2],
                        strlen(&processed_str[i + 2]) + 1);
            }
        }

        insert_token(startLine, processed_str, "STRING", "char*");
        free(processed_str);
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
    int c;
    while ((c = input()) != '\n' && c != EOF) {}
    BEGIN(INITIAL);
}

<STRING_MODE><<EOF>> {
    while (!isStringStackEmpty()) {
        int startLine = pop_string();
        if (yyleng > 0 && yytext[yyleng - 1] == '\\') {
            int msg_size = snprintf(NULL, 0, "ERROR UNCLOSED STRING AT LINE %d (trailing backslash)", startLine) + 1;
            char *error_msg = (char *)malloc(msg_size);
            if (error_msg) {
                snprintf(error_msg, msg_size, "ERROR UNCLOSED STRING AT LINE %d (trailing backslash)", startLine);
                insert_token(startLine, error_msg, "ERROR", "enumerated");
                free(error_msg);
            }
        } else {
            int msg_size = snprintf(NULL, 0, "ERROR UNCLOSED STRING AT LINE %d", startLine) + 1;
            char *error_msg = (char *)malloc(msg_size);
            if (error_msg) {
                snprintf(error_msg, msg_size, "ERROR UNCLOSED STRING AT LINE %d", startLine);
                insert_token(startLine, error_msg, "ERROR", "enumerated");
                free(error_msg);
            }
        }
    }
    BEGIN(INITIAL);
    yyterminate();
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
