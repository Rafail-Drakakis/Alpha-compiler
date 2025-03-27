
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
    #include "symbol_table.h"
    #include "parser.h"  // Include Bison-generated header
    extern int yylineno;
    extern char* yytext;
    extern int yylex();

    void print_rule(const char* rule) {
        printf("Reduced by rule: %s\n", rule);
    }
%}

%union {
    int intValue;
    double realValue;
    char* stringValue;
}

%token <stringValue> IF ELSE WHILE FOR RETURN BREAK CONTINUE LOCAL TRUE FALSE NIL
%token <stringValue> PLUS MINUS MULTIPLY DIVIDE ASSIGN EQUAL NOT_EQUAL GREATER_THAN GREATER_EQUAL LESS_THAN LESS_EQUAL
%token <stringValue> LEFT_PARENTHESIS RIGHT_PARENTHESIS
%token <stringValue> LEFT_BRACE RIGHT_BRACE LEFT_BRACKET RIGHT_BRACKET
%token <stringValue> SEMICOLON COMMA COLON
%token <stringValue> IDENTIFIER INTCONST REALCONST STRING
%token <stringValue> FUNCTION AND OR NOT MODULO PLUS_PLUS MINUS_MINUS EQUAL_EQUAL LESS GREATER
%token <stringValue> DOT DOT_DOT COLON_COLON PUNCTUATION OPERATOR

%start program

%%

program
    : stmt_list { print_rule("program -> stmt_list"); }
    ;

stmt_list
    : stmt stmt_list { print_rule("stmt_list -> stmt stmt_list"); }
    | /* empty */ { print_rule("stmt_list -> epsilon"); }
    ;

stmt
    : expr ';' { print_rule("stmt -> expr ;"); }
    | ifstmt { print_rule("stmt -> ifstmt"); }
    | whilestmt { print_rule("stmt -> whilestmt"); }
    | forstmt { print_rule("stmt -> forstmt"); }
    | returnstmt { print_rule("stmt -> returnstmt"); }
    | break_stmt { print_rule("stmt -> break ;"); }
    | continue_stmt { print_rule("stmt -> continue ;"); }
    | block { print_rule("stmt -> block"); }
    ;

expr
    : IDENTIFIER ASSIGN expr { print_rule("expr -> IDENTIFIER ASSIGN expr"); }
    | INTCONST { print_rule("expr -> INTCONST"); }
    | REALCONST { print_rule("expr -> REALCONST"); }
    | STRING { print_rule("expr -> STRING"); }
    | '(' expr ')' { print_rule("expr -> ( expr )"); }
    ;

%%

int yyerror(char* yaccProvidedMessage) {
    fprintf(stderr, "%s: at line %d, before token: %s\n", yaccProvidedMessage, yylineno, yytext);
    fprintf(stderr, "INPUT NOT VALID\n");
    return 1;
}
