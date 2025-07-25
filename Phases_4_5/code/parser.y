/**
 * HY-340 Project Phases 4 & 5 2024-2025
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
    // int checkLoopDepth = 0;
    int inside_function_scope = 0;
    int inside_function_depth = 0;      // 0 for global, >0 for function scope
    static int first_brace_of_func = 0;
    int is_calling = 0;			        // reducing lvalue for function call 1, normal lvalues 0
    expr* current_function_expr = NULL;
    char* current_function_char = NULL;
    int semantic_errors = 0;
    static unsigned checkFuncDepth = 0;
    unsigned int returnlist = 0;

    expr *call_lhs;


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

    /*
    static void dbg_print_expr(FILE *out, expr *e)
    {
        if (!e) { fputs("<null>", out); return; }

        switch (e->type) {
        case constnum_e:    fprintf(out, "%g",   e->numConst);          break;
        case conststring_e: fprintf(out, "\"%s\"", e->strConst);        break;
        case constbool_e:   fputs(e->boolConst ? "true" : "false", out);break;
        default:
            if (e->sym && e->sym->name) fputs(e->sym->name, out);
            else                       fprintf(out, "<type %d>", e->type);
        }
    }
    */

   /**
    * we use this function to reset loop depth when entering a new function
    */
    void enter_function_scope() {
        checkFuncDepth++;
        while (loopcounter() > 0) {
            pop_loopcounter();
        }
    }

    void exit_function_scope() {
        assert(checkFuncDepth > 0); // Add assertion to catch imbalanced calls
        checkFuncDepth--;
    }

    // Helper function to ensure an expression has a valid symbol
    expr* ensure_expr_has_symbol(expr* e) {
        if (!e) return NULL;
        
        // Skip constant types that don't need symbols
        if (e->type == constnum_e || e->type == conststring_e || e->type == constbool_e) {
            return e;
        }
        
        // Ensure the expression has a symbol
        if (!e->sym) {
            e->sym = newtemp();
        }
        
        return e;
    }

    static expr *incdec_lvalue(expr *lv, int is_inc, int is_prefix)
    {
        expr *one = newexpr_constnum(1);

        if (lv->type == tableitem_e) {
            expr *table = emit_iftableitem(lv->table);
            expr *index = lv->index;
            expr *old  = newexpr(var_e); old ->sym = newtemp();
            emit(tablegetelem, table, index, old, 0, yylineno);

            expr *ret  = NULL;                              /* value to return */

            if (!is_prefix) {
                ret = newexpr(var_e); ret->sym = newtemp();
                emit(assign, old, NULL, ret, 0, yylineno);  /* ret = old      */
            }

            expr *newv = newexpr(var_e); newv->sym = newtemp();
            emit(is_inc ? add : sub, old, one, newv, 0, yylineno);
            emit(tablesetelem, newv, index, table, 0, yylineno);

            return is_prefix ? newv : ret;
        }

        expr *ret = newexpr(var_e); ret->sym = newtemp();

        if (is_prefix) {          
            emit(is_inc ? add : sub, lv, one, lv , 0, yylineno);
            emit(assign, lv , NULL, ret, 0, yylineno);      /* result = new   */
        } else {                          
            emit(assign, lv , NULL, ret, 0, yylineno);      /* result = old   */
            emit(is_inc ? add : sub, lv, one, lv , 0, yylineno);
        }

        return ret;
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
%token <intValue>    INTCONST
%token <realValue>   REALCONST
%token <stringValue> FUNCTION AND OR NOT MODULO PLUS_PLUS MINUS_MINUS EQUAL_EQUAL LESS GREATER
%token <stringValue> DOT_DOT DOT COLON_COLON PUNCTUATION OPERATOR

%type <expression>  funcdef
%type <arglist>     idlist formal_arguments
%type <expression> 	expr term primary const lvalue member assignexpr call elist normcall methodcall callsuffix
%type <intValue>	ifprefix elseprefix ifstmt
%type <expression>  call_member indexed indexedelem objectdef
%type <intValue>    whilestmt
%type <expression> immediately_invoked_func_expr stmt

%type <intValue> MP

%right ASSIGNMENT        /* = has less priority in compare with all the other */
%left OR
%left AND
%nonassoc EQUAL_EQUAL NOT_EQUAL
%nonassoc GREATER_THAN GREATER_EQUAL LESS_THAN LESS_EQUAL
%right NOT
%left  PLUS 
%left MINUS              /* changed to UMINUS for (x-y) - z */
%left MULTIPLY DIVIDE MODULO
%right PLUS_PLUS
%right MINUS_MINUS
%right DOT_DOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%nonassoc UMINUS         /* unary minus operator */

// %expect 2

%start program

%%

MP : /* empty */ { $$ = nextquad(); }

program
    : stmt_list { print_rule("program -> stmt_list"); }
    ;

stmt_list
    : stmt stmt_list { print_rule("stmt_list -> stmt stmt_list"); }
    | /* empty */ { print_rule("stmt_list -> epsilon"); }
    ;

stmt
    : expr SEMICOLON 
        {

        // for a[3];
        if ($1 && $1->type == tableitem_e) {
            emit_iftableitem($1);
        }

	    expr *val = convert_to_value($1);
            if($1->type == constnum_e) { 
                expr *temp = newexpr(var_e); 
                temp->sym = newtemp();
                emit(add, newexpr_constnum($1->numConst), NULL, temp, 0, yylineno); 
            }
            /* 

            NOTE:   turns out this is WRONG because then tests like this one: 
                    [   (function four(four){four=4;});   ]
                    fail with extra assign anonymous
            
            if ($1 && $1->type != nil_e && $1->type != constnum_e && $1->type != conststring_e && $1->type != constbool_e) {
                expr *temp = newexpr(var_e);
                temp->sym = newtemp();
                emit(assign, $1, NULL, temp, 0, yylineno);
            }
            */

            print_rule("stmt -> expr ;");
            $$ = $1;
            resettemp(); 
        }
    | error SEMICOLON { print_rule("stmt -> error ;"); yyerrok; resettemp(); }
    | ifstmt { print_rule("stmt -> ifstmt"); resettemp(); }
    | whilestmt { print_rule("stmt -> whilestmt"); resettemp(); }
    | forstmt { print_rule("stmt -> forstmt"); resettemp(); }
    | returnstmt { print_rule("stmt -> returnstmt"); resettemp(); }
    | break_stmt { print_rule("stmt -> break ;"); resettemp(); }
    | continue_stmt { print_rule("stmt -> continue ;"); resettemp(); }
    | block { print_rule("stmt -> block"); resettemp(); }
    | funcdef { print_rule("stmt -> funcdef"); resettemp(); }
    | error ';' { print_rule("stmt -> error ;"); yyerrok; resettemp(); }
    ;

expr
    : expr PLUS expr
    {
        expr *r = newexpr(arithexpr_e);
        
        if ($1 && $1->sym && istempname($1->sym->name))
            r->sym = $1->sym;
        else if ($3 && $3->sym && istempname($3->sym->name))
            r->sym = $3->sym;
        else
            r->sym = newtemp();

        // emit(add, $1, $3, r, 0, yylineno);
        emit(add, emit_iftableitem($1), emit_iftableitem($3), r, 0, yylineno);
        $$ = r;
    }
    | expr MINUS expr
    {
        expr *r = newexpr(arithexpr_e);

        if ($1 && $1->sym && istempname($1->sym->name))
            r->sym = $1->sym;
        else if ($3 && $3->sym && istempname($3->sym->name))
            r->sym = $3->sym;
        else
            r->sym = newtemp(); 

        emit(sub, emit_iftableitem($1), emit_iftableitem($3), r, 0, yylineno);
        $$ = r;
    }
    | expr MULTIPLY expr
    {
        expr *r = newexpr(arithexpr_e);

        if ($1 && $1->sym && istempname($1->sym->name))
            r->sym = $1->sym;
        else if ($3 && $3->sym && istempname($3->sym->name))
            r->sym = $3->sym;
        else
            r->sym = newtemp();

        emit(mul, emit_iftableitem($1), emit_iftableitem($3), r, 0, yylineno);
        $$ = r;
    }
    | expr DIVIDE expr
    {
        expr *r = newexpr(arithexpr_e);
    
        if ($1 && $1->sym && istempname($1->sym->name))
            r->sym = $1->sym;
        else if ($3 && $3->sym && istempname($3->sym->name))
            r->sym = $3->sym;
        else
            r->sym = newtemp();
    
        // emit(idiv, $1, $3, r, 0, yylineno);
        emit(idiv, emit_iftableitem($1), emit_iftableitem($3), r, 0, yylineno);
        $$ = r;
    }
    | expr MODULO expr
    {
        expr *r = newexpr(arithexpr_e);
        
        if ($1 && $1->sym && istempname($1->sym->name))
            r->sym = $1->sym;
        else if ($3 && $3->sym && istempname($3->sym->name))
            r->sym = $3->sym;
        else
            r->sym = newtemp();

        // emit(mod, $1, $3, r, 0, yylineno);
        emit(mod, emit_iftableitem($1), emit_iftableitem($3), r, 0, yylineno);
        $$ = r;
    }
    | expr GREATER_THAN expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();

        if (!$1) $1 = newexpr(nil_e);
        if (!$3) $3 = newexpr(nil_e);
        // Check if either operand is NULL or nil
        if (!$1 || !$3 || $1->type == nil_e || $3->type == nil_e) {
           
            emit(assign, newexpr_constbool(0), NULL, r, 0, yylineno);
        } else {
            // emit(if_greater, $1, $3, NULL, nextquad()+2, yylineno);
            expr *left = emit_iftableitem($1);
            expr *right = emit_iftableitem($3);
            emit(if_greater, left, right, NULL, nextquad()+2, yylineno);

            emit(jump, NULL, NULL, NULL, nextquad()+1, yylineno);
        }
        $$ = r;
    }
    | expr LESS_THAN expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();
        if (!$1) $1 = newexpr(nil_e);
        if (!$3) $3 = newexpr(nil_e);
        if (!$1 || !$3 || $1->type == nil_e || $3->type == nil_e) {
            emit(assign, newexpr_constbool(0), NULL, r, 0, yylineno);
        } else {
            // emit(if_less, $1, $3, NULL, nextquad()+2, yylineno);
            expr *left = emit_iftableitem($1);
            expr *right = emit_iftableitem($3);
            emit(if_less, left, right, NULL, nextquad()+2, yylineno);
            
            emit(jump, NULL, NULL, NULL, nextquad()+1, yylineno);
        }
        $$ = r;
    }
    | expr GREATER_EQUAL expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();
        if (!$1) $1 = newexpr(nil_e);
        if (!$3) $3 = newexpr(nil_e);
        if (!$1 || !$3 || $1->type == nil_e || $3->type == nil_e) {
            emit(assign, newexpr_constbool(0), NULL, r, 0, yylineno);
        } else {
            // emit(if_greatereq, $1, $3, NULL, nextquad()+2, yylineno);
            expr *left = emit_iftableitem($1);
            expr *right = emit_iftableitem($3);
            emit(if_greatereq, left, right, NULL, nextquad()+2, yylineno);

            emit(jump, NULL, NULL, NULL, nextquad()+1, yylineno);
        }
        $$ = r;
    }
    | expr LESS_EQUAL expr
    {
        expr *r = newexpr(boolexpr_e);
        r->sym = newtemp();
        if (!$1) $1 = newexpr(nil_e);
        if (!$3) $3 = newexpr(nil_e);
        if (!$1 || !$3 || $1->type == nil_e || $3->type == nil_e) {
            emit(assign, newexpr_constbool(0), NULL, r, 0, yylineno);
        } else {
            // emit(if_lesseq, $1, $3, NULL, nextquad()+2, yylineno);
            expr *left = emit_iftableitem($1);
            expr *right = emit_iftableitem($3);
            emit(if_lesseq, left, right, NULL, nextquad()+2, yylineno);

            emit(jump, NULL, NULL, NULL, nextquad()+1, yylineno);
        }
        $$ = r;
    }
    | expr EQUAL_EQUAL expr { $$ = make_eq_neq($1, $3, if_eq); }
    | expr NOT_EQUAL expr { $$ = make_eq_neq($1, $3, if_noteq);  }
    | expr OR expr { $$ = make_or($1, $3); }
    | expr AND expr { $$ = make_and($1, $3); }
    | assignexpr { $$ = $1; }
    | immediately_invoked_func_expr { $$ = $1; }
    | term       { $$ = $1; } 
    //| expr DOT_DOT expr { print_rule("expr DOT_DOT expr"); $$ = newexpr(nil_e);}
    ;


