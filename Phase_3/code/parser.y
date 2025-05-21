/**
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include "symbol_table.h"
    #include "parser.h"
    #include "quads.h"

    extern int yylineno;
    extern char* yytext;
    extern SymbolTable *symbol_table;
    extern int yylex();
    int yyerror (char* yaccProvidedMessage);
    int anonymus_function_counter = 0;

    void print_rule(const char* rule) {
        (void)0; // printf("Reduced by rule: %s\n", rule);
    }

    unsigned int checkScope = 0;        // 0 for global, 1 for local
    int checkLoopDepth = 0;
    int inside_function_scope = 0;
    int inside_function_depth = 0;      // 0 for global, >0 for function scope
    static int first_brace_of_func = 0;
    int is_calling = 0;			        // reducing lvalue for function call 1, normal lvalues 0
    expr* current_function_expr = NULL;
    char* current_function_char = NULL;


    typedef struct formal_argument_node {
        char *name;
        struct formal_argument_node *next;
    } formal_argument_node;

    formal_argument_node *create_node(char *name) {
        formal_argument_node *node = (formal_argument_node *)malloc(sizeof(formal_argument_node));
        node->name = strdup(name);
        node->next = NULL;
        return node;
    }

    formal_argument_node *append_argument(formal_argument_node *list, char *name) {
        formal_argument_node *node = create_node(name);
        node->next = list;
        return node;
    }

    void enter_scope() {
        // printf("Entering new scope: %u\n", checkScope);
        checkScope++;
    }

    static void exit_scope(void) {
        if (checkScope == 0) {
            return;
	}

        // printf("Exiting  scope: %u\n", checkScope-1);
        deactivate_entries_from_curr_scope(symbol_table, checkScope-1);
        --checkScope;
    }

%}

%union {
    int intValue;
    double realValue;
    char* stringValue;
    struct formal_argument_node* arglist;
    struct SymbolTableEntry* symbol;    // for lvalue
    struct expr *expression;            // for expressions
}

%token <stringValue> IF ELSE WHILE FOR RETURN BREAK CONTINUE LOCAL TRUE FALSE NIL
%token <stringValue> PLUS MINUS MULTIPLY DIVIDE ASSIGNMENT EQUAL NOT_EQUAL GREATER_THAN GREATER_EQUAL LESS_THAN LESS_EQUAL
%token <stringValue> LEFT_PARENTHESIS RIGHT_PARENTHESIS
%token <stringValue> LEFT_BRACE RIGHT_BRACE LEFT_BRACKET RIGHT_BRACKET
%token <stringValue> SEMICOLON COMMA COLON
%token <stringValue> IDENTIFIER STRING
%token <intValue>   INTCONST
%token <realValue>  REALCONST
%token <stringValue> FUNCTION AND OR NOT MODULO PLUS_PLUS MINUS_MINUS EQUAL_EQUAL LESS GREATER
%token <stringValue> DOT_DOT DOT COLON_COLON PUNCTUATION OPERATOR
%type <expression> funcdef

%type <arglist>     idlist formal_arguments
%type <expression> 	expr term primary const lvalue member assignexpr call elist normcall methodcall callsuffix
%type <intValue>	ifprefix elseprefix ifstmt stmt
%type <expression>  call_member indexed indexedelem objectdef

%right ASSIGNMENT        /* = has less priority in compare with all the other */
%left OR
%left AND
%nonassoc EQUAL_EQUAL NOT_EQUAL
%nonassoc GREATER_THAN GREATER_EQUAL LESS_THAN LESS_EQUAL
%left  PLUS 
%left MINUS              /* changed to UMINUS for (x-y) - z */
%left MULTIPLY DIVIDE MODULO
%right NOT
%right PLUS_PLUS
%right MINUS_MINUS
%right DOT_DOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%nonassoc UMINUS         /* unary minus operator */

