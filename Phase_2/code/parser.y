/**
 * HY-340 Project Phase 2 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

%{
#include <stdio.h>
#include <stdlib.h>

/* from al.l */
extern int yylex(void);

void yyerror(const char* msg) {
    fprintf(stderr, "Parse Error: %s\n", msg);
}
%}

%token SEMICOLON
%token IDENTIFIER
%token IF
%token ELSE
/* ... etc., we will add more based on al.l after */

%start program

%%
/* Define the non-terminated 'program' */
program
    : /* empty */
    ;

%%