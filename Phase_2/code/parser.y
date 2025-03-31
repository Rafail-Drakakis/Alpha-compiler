
/**
 * HY-340 Project Phase 2 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

/**
 * note: thema me ta comparison operators
 * so some of the tests don't work
 */

%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "symbol_table.h"
    #include "parser.h"  // Include Bison-generated header

    extern int yylineno;
    extern char* yytext;
    extern SymbolTable *symbol_table;
    extern int yylex();
    int yyerror (char* yaccProvidedMessage);

    void print_rule(const char* rule) {
        printf("Reduced by rule: %s\n", rule);
    }

    unsigned int checkScope = 0; // 0 for global, 1 for local
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


%right ASSIGNMENT   /* = has less priority in compare with all the other */
%left OR
%left AND
%nonassoc EQUAL_EQUAL NOT_EQUAL
%nonassoc GREATER_THAN GREATER_EQUAL LESS_THAN LESS_EQUAL
%left  PLUS 
%right MINUS /* x - y - z is referenced as x-(y-z). We are gonna think if we will need UMINUS for (x-y)-z*/
%left MULTIPLY DIVIDE MODULO
%right NOT
%right PLUS_PLUS
%right MINUS_MINUS

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%start program

%%

program
    : stmt_list { print_rule("program -> stmt_list"); }
    ;

stmt_list
    : stmt stmt_list { print_rule("stmt_list -> stmt stmt_list"); }
    | stmt { print_rule("stmt_list -> stmt"); } //single statement allowed for testng cause "x;" might not reduce properly
    | /* empty */ { print_rule("stmt_list -> epsilon"); }
    ;

stmt
    : expr SEMICOLON { print_rule("stmt -> expr ;"); }
    | ifstmt { print_rule("stmt -> ifstmt"); }
    | whilestmt { print_rule("stmt -> whilestmt"); }
    | forstmt { print_rule("stmt -> forstmt"); }
    | returnstmt { print_rule("stmt -> returnstmt"); }
    | break_stmt { print_rule("stmt -> break ;"); }
    | continue_stmt { print_rule("stmt -> continue ;"); }
    | block { print_rule("stmt -> block"); }
    | funcdef { print_rule("stmt -> funcdef"); }
    | error ';' { print_rule("stmt -> error ;"); yyerrok; }
    ;

expr
    : assignexpr { print_rule("expr -> assignexpr"); }
    | expr op expr { print_rule("expr -> expr op expr"); }
    | term { print_rule("expr -> term"); }  // Now ensures `term` is used
    ;

assignexpr
    : lvalue ASSIGNMENT expr { print_rule("assignexpr -> lvalue = expr"); }
    ;

op
    : PLUS        { print_rule("op -> +"); }
    | MINUS       { print_rule("op -> -"); }
    | MULTIPLY    { print_rule("op -> *"); }
    | DIVIDE      { print_rule("op -> /"); }
    | MODULO      { print_rule("op -> %"); }
    | GREATER_THAN { print_rule("op -> >"); }
    | GREATER_EQUAL { print_rule("op -> >="); }
    | LESS_THAN    { print_rule("op -> <"); }
    | LESS_EQUAL   { print_rule("op -> <="); }
    | EQUAL_EQUAL  { print_rule("op -> =="); }
    | NOT_EQUAL    { print_rule("op -> !="); }
    | AND          { print_rule("op -> and"); }
    | OR           { print_rule("op -> or"); }
    ;

term
    : LEFT_PARENTHESIS expr RIGHT_PARENTHESIS { print_rule("term -> ( expr )"); }
    | MINUS expr { print_rule("term -> - expr"); }
    | NOT expr { print_rule("term -> not expr"); }
    | PLUS_PLUS lvalue { print_rule("term -> ++ lvalue"); }
    | lvalue PLUS_PLUS { print_rule("term -> lvalue ++"); }
    | MINUS_MINUS lvalue { print_rule("term -> -- lvalue"); }
    | lvalue MINUS_MINUS { print_rule("term -> lvalue --"); }
    | primary { print_rule("term -> primary"); }
    ;

primary
    : lvalue { print_rule("primary -> lvalue"); }
    | call { print_rule("primary -> call"); }
    | objectdef { print_rule("primary -> objectdef"); }
    | LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS { print_rule("primary -> ( funcdef )"); }
    | const { print_rule("primary -> const"); }
    ;

lvalue
    : IDENTIFIER { 
        print_rule("lvalue -> IDENTIFIER"); 
        SymbolTableEntry *found_identifier = lookup_symbol(symbol_table, $1, checkScope);
        if (!found_identifier) {
            SymbolType st = (checkScope == 0) ? GLOBAL : LOCAL_VAR;
            insert_symbol(symbol_table, $1, st, yylineno, checkScope);
        }
    }
    | LOCAL IDENTIFIER { 
        print_rule("lvalue -> LOCAL IDENTIFIER"); 
        SymbolTableEntry *found_identifier = lookup_symbol(symbol_table, $2, checkScope);
        if (!found_identifier) {
            SymbolType st = (checkScope == 0) ? GLOBAL : LOCAL_VAR;
            insert_symbol(symbol_table, $2, st, yylineno, checkScope);
        }
    }
    | COLON_COLON IDENTIFIER { 
        print_rule("lvalue -> :: IDENTIFIER"); 
        SymbolTableEntry *found_identifier = lookup_symbol(symbol_table, $2, checkScope);
        if (!found_identifier) {
            SymbolType st = (checkScope == 0) ? GLOBAL : LOCAL_VAR;
            insert_symbol(symbol_table, $2, st, yylineno, checkScope);
        }
    }
    | member { print_rule("lvalue -> member"); }
    ;