// %expect 2

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
    : expr SEMICOLON { if($1->type == constnum_e) { expr *temp = newexpr(var_e); temp->sym = newtemp();
      emit(add, newexpr_constnum($1->numConst), NULL, temp, 0, yylineno); }
      print_rule("stmt -> expr ;"); }
    | error SEMICOLON { print_rule("stmt -> error ;"); yyerrok; }
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
    : expr PLUS expr
    {
        // Direct operation for both variables and constants
        expr *r = newexpr(arithexpr_e);
        r->sym = newtemp();
        emit(add, $1, $3, r, 0, yylineno);
        $$ = r;
    }
    | expr MINUS expr
    {
        expr *r = newexpr(arithexpr_e);
        r->sym = newtemp();
        emit(sub, $1, $3, r, 0, yylineno);
        $$ = r;
    }
    | expr MULTIPLY expr
    {
        expr *r = newexpr(arithexpr_e);
        r->sym = newtemp();
        emit(mul, $1, $3, r, 0, yylineno);
        $$ = r;
    }
    | expr DIVIDE expr
    {
        expr *r = newexpr(arithexpr_e);
        r->sym = newtemp();
        emit(idiv, $1, $3, r, 0, yylineno);
        $$ = r;
    }
    | expr MODULO expr
    {
        expr *r = newexpr(arithexpr_e);
        r->sym = newtemp();
        emit(mod, $1, $3, r, 0, yylineno);
        $$ = r;
    }
    | expr GREATER_THAN expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();
        emit(if_greater, $1, $3, r, 0, yylineno);
        $$ = r;
    }
    | expr GREATER_EQUAL expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();
        emit(if_greatereq, $1, $3, r, 0, yylineno);
        $$ = r;
    }
    | expr LESS_THAN expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();
        emit(if_less, $1, $3, r, 0, yylineno);
        $$ = r;
    }
    | expr LESS_EQUAL expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();
        emit(if_lesseq, $1, $3, r, 0, yylineno);
        $$ = r;
    }
    | expr EQUAL_EQUAL expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();
        emit(if_eq, $1, $3, r, 0, yylineno);
        $$ = r;
    }
    | expr NOT_EQUAL expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();
        emit(if_noteq, $1, $3, r, 0, yylineno);
        $$ = r;
    }

    | expr AND expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();

        emit(and, $1, $3, r, 0, yylineno);
        $$ = r;
    }

    | expr OR expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();

        emit(or, $1, $3, r, 0, yylineno);
        $$ = r;
    }

    | assignexpr { $$ = $1; }
    | term       { $$ = $1; } 
    | expr DOT_DOT expr { print_rule("expr DOT_DOT expr"); }
    ;

assignexpr
    : lvalue ASSIGNMENT expr
    {
        if($1->type == programfunc_e || $1->type == libraryfunc_e)
            fprintf(stderr,"Error: Symbol '%s' is not a valid l-value (line %d)\n",
                    $1->sym->name, yylineno);
        
        if($1->type == tableitem_e) {
            // Table element assignment
            emit(tablesetelem, $3, $1->index, $1, 0, yylineno);
            
            // Create a temporary for the result
            expr *result = newexpr(var_e);
            result->sym = newtemp();
            emit(tablegetelem, $1, $1->index, result, 0, yylineno);
            $$ = result;
        } else {
            // Regular variable assignment
            emit(assign, $3, NULL, $1, 0, yylineno);
            
            // Add extra assignment after the main assignment
            expr *final = newexpr(var_e);
            final->sym = newtemp();
            emit(assign, $1, NULL, final, 0, yylineno);
            
            $$ = $1;
        }
    }
;

/*
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

    plus_op:            PLUS { print_rule("op -> +"); };
    minus_op:           MINUS { print_rule("op -> -"); };
    mult_op:            MULTIPLY { print_rule("op -> *"); };
    div_op:             DIVIDE { print_rule("op -> /"); };
    mod_op:             MODULO { print_rule("op -> %"); };
    greaterthan_op:     GREATER_THAN { print_rule("op -> >"); };
    greaterequal_op:    GREATER_EQUAL { print_rule("op -> >="); };
    lessthan_op:        LESS_THAN { print_rule("op -> <"); };
    lessequal_op:       LESS_EQUAL { print_rule("op -> <="); };
    eqeq_op:            EQUAL_EQUAL { print_rule("op -> =="); };
    noteq_op:           NOT_EQUAL { print_rule("op -> !="); };
    and_op:             AND { print_rule("op -> and"); };
    or_op:              OR { print_rule("op -> or"); };
*/