assignexpr
    : lvalue ASSIGNMENT expr
    {
        expr *rhs = convert_to_value($3);

        if($1->type == programfunc_e || $1->type == libraryfunc_e) {
            fprintf(stderr,"Error: Symbol '%s' is not a valid l-value (line %d)\n",
                    $1->sym->name, yylineno);
        }

	rhs = emit_iftableitem(rhs);

        if ($1->type == tableitem_e) {
            expr *tbl = emit_iftableitem($1->table);
            expr *idx = $1->index;

            emit(tablesetelem, rhs, idx, tbl, 0, yylineno); // changed order

            expr *result = newexpr(var_e);
            result->sym = newtemp();
            
            emit(tablegetelem, tbl, idx, result, 0, yylineno);
            
            $$ = result;
        } else {
            // emit: a := rhs
            // regular var assing
            emit(assign, rhs, NULL, $1, 0, yylineno);

            // create a temp to hold the result
            expr *final = newexpr(var_e);
            final->sym = newtemp();
            emit(assign, $1, NULL, final, 0, yylineno);
            $$ = final;
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
    : LEFT_PARENTHESIS expr RIGHT_PARENTHESIS { $$ = $2; print_rule("term -> ( expr )"); }
    | MINUS expr %prec UMINUS 
    { 
        expr *r = newexpr(arithexpr_e); 
        if ($2 && $2->sym && istempname($2->sym->name)) {
            r->sym = $2->sym;
        } else {
            r->sym = newtemp();
        }
        printf("%d %s\n", $2->type, $2->sym ? $2->sym->name : "NULL");
        emit(uminus, $2, NULL, r, 0, yylineno); 
        $$ = r; 
        print_rule("term -> - expr"); 
    }
    | NOT expr {
        /* build a proper boolexpr node right here */
        $$ = make_not($2);          /* ← only one call does everything */
    }
    | PLUS_PLUS lvalue 
    {
        $$ = incdec_lvalue($2, 1, 1);
    }
    | lvalue PLUS_PLUS 
    {
        $$ = incdec_lvalue($1, 1, 0); 
    }
    | MINUS_MINUS lvalue 
    {
        $$ = incdec_lvalue($2, 0, 1);
    }
    | lvalue MINUS_MINUS 
    {
        $$ = incdec_lvalue($1, 0, 0); 
    }
    | primary 
    { 
        print_rule("term -> primary"); 
        $$=$1;
    }
    ;

primary
    : lvalue { print_rule("primary -> lvalue"); $$ = emit_iftableitem($1); }
    | call { print_rule("primary -> call"); $$ = emit_iftableitem($1); }
    | objectdef { print_rule("primary -> objectdef"); $$ = $1; }
    | LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS { print_rule("primary -> ( funcdef )"); $$ = $2;}
    | const { print_rule("primary -> const"); $$ = $1; }
    ;

lvalue
    : IDENTIFIER
      {
        SymbolTableEntry *sym = lookup_symbol(symbol_table, $1, checkScope, inside_function_scope);
        if (!sym) {
            sym = insert_symbol(symbol_table, $1, (checkScope==0)? GLOBAL : LOCAL_VAR, yylineno, checkScope);
            if (!sym) {
                fprintf(stderr, "Error: Failed to insert symbol '%s' (line %d)\n", $1, yylineno);
                semantic_errors++;
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
        SymbolTableEntry *sym = insert_symbol(symbol_table, $2, LOCAL_VAR, yylineno, checkScope);
        if (!sym) {
            fprintf(stderr, "Error: Failed to insert symbol '%s' (line %d)\n", $2, yylineno);
            semantic_errors++;
            $$ = newexpr(nil_e);
        } else {
            $$ = lvalue_expr(sym);
        }
    }
    | COLON_COLON IDENTIFIER
    {
        SymbolTableEntry *sym = lookup_symbol(symbol_table, $2, 0, 0);
        if (!sym) {
            fprintf(stderr, "Error: Symbol '%s' not found in global scope (line %d)\n", $2, yylineno);
            semantic_errors++;
            $$ = newexpr(nil_e); // Return a safe nil expression instead of using NULL
        } else {
            $$ = lvalue_expr(sym);
        }
    }
    | member { print_rule("lvalue -> member"); }
    ;

const
    : INTCONST    { $$ = newexpr_constnum($1);        }
    | REALCONST   { $$ = newexpr_constnum($1);        }
    | STRING      { $$ = newexpr_conststring($1);     }
    | NIL         { $$ = newexpr(nil_e);              }
    | TRUE       { $$ = newexpr_constbool(1);         }
    | FALSE      { $$ = newexpr_constbool(0);         }
;

member
    : lvalue DOT IDENTIFIER 
    { 
        expr* result = newexpr(tableitem_e);
        result->sym = newtemp();
        result->index = newexpr_conststring($3);
        result->table = $1;
        $$ = result;
        print_rule("member -> lvalue . IDENTIFIER"); 
    }
    | lvalue LEFT_BRACKET expr RIGHT_BRACKET 
    { 
        expr* result = newexpr(tableitem_e);
        result->sym = newtemp();
        result->index = $3;
        result->table = $1;
        //emit(tablegetelem, $1, result->index, result, 0, yylineno);
        $$ = result;
        print_rule("member -> lvalue [ expr ]"); 
    }
    | call_member { $$ = $1; print_rule("member -> call_member"); }
    ;

/* Helpful for member */
call_member
    : call DOT IDENTIFIER 
    { 
        expr* result = newexpr(tableitem_e);
        result->sym = newtemp();
        result->index = newexpr_conststring($3);
        result->table = $1;
        $$ = result;
        print_rule("call_member -> call . IDENTIFIER"); 
    }
    | call LEFT_BRACKET expr RIGHT_BRACKET 
    { 
        expr* result = newexpr(tableitem_e);
        result->sym = newtemp();
        result->index = $3;
        result->table = $1;
        $$ = result;
        print_rule("call_member -> call [ expr ]"); 
    }
    ;

call
  : lvalue
    {
      call_lhs = $1;
    }
    callsuffix
      {
        /* now callsuffix can safely use call_lhs */
        $$ = $3;
      }
  | call LEFT_PARENTHESIS elist RIGHT_PARENTHESIS
      {
        /* chaining: f(...)(...) */
        $$ = make_call_expr($1, $3);
        print_rule("call -> call ( elist )");
      }
  ;


immediately_invoked_func_expr
    : LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS LEFT_PARENTHESIS elist RIGHT_PARENTHESIS {
        // Add safety check for anonymous function calls
        if (!$2) {
            debug(1, "Warning: Invalid function definition at line %d\n", yylineno);
            $$ = newexpr(nil_e); // Return a safe nil expression
        } else {

            // ensure funcdef has a symbol
            if (!$2->sym) {
                $2->sym = newtemp();
            }

            $$ = make_call_expr($2, $5);
        }
        print_rule("call -> ( funcdef ) ( elist )"); 
    }
;

callsuffix
  : normcall
      {
        $$ = $1;
        print_rule("callsuffix -> ( elist )");
      }
  | methodcall
      {
        $$ = $1;
        print_rule("callsuffix -> .. IDENTIFIER ( elist )");
      }
  ;

/* normal call:  ( elist ) */
normcall
  : LEFT_PARENTHESIS elist RIGHT_PARENTHESIS
      {
        $$ = make_call_expr(call_lhs, $2);
      }
  ;

/* method call: ..IDENTIFIER(elist) */
methodcall
  : DOT_DOT IDENTIFIER LEFT_PARENTHESIS elist RIGHT_PARENTHESIS
      {
        /* look up the method in the table */
        expr *mname   = newexpr_conststring($2);
        expr *method  = newexpr(tableitem_e);
        method->sym   = newtemp();
        emit(tablegetelem, call_lhs, mname, method, 0, yylineno);

        /* prepend self and call */
        expr *full_args = create_expr_list(call_lhs, $4);
        $$ = make_call_expr(method, full_args);
      }
  ;

elist
    : expr
      {
          if ($1 && !$1->sym &&
              $1->type != constnum_e &&
              $1->type != conststring_e &&
              $1->type != constbool_e)
              $1->sym = newtemp();

          $$ = $1;
          print_rule("elist -> expr");
      }
    | expr COMMA elist
      {
          if ($1 && !$1->sym &&
              $1->type != constnum_e &&
              $1->type != conststring_e &&
              $1->type != constbool_e)
              $1->sym = newtemp();

          $$ = create_expr_list($1, $3);
          print_rule("elist -> expr , elist");
      }
    | /* empty */
      {
          $$ = NULL;
          print_rule("elist -> epsilon");
      }
;

objectdef
    : LEFT_BRACKET elist RIGHT_BRACKET 
    { 
        expr* t = newexpr(newtable_e);
        t->sym = newtemp();
        emit(tablecreate, NULL, NULL, t, 0, yylineno);
        
        int i = 0;
        expr* curr = $2;
        while(curr) {
            if (!curr->sym) curr->sym = newtemp();

            expr* index = newexpr_constnum(i++);
            emit(tablesetelem, curr, index, t, 0, yylineno);
            curr = curr->next;
        }
        
        $$ = t;
        print_rule("objectdef -> [ elist ]"); 
    }
    | LEFT_BRACKET indexed RIGHT_BRACKET 
    { 
        expr* t = newexpr(newtable_e);
        t->sym = newtemp();
        emit(tablecreate, NULL, NULL, t, 0, yylineno);

        expr* curr = $2;
        while(curr) {
            if (!curr->args) {
                fprintf(stderr, "ERROR: indexedelem->args is NULL at line %d\n", yylineno);
                curr = curr->next;
                continue;
            }
            if (!curr->index) {
                fprintf(stderr, "ERROR: indexedelem->index is NULL at line %d\n", yylineno);
                curr = curr->next;
                continue;
            }
            if (!curr->args->sym) curr->args->sym = newtemp();
            if (!curr->index->sym && curr->index->type != constnum_e && curr->index->type != conststring_e) curr->index->sym = newtemp();

            emit(tablesetelem, curr->args, curr->index, t, 0, yylineno);
            curr = curr->next;
        }
        
        $$ = t;
        print_rule("objectdef -> [ indexed ]"); 
    }
;

indexed
    : indexedelem { $$ = $1; print_rule("indexed -> indexedelem"); }
    | indexedelem COMMA indexed 
    {
        $1->next = $3;
        $$ = $1;
        print_rule("indexed -> indexedelem, indexed"); 
    }
    ;

indexedelem
    : LEFT_BRACE expr COLON expr RIGHT_BRACE 
    { 
        expr* elem = newexpr(var_e);
        elem->sym = newtemp();
        elem->index = $2;
        elem->args = $4;
        elem->next = NULL;

        if (elem->index && !elem->index->sym &&
            elem->index->type != constnum_e &&
            elem->index->type != conststring_e) {
            elem->index->sym = newtemp();
        }
        if (elem->args && 
        elem->args->type != constnum_e &&
        elem->args->type != conststring_e &&
        elem->args->type != constbool_e &&
        !elem->args->sym) { elem->args->sym = newtemp();
        }
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

        emit(jump, NULL, NULL, NULL, nextquad() + 2, yylineno);
        emit(funcstart, f, NULL, NULL, 0, yylineno);

	    returnlist = 0;

        enter_scope();
        ++inside_function_depth;
        inside_function_scope = 1;
        first_brace_of_func = 1;  // Indicates the first brace of the function
	    enter_function_scope();   // for loop

        $<expression>$ = f;
    }
    LEFT_PARENTHESIS formal_arguments RIGHT_PARENTHESIS
    {
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
        exit_function_scope();      // for loop
        emit(funcend, $<expression>3, NULL, NULL, 0, yylineno); // emit funcend
        patchlist(returnlist, nextquad());

        $$ = $<expression>3;        // use previously stored expr*
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
        free(anonymous_name);
        expr* func_expr = newexpr(programfunc_e);
        func_expr->sym = func_sym;
        emit(jump, NULL, NULL, NULL, nextquad() + 2, yylineno);
        emit(funcstart, func_expr, NULL, NULL, 0, yylineno);
        returnlist = 0;
        current_function_expr = func_expr;  // we save this function as current
        enter_scope();
        ++inside_function_depth;
        first_brace_of_func = 1;
        enter_function_scope();             // for loop
        $<expression>$ = func_expr;         // and then pass it to later rules

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
	    exit_function_scope();  // for loop
        exit_scope();

        emit(funcend, $<expression>2, NULL, NULL, 0, yylineno);
        patchlist(returnlist, nextquad());

        $$ = $<expression>2;    // rtrn func_expr
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
        patchlabel($1, $3 + 1);
        patchlabel($3, nextquad());
        print_rule("ifstmt -> if ( expr ) stmt else stmt");
        }
    ;

ifprefix
    : IF LEFT_PARENTHESIS expr RIGHT_PARENTHESIS
    {
        // Ensure expr has a symbol
        $3 = ensure_expr_has_symbol($3);

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
    : WHILE MP
      LEFT_PARENTHESIS expr RIGHT_PARENTHESIS
      {
          push_loopcounter();

          expr *cond = $4;
          if (cond->type == boolexpr_e)        /* a chain of or/and/not           */
            cond = convert_to_value(cond);     /* generate t := true/false      */
          else
            cond = emit_iftableitem(cond);     /* still handle tableitem         */

          /* IF cond == TRUE jump somewhere (patch later) */
          emit(if_eq, cond, newexpr_constbool(1), NULL, 0, yylineno);
          emit(jump , NULL, NULL, NULL,0, yylineno);  /* JFALSE */

          $<intValue>$ = nextquad() - 2; /* $6 : IF-quad id */
      }
      MP                                         
      stmt
      {
          //printf("EMIT-LABEL-2: %d\n", $2);
          emit(jump, NULL, NULL, NULL, $2, yylineno);      /* back-edge  */

          /* patch IF-TRUE  → body, IF-FALSE → exit */
          patchlabel($<intValue>6    , $7);
          patchlabel($<intValue>6 + 1, nextquad());

          lc_stack_t *loop = current_loop();
          if (loop) {
              patchlist(loop->breaklist, nextquad());  /* break    → exit */
              patchlist(loop->contlist, $2);           /* continue */
          }

          pop_loopcounter();
      }
    ;

forstmt
    : FOR
        { push_loopcounter(); enter_scope(); } 
      LEFT_PARENTHESIS
      elist SEMICOLON                      
      MP
      expr                                    
        {
            // Ensure expr has a symbol
            $7 = ensure_expr_has_symbol($7);

            emit(if_eq, $7, newexpr_constbool(1), NULL, nextquad() + 1, yylineno);
            emit(jump , NULL, NULL, NULL, nextquad() + 3, yylineno);  /* JFALSE */

            $<intValue>$ = nextquad() - 2; /* $8 : IF-quad id */
        }
      SEMICOLON
      MP                                       
      elist                                      
        {
            emit(jump, NULL, NULL, NULL, $6, yylineno);   
        }
      RIGHT_PARENTHESIS
      MP                                          
      stmt
            {
        int ifQuad = $<intValue>8;

        emit(jump, NULL, NULL, NULL, $10, yylineno);

        patchlabel(ifQuad, $14);

        patchlabel(ifQuad + 1, nextquad());

        lc_stack_t *loop = current_loop();
        if (loop) {
            // 4. Patch any 'break' statements to also exit the loop.
            patchlist(loop->breaklist, nextquad());
            // 5. Patch any 'continue' statements to go to the increment part ($10).
            patchlist(loop->contlist, $10);
        }
        exit_scope();
        pop_loopcounter();
    }
    ;

returnstmt
    : RETURN SEMICOLON
    {
        if (inside_function_depth < 1) {
            fprintf(stderr, "Error: 'return' used outside of any function (line %d)\n", yylineno);
            semantic_errors++;
        }
        print_rule("returnstmt -> return ;");
    }
    | RETURN expr SEMICOLON
    {
        if (inside_function_depth < 1) {
            fprintf(stderr, "Error: 'return' used outside of any function (line %d)\n", yylineno);
            semantic_errors++;
        }
        expr *val = convert_to_value($2);
        emit(ret, val, NULL, NULL, 0, yylineno);

        emit(jump, NULL, NULL, NULL, 0, yylineno);  // label to patch later
        returnlist = mergelist(returnlist, newlist(nextquad() - 1));

        print_rule("returnstmt -> return expr ;");
    }
    ;

break_stmt
    : BREAK SEMICOLON 
    {
        //if (checkLoopDepth < 1) { 
        if (loopcounter() == 0) {
            fprintf(stderr, "Error: 'break' used outside of any loop (line %d)\n", yylineno);
            semantic_errors++;
        } 
        print_rule("break_stmt -> break ;"); 
    }
    {
        if (loopcounter() == 0) {
            // Skip patching if not in a loop
            fprintf(stderr, "Error: 'break' used outside of any loop (line %d)\n", yylineno);
            semantic_errors++;
        } else {
            emit(jump, NULL, NULL, NULL, 0, yylineno); // emit jump to break
            struct lc_stack_t *n = current_loop();
            if (n) { // Add safety check
                n->breaklist = mergelist(n->breaklist, newlist(nextquad() - 1));
            }
        }
    }
    ;

continue_stmt
    : CONTINUE SEMICOLON 
    {
        //if (checkLoopDepth < 1) { 
        if (loopcounter() == 0) {
            fprintf(stderr, "Error: 'continue' used outside of any loop (line %d)\n", yylineno);
            semantic_errors++;
        } 
        print_rule("continue_stmt -> continue ;"); 
    }
    {
        if (loopcounter() == 0) {
            // Skip patching if not in a loop
            fprintf(stderr, "Error: 'continue' used outside of any loop (line %d)\n", yylineno);
            semantic_errors++;
        } else {
            emit(jump, NULL, NULL, NULL, 0, yylineno); // emit jump to break
            struct lc_stack_t *n = current_loop();
            if (n) { // Add safety check
                n->contlist = mergelist(n->contlist, newlist(nextquad() - 1));
            }
        }
    }
    ;

block
    : LEFT_BRACE { enter_scope(); } stmt_list RIGHT_BRACE { exit_scope(); print_rule("block -> { stmt_list }"); }
    ;

%%

int yyerror (char* yaccProvidedMessage)
{
    fprintf(stderr, "%s: at line %d, before token: '%s'\n", yaccProvidedMessage, yylineno, yytext);
    fprintf(stderr, "unexpected token: %s with ascii: %d\n", yytext, yytext[0]);
    fprintf(stderr, "INPUT NOT VALID\n");
    return 1;
}