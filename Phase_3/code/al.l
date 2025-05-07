/**
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

%{
#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
extern int yylineno;

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
operator    (\+|\-|\*|\/|=|%|\+\+|\-\-|==|!=|>|<|>=|<=|&&|\|\|)
keyword     (function|while|for|if|else|continue|break|return|and|or|not|local|true|false|nil)
punctuation (:|\.|\{|\}|\[|\]|\(|\)|;|,)

%%
{keyword} {
    if (!token_string_compare(yytext, "if"))            { insert_token(yylineno, yytext, "KEYWORD IF", "enumerated"); return IF; }
    else if (!token_string_compare(yytext, "else"))     { insert_token(yylineno, yytext, "KEYWORD ELSE", "enumerated"); return ELSE; }
    else if (!token_string_compare(yytext, "while"))    { insert_token(yylineno, yytext, "KEYWORD WHILE", "enumerated"); return WHILE; }
    else if (!token_string_compare(yytext, "for"))      { insert_token(yylineno, yytext, "KEYWORD FOR", "enumerated"); return FOR; }
    else if (!token_string_compare(yytext, "function")) { insert_token(yylineno, yytext, "KEYWORD FUNCTION", "enumerated"); return FUNCTION; }
    else if (!token_string_compare(yytext, "return"))   { insert_token(yylineno, yytext, "KEYWORD RETURN", "enumerated"); return RETURN; }
    else if (!token_string_compare(yytext, "break"))    { insert_token(yylineno, yytext, "KEYWORD BREAK", "enumerated"); return BREAK; }
    else if (!token_string_compare(yytext, "continue")) { insert_token(yylineno, yytext, "KEYWORD CONTINUE", "enumerated"); return CONTINUE; }
    else if (!token_string_compare(yytext, "and"))      { insert_token(yylineno, yytext, "KEYWORD AND", "enumerated"); return AND; }
    else if (!token_string_compare(yytext, "not"))      { insert_token(yylineno, yytext, "KEYWORD NOT", "enumerated"); return NOT; }
    else if (!token_string_compare(yytext, "or"))       { insert_token(yylineno, yytext, "KEYWORD OR", "enumerated"); return OR; }
    else if (!token_string_compare(yytext, "local"))    { insert_token(yylineno, yytext, "KEYWORD LOCAL", "enumerated"); return LOCAL; }
    else if (!token_string_compare(yytext, "true"))     { insert_token(yylineno, yytext, "KEYWORD TRUE", "enumerated"); return TRUE; }
    else if (!token_string_compare(yytext, "false"))    { insert_token(yylineno, yytext, "KEYWORD FALSE", "enumerated"); return FALSE; }
    else if (!token_string_compare(yytext, "nil"))      { insert_token(yylineno, yytext, "KEYWORD NIL", "enumerated"); return NIL; }
    else {
        insert_token(yylineno, yytext, "KEYWORD", "enumerated");
    }
    return 1;
}

{operator} {
    if (!token_string_compare(yytext, "+"))        { insert_token(yylineno, yytext, "OPERATOR PLUS", "enumerated"); return PLUS; }
    else if (!token_string_compare(yytext, "-"))   { insert_token(yylineno, yytext, "OPERATOR MINUS", "enumerated"); return MINUS; }
    else if (!token_string_compare(yytext, "*"))   { insert_token(yylineno, yytext, "OPERATOR MULT", "enumerated"); return MULTIPLY; }
    else if (!token_string_compare(yytext, "/"))   { insert_token(yylineno, yytext, "OPERATOR DIVIDE", "enumerated"); return DIVIDE; }
    else if (!token_string_compare(yytext, "%"))   { insert_token(yylineno, yytext, "OPERATOR MODULO", "enumerated"); return MODULO; }
    else if (!token_string_compare(yytext, "++"))  { insert_token(yylineno, yytext, "OPERATOR PLUS_PLUS", "enumerated"); return PLUS_PLUS; }
    else if (!token_string_compare(yytext, "--"))  { insert_token(yylineno, yytext, "OPERATOR MINUS_MINUS", "enumerated"); return MINUS_MINUS; }
    else if (!token_string_compare(yytext, "="))   { insert_token(yylineno, yytext, "OPERATOR ASSIGNMENT", "enumerated"); return ASSIGNMENT; }
    else if (!token_string_compare(yytext, "=="))  { insert_token(yylineno, yytext, "OPERATOR EQUAL_EQUAL", "enumerated"); return EQUAL_EQUAL; }
    else if (!token_string_compare(yytext, "!="))  { insert_token(yylineno, yytext, "OPERATOR NOT_EQUAL", "enumerated"); return NOT_EQUAL; }
    else if (!token_string_compare(yytext, ">"))   { insert_token(yylineno, yytext, "OPERATOR GREATER_THAN", "enumerated"); return GREATER_THAN;}
    else if (!token_string_compare(yytext, "<"))   { insert_token(yylineno, yytext, "OPERATOR LESS_THAN", "enumerated"); return LESS_THAN; }
    else if (!token_string_compare(yytext, ">="))  { insert_token(yylineno, yytext, "OPERATOR GREATER_EQUAL", "enumerated"); return GREATER_EQUAL; }
    else if (!token_string_compare(yytext, "<="))  { insert_token(yylineno, yytext, "OPERATOR LESS_EQUAL", "enumerated"); return LESS_EQUAL; }
    else if (!token_string_compare(yytext, "&&"))  { insert_token(yylineno, yytext, "OPERATOR AND", "enumerated"); return AND; }
    else if (!token_string_compare(yytext, "||"))  { insert_token(yylineno, yytext, "OPERATOR OR", "enumerated"); return OR; }
    else {
        /* If none of the above operators doesnt match then just store it as a general OPERATOR. */
        insert_token(yylineno, yytext, "OPERATOR", "enumerated");
    }
    return 1;
}