term
    : LEFT_PARENTHESIS expr RIGHT_PARENTHESIS { print_rule("term -> ( expr )"); }
    | MINUS expr %prec UMINUS 
        { 
            expr *r = newexpr(arithexpr_e); 
            r->sym = newtemp(); 
            emit(uminus, $2, NULL, r, 0, yylineno); 
            $$ = r; print_rule("term -> - expr"); 
        }
    | NOT expr 
        { 
            print_rule("term -> not expr"); 
        }
    | PLUS_PLUS lvalue 
        { 
            if ($2->type == programfunc_e || $2->type == libraryfunc_e) fprintf(stderr,"Error: Symbol '%s' is not a modifiable lvalue (line %d).\n", $2->sym->name, yylineno); 
            expr *r = newexpr(var_e); 
            r->sym = newtemp(); 
            emit(assign, $2, NULL, r, 0, yylineno); 
            emit(add, $2, newexpr_constnum(1), $2, 0, yylineno); 
            $$ = r; 
        }
    | lvalue PLUS_PLUS 
        { 
            if ($1->type == programfunc_e || $1->type == libraryfunc_e) fprintf(stderr,"Error: Symbol '%s' is not a modifiable lvalue (line %d).\n", $1->sym->name, yylineno); 
            expr *r = newexpr(var_e); 
            r->sym = newtemp(); 
            emit(assign, $1, NULL, r, 0, yylineno); 
            emit(add, $1, newexpr_constnum(1), $1, 0, yylineno); 
            $$ = r; 
        }
    | MINUS_MINUS lvalue 
        { 
            if ($2->type == programfunc_e || $2->type == libraryfunc_e) fprintf(stderr, "Error: Symbol '%s' is not a modifiable lvalue (line %d).\n", $2->sym->name, yylineno);
            expr *r = newexpr(var_e); 
            r->sym = newtemp(); 
            emit(assign, $2, NULL, r, 0, yylineno); 
            emit(sub, $2, newexpr_constnum(1), $2, 0, yylineno); 
            $$ = r; 
        }
    | lvalue MINUS_MINUS 
        { 
            if ($1->type == programfunc_e || $1->type == libraryfunc_e) fprintf(stderr, "Error: Symbol '%s' is not a modifiable lvalue (line %d).\n", $1->sym->name, yylineno); 
            expr *r = newexpr(var_e); 
            r->sym = newtemp(); 
            emit(assign, $1, NULL, r, 0, yylineno); 
            emit(sub, $1, newexpr_constnum(1), $1, 0, yylineno); 
            $$ = r; 
            }
    | primary 
        { 
            print_rule("term -> primary"); 
        }
    ;

primary
    : lvalue { print_rule("primary -> lvalue"); }
    | call { print_rule("primary -> call"); }
    | objectdef { print_rule("primary -> objectdef"); }
    | LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS { print_rule("primary -> ( funcdef )"); }
    | const { print_rule("primary -> const"); }
    ;

lvalue
    : IDENTIFIER
      {
          SymbolTableEntry *sym = lookup_symbol(symbol_table, $1, checkScope, inside_function_scope);
          if (!sym) {
              insert_symbol(symbol_table, $1, (checkScope==0)? GLOBAL : LOCAL_VAR, yylineno, checkScope);
              sym = lookup_symbol(symbol_table, $1, checkScope, inside_function_scope);

              if (!sym) {
                  fprintf(stderr, "Error: Failed to insert or lookup symbol '%s' (line %d)\n", $1, yylineno);
                  $$ = newexpr(nil_e);
              } else {
                  $$ = lvalue_expr(sym);
              }
          } else {
              $$ = lvalue_expr(sym);
          }
      }

    | LOCAL IDENTIFIER
      {
          insert_symbol(symbol_table, $2, LOCAL_VAR, yylineno, checkScope);
          SymbolTableEntry *sym = lookup_symbol(symbol_table, $2, checkScope, inside_function_scope);
          $$ = lvalue_expr(sym);
      }
    | COLON_COLON IDENTIFIER
      {
          SymbolTableEntry *sym = lookup_symbol(symbol_table, $2, 0, 0);
          if (!sym)
              fprintf(stderr, "Error: Symbol '%s' not found in global scope (line %d)\n", $2, yylineno);
          $$ = lvalue_expr(sym);
      }
    | member
;

const
    : INTCONST   { $$ = newexpr_constnum($1);         }
    | REALCONST  { $$ = newexpr_constnum($1);         }
    | STRING     { $$ = newexpr_conststring($1);      }
    | NIL        { $$ = newexpr(nil_e);               }
    | TRUE       { $$ = newexpr_constbool(1);         }
    | FALSE      { $$ = newexpr_constbool(0);         }
;

member
    : lvalue DOT IDENTIFIER { 
          expr* result = newexpr(tableitem_e);
          result->sym = newtemp();
          result->index = newexpr_conststring($3);
          emit(tablegetelem, $1, result->index, result, 0, yylineno);
          $$ = result;
          print_rule("member -> lvalue . IDENTIFIER"); 
      }
    | lvalue LEFT_BRACKET expr RIGHT_BRACKET { 
          expr* result = newexpr(tableitem_e);
          result->sym = newtemp();
          result->index = $3;
          emit(tablegetelem, $1, result->index, result, 0, yylineno);
          $$ = result;
          print_rule("member -> lvalue [ expr ]"); 
      }
    | call_member { $$ = $1; print_rule("member -> call_member"); 
      }
    ;

