
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
    int yyerror (char* yaccProvidedMessage);

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
%token <stringValue> PLUS MINUS MULTIPLY DIVIDE ASSIGNMENT EQUAL NOT_EQUAL GREATER_THAN GREATER_EQUAL LESS_THAN LESS_EQUAL
%token <stringValue> LEFT_PARENTHESIS RIGHT_PARENTHESIS
%token <stringValue> LEFT_BRACE RIGHT_BRACE LEFT_BRACKET RIGHT_BRACKET
%token <stringValue> SEMICOLON COMMA COLON
%token <stringValue> IDENTIFIER INTCONST REALCONST STRING
%token <stringValue> FUNCTION AND OR NOT MODULO PLUS_PLUS MINUS_MINUS EQUAL_EQUAL LESS GREATER
%token <stringValue> DOT DOT_DOT COLON_COLON PUNCTUATION OPERATOR


%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

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
    : IDENTIFIER { print_rule("expr -> IDENTIFIER"); }
    | IDENTIFIER ASSIGNMENT expr { print_rule("expr -> IDENTIFIER ASSIGNMENT expr"); }
    | INTCONST { print_rule("expr -> INTCONST"); }
    | REALCONST { print_rule("expr -> REALCONST"); }
    | STRING { print_rule("expr -> STRING"); }
    | LEFT_PARENTHESIS expr RIGHT_PARENTHESIS { print_rule("expr -> ( expr )"); }
    ;

ifstmt
    : IF LEFT_PARENTHESIS expr RIGHT_PARENTHESIS stmt %prec LOWER_THAN_ELSE { print_rule("ifstmt -> if ( expr ) stmt"); }
    | IF LEFT_PARENTHESIS expr RIGHT_PARENTHESIS stmt ELSE stmt { print_rule("ifstmt -> if ( expr ) stmt else stmt"); }
    ;


whilestmt
    : WHILE LEFT_PARENTHESIS expr RIGHT_PARENTHESIS stmt { print_rule("whilestmt -> while ( expr ) stmt"); }
    ;

forstmt
    : FOR LEFT_PARENTHESIS /* elist */ SEMICOLON expr SEMICOLON /* elist */ RIGHT_PARENTHESIS stmt { print_rule("forstmt -> for ( ... ) stmt"); }
    ;

returnstmt
    : RETURN SEMICOLON { print_rule("returnstmt -> return ;"); }
    | RETURN expr SEMICOLON { print_rule("returnstmt -> return expr ;"); }
    ;

break_stmt
    : BREAK SEMICOLON { print_rule("break_stmt -> break ;"); }
    ;

continue_stmt
    : CONTINUE SEMICOLON { print_rule("continue_stmt -> continue ;"); }
    ;

block
    : LEFT_BRACE stmt_list RIGHT_BRACE { print_rule("block -> { stmt_list }"); }
    | LEFT_BRACE RIGHT_BRACE { print_rule("block -> { }"); }
    ;

%%

int yyerror(char* yaccProvidedMessage) {
    fprintf(stderr, "%s: at line %d, before token: %s\n", yaccProvidedMessage, yylineno, yytext);
    fprintf(stderr, "INPUT NOT VALID\n");
    return 1;
}
