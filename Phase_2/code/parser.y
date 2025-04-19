
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
    extern SymbolTable *symbol_table;
    extern int yylex();
    int yyerror (char* yaccProvidedMessage);

    void print_rule(const char* rule) {
        // printf("Reduced by rule: %s\n", rule);
        (void)0;
    }

    unsigned int checkScope = 0; // 0 for global, 1 for local
    int checkLoopDepth = 0;
    int inside_function_scope = 0;

    void enter_scope() {
        checkScope++;
    }

    void exit_scope() {
        deactivate_entries_from_curr_scope(symbol_table, checkScope);
        checkScope--;
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
%token <stringValue> DOT_DOT DOT COLON_COLON PUNCTUATION OPERATOR


%right ASSIGNMENT   /* = has less priority in compare with all the other */
%left OR
%left AND
%nonassoc EQUAL_EQUAL NOT_EQUAL
%nonassoc GREATER_THAN GREATER_EQUAL LESS_THAN LESS_EQUAL
%left  PLUS 
%left MINUS /* changed to UMINUS for (x-y) - z */
%left MULTIPLY DIVIDE MODULO
%right NOT
%right PLUS_PLUS
%right MINUS_MINUS


%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%nonassoc UMINUS /* unary minus operator */

// %expect 2 // Expect 2 conflicts to be resolved
// changed this gt efaga error: shift/reduce conflicts: 0 found, 2

%start program

%%

program
    : stmt_list { print_rule("program -> stmt_list"); }
    ;

stmt_list
    : stmt stmt_list { print_rule("stmt_list -> stmt stmt_list"); }
    //| stmt { print_rule("stmt_list -> stmt"); } //single statement allowed for testng cause "x;" might not reduce properly
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
    : expr op { print_rule("expr -> expr op expr"); }
    | assignexpr { print_rule("expr -> assignexpr"); }
    | term { print_rule("expr -> term"); }  // Now ensures `term` is used
    //| call { print_rule("expr -> call"); } We will remove it to avoid conflict and also we dont need that as we can follow this path: [expr -> term -> primary -> call]
    ;

assignexpr
    : lvalue ASSIGNMENT expr { print_rule("assignexpr -> lvalue = expr"); }
    ;

op
  : plus_op expr       %prec PLUS
  | minus_op expr      %prec MINUS
  | mult_op expr       %prec MULTIPLY
  | div_op expr        %prec DIVIDE
  | mod_op expr        %prec MODULO
  | greaterthan_op expr %prec GREATER_THAN
  | greaterequal_op expr %prec GREATER_EQUAL
  | lessthan_op expr     %prec LESS_THAN
  | lessequal_op expr    %prec LESS_EQUAL
  | eqeq_op expr       %prec EQUAL_EQUAL
  | noteq_op expr      %prec NOT_EQUAL
  | and_op expr        %prec AND
  | or_op expr         %prec OR
  ;

plus_op: PLUS { print_rule("op -> +"); };
minus_op: MINUS { print_rule("op -> -"); };
mult_op: MULTIPLY { print_rule("op -> *"); };
div_op: DIVIDE { print_rule("op -> /"); };
mod_op: MODULO { print_rule("op -> %"); };
greaterthan_op: GREATER_THAN { print_rule("op -> >"); };
greaterequal_op: GREATER_EQUAL { print_rule("op -> >="); };
lessthan_op: LESS_THAN { print_rule("op -> <"); };
lessequal_op: LESS_EQUAL { print_rule("op -> <="); };
eqeq_op: EQUAL_EQUAL { print_rule("op -> =="); };
noteq_op: NOT_EQUAL { print_rule("op -> !="); };
and_op: AND { print_rule("op -> and"); };
or_op: OR { print_rule("op -> or"); };

term
    : LEFT_PARENTHESIS expr RIGHT_PARENTHESIS { print_rule("term -> ( expr )"); }
    | MINUS expr %prec UMINUS { print_rule("term -> - expr"); }
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
         SymbolTableEntry *found_identifier = lookup_symbol(symbol_table, $1, checkScope, inside_function_scope);
         if (found_identifier) {
             if (found_identifier->type == USER_FUNCTION || found_identifier->type == LIBRARY_FUNCTION) {
                 fprintf(stderr, "Error: Symbol '%s' is not a valid lvalue (line %d).\n", $1, yylineno);
             }
         } else {
             SymbolType st = (checkScope == 0) ? GLOBAL : LOCAL_VAR;
             insert_symbol(symbol_table, $1, st, yylineno, checkScope);
         }
    }
    | LOCAL IDENTIFIER { 
        print_rule("lvalue -> LOCAL IDENTIFIER"); 
        SymbolTableEntry *found_identifier = lookup_symbol(symbol_table, $2, checkScope, inside_function_scope);
        if (!found_identifier) {
            SymbolType st = (checkScope == 0) ? GLOBAL : LOCAL_VAR;
            insert_symbol(symbol_table, $2, st, yylineno, checkScope);
        }
    }
    | COLON_COLON IDENTIFIER { 
        print_rule("lvalue -> :: IDENTIFIER"); 
        SymbolTableEntry *found_identifier = lookup_symbol(symbol_table, $2, 0, 0); // Check global scope only
        if (!found_identifier) {
            fprintf(stderr, "Error: Symbol '%s' not found in global scope at line %d\n", $2, yylineno);
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
    | call_member { print_rule("member -> call_member"); }
    ;

/* Helpful for member */
call_member
    : call DOT IDENTIFIER { print_rule("call_member -> call . IDENTIFIER"); }
    | call LEFT_BRACKET expr RIGHT_BRACKET { print_rule("call_member -> call [ expr ]"); }
    ;

call
    : call LEFT_PARENTHESIS elist RIGHT_PARENTHESIS { print_rule("call -> call (elist)"); }
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
    : lvalue DOT_DOT IDENTIFIER LEFT_PARENTHESIS elist RIGHT_PARENTHESIS { print_rule("methodcall -> lvalue .. IDENTIFIER ( elist )"); }
    ;

elist
    : expr { print_rule("elist -> expr"); }
    | expr COMMA elist { print_rule("elist -> expr , elist"); }
    | /* empty */ { print_rule("elist -> epsilon"); }  // Allows empty argument lists
    ;

objectdef
    : LEFT_BRACKET elist RIGHT_BRACKET { print_rule("objectdef -> [ elist ]"); }
    | LEFT_BRACKET indexed RIGHT_BRACKET { print_rule("objectdef -> [ indexed ]"); }
    ;

// can be empty but due to shift reduce to ebgala 
indexed
    : indexedelem { print_rule("indexed -> indexedelem"); }
    | indexedelem COMMA indexed { print_rule("indexed -> indexedelem, indexed"); }
    // | /* empty */ { print_rule("indexed -> epsilon"); }
    ;

indexedelem
    : LEFT_BRACE expr COLON expr RIGHT_BRACE { print_rule("indexedelem -> { expr : expr }"); }
    ;

funcdef
    : FUNCTION IDENTIFIER LEFT_PARENTHESIS idlist RIGHT_PARENTHESIS {
          SymbolTableEntry *found_identifier = lookup_symbol(symbol_table, $2, checkScope, inside_function_scope);
          if (!found_identifier) {
              insert_symbol(symbol_table, $2, USER_FUNCTION, yylineno, checkScope);
          }
          enter_scope(); // increse scope for funcargs
          inside_function_scope = 1; // we enter function
      } block { 
          inside_function_scope = 0; // we get out of function
          exit_scope();
	  print_rule("funcdef -> function [ IDENTIFIER ] ( idlist ) block"); 
      }
    | FUNCTION LEFT_PARENTHESIS idlist RIGHT_PARENTHESIS {
          char *anonymus_name = malloc(32);
	  if(!anonymus_name){
	  	fprintf(stderr, "Memory Allocation failed\n");
		exit(EXIT_FAILURE);
	  }
          sprintf(anonymus_name, "_anonymus_func_%d", yylineno); // debug print
	  insert_symbol(symbol_table, anonymus_name, USER_FUNCTION, yylineno, checkScope);
          inside_function_scope = 1;
      } block {
          inside_function_scope = 0;
          print_rule("funcdef -> function ( idlist ) block");
      }
    ;

idlist
    : IDENTIFIER { 
        print_rule("idlist -> IDENTIFIER"); 
        /* Pass to symbol table as ARGUMENT */
        insert_symbol(symbol_table, $1, ARGUMENT, yylineno, checkScope);
    }
    | IDENTIFIER COMMA idlist { 
        print_rule("idlist -> IDENTIFIER , idlist"); 
        insert_symbol(symbol_table, $1, ARGUMENT, yylineno, checkScope);
    }
    | /* empty */ { print_rule("idlist -> epsilon"); }
    ;

ifstmt
    : IF LEFT_PARENTHESIS expr RIGHT_PARENTHESIS stmt %prec LOWER_THAN_ELSE { print_rule("ifstmt -> if ( expr ) stmt"); }
    | IF LEFT_PARENTHESIS expr RIGHT_PARENTHESIS stmt ELSE stmt { print_rule("ifstmt -> if ( expr ) stmt else stmt"); }
    ;


whilestmt
    : WHILE LEFT_PARENTHESIS expr RIGHT_PARENTHESIS { checkLoopDepth++; } stmt { checkLoopDepth--; print_rule("whilestmt -> while ( expr ) stmt"); }
    ;

forstmt
    : FOR LEFT_PARENTHESIS elist SEMICOLON expr SEMICOLON elist RIGHT_PARENTHESIS { checkLoopDepth++; } stmt { checkLoopDepth--; print_rule("forstmt -> for ( elist ; expr ; elist ) stmt"); }
    ;

returnstmt
    : RETURN SEMICOLON { print_rule("returnstmt -> return ;"); }
    | RETURN expr SEMICOLON { print_rule("returnstmt -> return expr ;"); }
    ;

break_stmt
    : BREAK SEMICOLON { if(checkLoopDepth < 1){ fprintf(stderr, "Error: 'break' used outside of any loop (line %d)\n", yylineno); } print_rule("break_stmt -> break ;"); }
    ;

continue_stmt
    : CONTINUE SEMICOLON { if(checkLoopDepth < 1){ fprintf(stderr, "Error: 'continue' used outside of any loop (line %d)\n", yylineno); } print_rule("continue_stmt -> continue ;"); }
    ;

block
    : LEFT_BRACE { enter_scope(); } stmt_list RIGHT_BRACE { exit_scope();  print_rule("block -> { stmt_list }"); }
    ;

%%

int yyerror(char* yaccProvidedMessage) {
    fprintf(stderr, "%s: at line %d, before token: %s\n", yaccProvidedMessage, yylineno, yytext);
    fprintf(stderr, "unexpected token: %s with ascii: %d\n", yytext, yytext[0]);
    fprintf(stderr, "INPUT NOT VALID\n");
    return 1;
}