const
    : INTCONST { print_rule("const -> INTCONST"); }
    | REALCONST { print_rule("const -> REALCONST"); }
    | STRING { print_rule("const -> STRING"); }
    | NIL { print_rule("const -> NIL"); }
    | TRUE { print_rule("const -> TRUE"); }
    | FALSE { print_rule("const -> FALSE"); }
    ;

member
    : lvalue DOT IDENTIFIER { print_rule("member -> lvalue . IDENTIFIER"); }
    | lvalue LEFT_BRACKET expr RIGHT_BRACKET { print_rule("member -> lvalue [ expr ]"); }
    | call DOT IDENTIFIER { print_rule("member -> call . IDENTIFIER"); }
    | call LEFT_BRACKET expr RIGHT_BRACKET { print_rule("member -> call [ expr ]"); }
    ;

call
    : call LEFT_PARENTHESIS elist RIGHT_PARENTHESIS { print_rule("call -> call ( elist )"); }
    | lvalue callsuffix { print_rule("call -> lvalue callsuffix"); }
    | LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS LEFT_PARENTHESIS elist RIGHT_PARENTHESIS { print_rule("call -> ( funcdef ) ( elist )"); }
    ;

callsuffix
    : normcall { print_rule("callsuffix -> normcall"); }
    | methodcall { print_rule("callsuffix -> methodcall"); }
    ;

normcall
    : LEFT_PARENTHESIS elist RIGHT_PARENTHESIS { print_rule("normcall -> ( elist )"); }
    ;

methodcall
    : lvalue DOT IDENTIFIER LEFT_PARENTHESIS lvalue elist RIGHT_PARENTHESIS { print_rule("methodcall -> lvalue . IDENTIFIER ( lvalue , elist )"); }
    ;

    | expr COMMA elist { print_rule("elist -> expr , elist"); }
    | /* empty */ { print_rule("elist -> epsilon"); }  // Allows empty argument lists
    ;

elist
    : expr { print_rule("elist -> expr"); }
    | expr COMMA elist { print_rule("elist -> expr , elist"); }
    | /* empty */ { print_rule("elist -> epsilon"); }  // Allows empty argument lists
    ;

objectdef
    : LEFT_BRACKET LEFT_BRACKET elist RIGHT_BRACKET RIGHT_BRACKET { print_rule("objectdef -> [ [ elist ] ]"); }
    | LEFT_BRACKET LEFT_BRACKET indexed RIGHT_BRACKET RIGHT_BRACKET { print_rule("objectdef -> [ [ indexed ] ]"); }
    ;

indexed
    : LEFT_BRACKET indexedelem { print_rule("indexed -> [ indexedelem ]"); }
    | LEFT_BRACKET indexedelem COMMA indexed { print_rule("indexed -> [ indexedelem , indexed ]"); }
    ;

indexedelem
    : LEFT_BRACE expr COLON expr RIGHT_BRACE { print_rule("indexedelem -> { expr : expr }"); }
    ;

funcdef
    : FUNCTION IDENTIFIER LEFT_PARENTHESIS idlist RIGHT_PARENTHESIS {
        SymbolTableEntry *found_identifier = lookup_symbol(symbol_table, $2, checkScope);
        if (!found_identifier) {
            SymbolType st = (checkScope == 0) ? GLOBAL : LOCAL_VAR;
            insert_symbol(symbol_table, $2, st, yylineno, checkScope);
        }
        checkScope++; 
    } block { checkScope--; print_rule("funcdef -> function [ IDENTIFIER ] ( idlist ) block"); }
    ;

idlist
    : IDENTIFIER { print_rule("idlist -> IDENTIFIER"); }
    | IDENTIFIER COMMA idlist { print_rule("idlist -> IDENTIFIER , idlist"); }
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
    : LEFT_BRACE { checkScope++; } stmt_list RIGHT_BRACE { checkScope--; print_rule("block -> { stmt_list }"); }
    | LEFT_BRACE RIGHT_BRACE { checkScope++; checkScope--; print_rule("block -> { }"); }
    ;

%%

int yyerror(char* yaccProvidedMessage) {
    fprintf(stderr, "%s: at line %d, before token: %s\n", yaccProvidedMessage, yylineno, yytext);
    fprintf(stderr, "unexpected token: %s with ascii: %d\n", yytext, yytext[0]);
    fprintf(stderr, "INPUT NOT VALID\n");
    return 1;
}