"::" { insert_token(yylineno, yytext, "PUNCTUATION COLON_COLON", "enumerated"); return COLON_COLON; }
".." { insert_token(yylineno, yytext, "PUNCTUATION DOT_DOT", "enumerated"); return DOT_DOT; 

/* due to ascii 46 error for ".." i split these into top level rules */ }


{punctuation} {
    if (!token_string_compare(yytext, "("))       { insert_token(yylineno, yytext, "PUNCTUATION LEFT_PARENTHESIS", "enumerated"); return LEFT_PARENTHESIS; }
    else if (!token_string_compare(yytext, ")"))  { insert_token(yylineno, yytext, "PUNCTUATION RIGHT_PARENTHESIS", "enumerated"); return RIGHT_PARENTHESIS;  }
    else if (!token_string_compare(yytext, "["))  { insert_token(yylineno, yytext, "PUNCTUATION LEFT_BRACKET", "enumerated"); return LEFT_BRACKET; }
    else if (!token_string_compare(yytext, "]"))  { insert_token(yylineno, yytext, "PUNCTUATION RIGHT_BRACKET", "enumerated"); return RIGHT_BRACKET; }
    else if (!token_string_compare(yytext, "{"))  { insert_token(yylineno, yytext, "PUNCTUATION LEFT_BRACE", "enumerated"); return LEFT_BRACE; }
    else if (!token_string_compare(yytext, "}"))  { insert_token(yylineno, yytext, "PUNCTUATION RIGHT_BRACE", "enumerated"); return RIGHT_BRACE; }
    else if (!token_string_compare(yytext, ";"))  { insert_token(yylineno, yytext, "PUNCTUATION SEMICOLON", "enumerated"); return SEMICOLON;}
    else if (!token_string_compare(yytext, "."))  { insert_token(yylineno, yytext, "PUNCTUATION DOT", "enumerated"); return DOT; }
    else if (!token_string_compare(yytext, ":"))  { insert_token(yylineno, yytext, "PUNCTUATION COLON", "enumerated"); return COLON; }
    else if (!token_string_compare(yytext, ","))  { insert_token(yylineno, yytext, "PUNCTUATION COMMA", "enumerated"); return COMMA; }
    else {
        insert_token(yylineno, yytext, "PUNCTUATION", "enumerated");
    }
    return 1;
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
    return STRING;
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