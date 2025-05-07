/**
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

 #ifndef QUADS_H
 #define QUADS_H
 
 #include <assert.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #include <stdarg.h>
 
 #include "symbol_table.h"
 
 extern int yylineno;
 
 /* Offsets for scope spaces */
 extern unsigned programVarOffset;
 extern unsigned functionLocalOffset;
 extern unsigned formalArgOffset;
 extern unsigned scopeSpaceCounter;
 
 extern unsigned total;
 extern unsigned int currQuad;
 
 #define EXPAND_SIZE 1024
 #define CURR_SIZE (total * sizeof(quad))
 #define NEW_SIZE (EXPAND_SIZE * sizeof(quad) + CURR_SIZE)
 
 /* Print function (defined in quads_print_functions.c) */
 void print_quads(FILE *f);
 
 typedef enum iopcode {
     assign, add, sub, mul, div, mod,
     uminus, and, or, not,
     if_eq, if_noteq,
     if_lesseq, if_greatereq, if_less, if_greater,
     jump,
     call, param, ret, getretval,
     funcstart, funcend,
     tablecreate, tablegetelem, tablesetelem, if_geatereq
 } iopcode;
 
 typedef enum expr_t {
     var_e, tableitem_e,
     programfunc_e, libraryfunc_e,
     arithexpr_e, boolexpr_e, assignexpr_e, newtable_e,
     constnum_e, constbool_e, conststring_e,
     nil_e
 } expr_t;
 
 typedef struct expr {
     expr_t type;
     SymbolTableEntry* sym;
     struct expr* index;
     double numConst;
     char* strConst;
     unsigned char boolConst;
     struct expr* next;
 } expr;
 
 typedef struct quad {
     iopcode op;
     expr* result;
     expr* arg1;
     expr* arg2;
     unsigned label;
     unsigned line;
 } quad;
 
 extern quad* quads;
 
 /* Quad management */
 void expand(void);
 void emit(iopcode op, expr* arg1, expr* arg2, expr* result, unsigned label, unsigned line);
 unsigned nextquad(void);
 void patchlabel(unsigned quadNo, unsigned label);
 
 /* Scope and offset management */
 scopespace_t currscopespace(void);
 unsigned currscopeoffset(void);
 void inccurrscopeoffset(void);
 void enterscopespace(void);
 void exitscopespace(void);
 
 /* Expressions and temporaries */
 expr* lvalue_expr(SymbolTableEntry* sym);
 expr* newexpr(expr_t t);
 expr* newexpr_constnum(double i);
 expr* newexpr_conststring(char* s);
 expr* newexpr_constbool(unsigned int b);
 char* newtempname(void);
 SymbolTableEntry* newtemp(void);
 void resettemp(void);
 unsigned int istempname(char* s);
 unsigned int istempexpr(expr* e);
 expr* emit_iftableitem(expr* e);
 
 /* For patching incomplete jumps (e.g. break/continue) */
 int newlist(int quadNo);
 int mergelist(int list1, int list2);
 void patchlist(int list, int label);
 
 /* Statement list structure for break/continue */
 typedef struct stmt_t {
     int breaklist;
     int contlist;
 } stmt_t;
 
 void make_stmt(stmt_t* s);
 
 #endif 