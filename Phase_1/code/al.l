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

/* Token Structure */
typedef struct alpha_token_t {
    unsigned int line_number;
    unsigned int token_number;
    char *content;
    char *token_type;
    struct alpha_token_t *next_t;
} alpha_token_t;

alpha_token_t* list_head = NULL;
int counter_tk_number = 0;

/* Function to insert tokens into the list */
void insert_token(int line, const char* text, const char* type) {
    alpha_token_t* new_token = (alpha_token_t*)malloc(sizeof(alpha_token_t));
    new_token->line_number = line;
    new_token->token_number = ++counter_tk_number;
    new_token->content = strdup(text);
    new_token->token_type = strdup(type);
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

%}

%option yylineno
%option noyywrap

/* Regular Expressions */
id          [a-zA-Z][a-zA-Z_0-9]*
intconst    (0[xX][0-9A-Fa-f]+)|([0-9]+)
realconst   [0-9]+\.[0-9]+(e[+-]?[0-9]+)?
string      \"([^\n"\\]|\\.)*\"
whitespace  [ \t\r\n]+
comment1    \/\/.*
comment2    \/\*([^*]|\*+[^*/])*\*+\/
operator    (\+|\-|\*|\/|=|%|\+\+|\-\-|==|!=|>|<|>=|<=)
keyword     (function|while|for|if|else|continue|break|return|and|or|not|local|true|false|nil)

%%
{keyword}    { insert_token(yylineno, yytext, "KEYWORD"); return 1; }
{operator}   { insert_token(yylineno, yytext, "OPERATOR"); return 1; }
{id}         { insert_token(yylineno, yytext, "IDENTIFIER"); return 1; }
{intconst}   { insert_token(yylineno, yytext, "INTCONST"); return 1; }
{realconst}  { insert_token(yylineno, yytext, "REALCONST"); return 1; }
{string}     { insert_token(yylineno, yytext, "STRING"); return 1; }
{comment1}   {  }
{comment2}   {  }
{whitespace} {  }
.   { /* Ignore unrecognized characters */ }

%%
/* Function to print tokens */
void print_output() {
    alpha_token_t* temp = list_head;
    while (temp) {
        printf("%d: #%d \"%s\" %s\n", temp->line_number, temp->token_number, temp->content, temp->token_type);
        temp = temp->next_t;
    }
}

int main(int argc, char *argv[]) {
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
    return EXIT_SUCCESS;
}