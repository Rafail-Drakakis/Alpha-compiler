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
#include "sym_table.h"

int yyerror (char* yaccProvidedMessage);
extern int yylineno;
extern char* yytext;
extern FILE* yyin;

%}

%token SEMICOLON
%token IDENTIFIER
%token IF
%token ELSE
/* ... etc., we will add more based on al.l after */

%start program

%%
program
    : /* empty */
    ;

%%

int yyerror (char* yaccProvidedMessage) {
    fprintf(stderr, "%s: at line %d, before token: %s\n",yaccProvidedMessage,yylineno,yytext);
    fprintf(stderr,"INPUT NOT VALID\n");
    return 1;
}