/* Helpful for member */
call_member
    : call DOT IDENTIFIER { 
          expr* result = newexpr(tableitem_e);
          result->sym = newtemp();
          result->index = newexpr_conststring($3);
          emit(tablegetelem, $1, result->index, result, 0, yylineno);
          $$ = result;
          print_rule("call_member -> call . IDENTIFIER"); 
      }
    | call LEFT_BRACKET expr RIGHT_BRACKET { 
          expr* result = newexpr(tableitem_e);
          result->sym = newtemp();
          result->index = $3;
          emit(tablegetelem, $1, result->index, result, 0, yylineno);
          $$ = result;
          print_rule("call_member -> call [ expr ]"); 
      }
    ;

call
    : call LEFT_PARENTHESIS elist RIGHT_PARENTHESIS { 
        $$ = make_call_expr($1, $3); // $1 = previous call expr, $3 = argument list
        print_rule("call -> call (elist)"); 
      }
    | lvalue {
        is_calling = 1;
      } callsuffix { 
        is_calling = 0;
        $$ = make_call_expr($1, $3);
        print_rule("call -> lvalue callsuffix"); 
      }
    | LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS LEFT_PARENTHESIS elist RIGHT_PARENTHESIS { 
        /* NOTE: THIS IS WHERE THE INCOMPATIBILITY OCCURS */
        // expr* fexpr = $2; // funcdef now returns expr*
        // $$ = make_call_expr(current_function_expr, $6);
        $$ = make_call_expr($2, $5);
        print_rule("call -> ( funcdef ) ( elist )"); }     
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
    : expr { 
        // $$ = create_expr_list($1, NULL);	// $1 is expr*
        $$ = $1;              			// $1 is expr*, so $$ should be expr*
        print_rule("elist -> expr"); 
      }
    | expr COMMA elist { 
        $$ = create_expr_list($1, $3);		// returns expr* list
        print_rule("elist -> expr , elist"); 
      }
    | /* empty */ { 
        $$ = NULL;
        print_rule("elist -> epsilon");  	// Allows empty argument lists
      }
    ;

objectdef
    : LEFT_BRACKET elist RIGHT_BRACKET { 
          expr* t = newexpr(newtable_e);
          t->sym = newtemp();
          emit(tablecreate, NULL, NULL, t, 0, yylineno);
          
          // Add elements from elist
          int i = 0;
          expr* curr = $2;
          while(curr) {
              expr* index = newexpr_constnum(i++);
              emit(tablesetelem, curr, index, t, 0, yylineno);
              curr = curr->next;
          }
          
          $$ = t;
          print_rule("objectdef -> [ elist ]"); 
      }
    | LEFT_BRACKET indexed RIGHT_BRACKET { 
          expr* t = newexpr(newtable_e);
          t->sym = newtemp();
          emit(tablecreate, NULL, NULL, t, 0, yylineno);
          
          // Process indexed elements
          expr* curr = $2;
          while(curr) {
              emit(tablesetelem, curr->args, curr->index, t, 0, yylineno);
              curr = curr->next;
          }
          
          $$ = t;
          print_rule("objectdef -> [ indexed ]"); 
      }
    ;

indexed
    : indexedelem { $$ = $1; print_rule("indexed -> indexedelem"); }
    | indexedelem COMMA indexed { 
          // Link the current indexedelem with the rest of the indexed list
          $1->next = $3;
          $$ = $1;
          print_rule("indexed -> indexedelem, indexed"); 
      }
    ;

indexedelem
    : LEFT_BRACE expr COLON expr RIGHT_BRACE { 
          expr* elem = newexpr(var_e);
          elem->sym = newtemp();
          elem->index = $2;
          elem->args = $4;
          elem->next = NULL;
          $$ = elem;
          print_rule("indexedelem -> { expr : expr }"); 
      }
    ;

formal_arguments
    : idlist { $$ = $1; }
    ;

funcdef
  : FUNCTION IDENTIFIER
      {
          SymbolTableEntry *func_sym = insert_symbol(symbol_table, $2, USER_FUNCTION, yylineno, checkScope);
          expr* f = newexpr(programfunc_e);
          f->sym = func_sym;
	  current_function_expr = f;
          //$<expression>3 = e;

          enter_scope();
          ++inside_function_depth;
          inside_function_scope = 1;
          first_brace_of_func = 1;  // Indicates the first brace of the function
      }
      LEFT_PARENTHESIS formal_arguments RIGHT_PARENTHESIS
      {
          // we insert the formal arguments here for normal functions
          formal_argument_node* arg = $5;
          while (arg != NULL) {
              insert_symbol(symbol_table, arg->name, ARGUMENT, yylineno, checkScope);
              arg = arg->next;
          }
      }
      block
      {
          --inside_function_depth;
          exit_scope();
          $$ = $<expression>3;      // use previously stored expr*
      }

| FUNCTION
    {
        char *anonymous_name = malloc(32);
        if (!anonymous_name) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        sprintf(anonymous_name, "$%d", anonymus_function_counter++);
        SymbolTableEntry *func_sym = insert_symbol(symbol_table, anonymous_name, USER_FUNCTION, yylineno, checkScope);

        $<expression>$ = newexpr(programfunc_e);
        $<expression>$->sym = func_sym;

        enter_scope();
        ++inside_function_depth;
        first_brace_of_func = 1;

        free(anonymous_name);
    }
    LEFT_PARENTHESIS formal_arguments RIGHT_PARENTHESIS
    {
        formal_argument_node* arg = $4;
        while (arg != NULL) {
            insert_symbol(symbol_table, arg->name, ARGUMENT, yylineno, checkScope);
            arg = arg->next;
        }
    }
    block
    {
        --inside_function_depth;
        exit_scope();
        $$ = $<expression>2;
        print_rule("funcdef -> function ( idlist ) block");
    }
    ;

idlist
    : IDENTIFIER { 
        print_rule("idlist -> IDENTIFIER"); 
        $$ = create_node($1);
    }
    | IDENTIFIER COMMA idlist { 
        print_rule("idlist -> IDENTIFIER , idlist"); 
        $$ = append_argument($3, $1);
    }
    | /* empty */ { print_rule("idlist -> epsilon");
        $$ = NULL;
    }
    ;

ifstmt
    : ifprefix stmt %prec LOWER_THAN_ELSE
      {
        patchlabel($1, nextquad());
        print_rule("ifstmt -> if ( expr ) stmt");
      }
    | ifprefix stmt elseprefix stmt
      {
        patchlabel($1, $3);
        //patchlabel($2, nextquad()); // $2 is stmt not a quad number so we use $3
	    patchlabel($3, nextquad());
        print_rule("ifstmt -> if ( expr ) stmt else stmt");
      }
    ;

ifprefix
    : IF LEFT_PARENTHESIS expr RIGHT_PARENTHESIS
      {

	    // we ensure the expression is in boolean form
        if ($3->type != boolexpr_e) {
            expr* true_const = newexpr_constbool(1);
            emit(if_eq, $3, true_const, NULL, nextquad() + 2, yylineno);
        } else {
            emit(if_eq, $3, NULL, NULL, nextquad() + 2, yylineno);
        }

	    // we emit jump and record its position for patching
        emit(jump, NULL, NULL, NULL, 0, yylineno);
        $$ = nextquad() - 1;

      }

elseprefix
    : ELSE
      {
        $$ = nextquad();
        emit(jump, NULL, NULL, NULL, 0, yylineno);
      }
    ;

whilestmt
    : WHILE LEFT_PARENTHESIS expr RIGHT_PARENTHESIS { checkLoopDepth++; } stmt { checkLoopDepth--; print_rule("whilestmt -> while ( expr ) stmt"); }
    ;

forstmt
    : FOR
      {
          checkLoopDepth++;
          enter_scope();
      }
      LEFT_PARENTHESIS elist SEMICOLON expr SEMICOLON elist RIGHT_PARENTHESIS
      stmt
      {
          exit_scope();
          checkLoopDepth--;
          print_rule("forstmt -> for ( elist ; expr ; elist ) stmt");
      }
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
  : LEFT_BRACE
    {
        int opened = 0;
        if (first_brace_of_func)
            first_brace_of_func = 0;
        else {
            enter_scope();
            opened = 1;
        }
        $<intValue>$ = opened;
    }
    stmt_list RIGHT_BRACE
    {
        if ( $<intValue>2 )
            exit_scope();
    }
  ;

%%

int yyerror(char* yaccProvidedMessage) {
    fprintf(stderr, "%s: at line %d, before token: %s\n", yaccProvidedMessage, yylineno, yytext);
    fprintf(stderr, "unexpected token: %s with ascii: %d\n", yytext, yytext[0]);
    fprintf(stderr, "INPUT NOT VALID\n");
    return 1;
}
