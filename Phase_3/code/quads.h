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
#include <stdarg.h>
#include "symbol_table.h"

/* NOTE: we may need to add more */

unsigned programVarOffset = 0; 
unsigned functionLocalOffset = 0; 
unsigned formalArgOffset = 0; 
unsigned scopeSpaceCounter = 1;

unsigned total = 0; 
unsigned int currQuad = 0;

#define EXPAND_SIZE 1024 
#define CURR_SIZE (total * sizeof(quad)) 
#define NEW_SIZE (EXPAND_SIZE * sizeof(quad) + CURR_SIZE)

/* NOTE: we may need to add more and change some */

typedef enum iopcode { 
    assign,         add,            sub, 
    mul,            div,            mod, 
    uminus,         and,            or, 
    not,            if_eq,          if_noteq, 
    if_lesseq,      if_geatereq,    if_less, 
    if_greater,     call,           param,
    ret,            getretval,      funcstart, 
    funcend,        tablecreate, 
    tablegetelem,   tablesetelem
} iopcode;

/* NOTE: Expression types. You use only the types you really need for i-code generation, so you may drop some entries. */

typedef enum expr_t {       /* lec 10 slide 17 */
    var_e, 
    tableitem_e,

    programfunc_e, 
    libraryfunc_e, 

    arithexpr_e, 
    boolexpr_e, 
    assignexpr_e, 
    newtable_e, 
    
    costnum_e, 
    constbool_e, 
    conststring_e,
    
    nil_e 
} expr_t;

/* NOTE: For simplicity this is a superset type, but you may hack around with more clever storage (if you like) */

typedef struct expr {        /* lec 10 slide 17 */
    expr_t type; 
    SymbolTableEntry* sym; 
    expr* index; 
    double numConst; 
    char* strConst; 
    unsigned char boolConst; 
    expr* next;              /* Just to make trivial s-lists. */
} expr;

typedef struct quad { 
    iopcode         op;
    expr*           result; 
    expr*           arg1; 
    expr*           arg2; 
    unsigned        label; 
    unsigned        line; 
} quad; 

quad *quads = (quad*) 0; 

void expand (void) {                        /* slide 38 */
    assert (total == currQuad); 
    quad *p = (quad*) malloc (NEW_SIZE); 
    if (quads) { 
        memcpy (p, quads, CURR_SIZE); 
        free (quads); 
    } 
    quads = p; 
    total += EXPAND_SIZE; 
}

void emit (                                 /* slide 38: some functions may need modification */
    iopcode     op, 
    expr*       arg1, 
    expr*       arg2, 
    expr*       result, 
    unsigned    label, 
    unsigned    line 
    ) 
{ 
    if (currQuad == total) {
        expand(); 
    }

    quad *p = quads + currQuad++; 
    p->arg1 = arg1; 
    p->arg2 = arg2; 
    p->result = result; 
    p->label = label; 
    p->line = line;
}

scopespace_t currscopespace (void) { 
    if (scopeSpaceCounter == 1) {
        return programvar; 
    } else if (scopeSpaceCounter % 2 == 0) {
        return formalarg; 
    } else {
        return functionlocal; 
    }
}

/* slide 50 */

unsigned currscopeoffset (void) { 
    switch (currscopespace()) { 
        case programvar:  
            return programVarOffset; 
        case functionlocal: 
            return functionLocalOffset; 
        case formalarg: 
            return formalArgOffset; 
        default: 
            assert(0); 
    }
}

void inccurrscopeoffset (void) { 
    switch (currscopespace()) { 
        case programvar: 
            ++programVarOffset;
            break; 
        case functionlocal: 
            ++functionLocalOffset;    
            break; 
        case formalarg:
            ++formalArgOffset;        
            break; 
        default: 
            assert(0); 
    } 
}

void enterscopespace (void) { 
    ++scopeSpaceCounter; 
}

void exitscopespace (void) {
    assert (scopeSpaceCounter>1); 
    --scopeSpaceCounter; 
}

/**
 * Making an l-value expression out of a symbol is simple, 
 * since the expression inherits the symbol type. Also, 
 * getting information like 'library function' name or the 
 * program function' address (after code generation), is 
 * straightforward through the symbol 'sym' field.
 */

expr* lvalue_expr (SymbolTableEntry *sym) {     /* lec 10 slide 18 */
    assert(sym); 
    expr *e = (expr*) malloc (sizeof(expr)); 
    memset(e, 0, sizeof(expr)); 

    e->next = (expr*) 0; 
    e->sym = sym;

    switch (sym->type) { 
        case var_s:
            e->type = var_e; 
            break;
        case programfunc_s: 
            e->type = programfunc_e; 
            break; 
        case libraryfunc_s: 
            e->type = libraryfunc_e; 
            break; 
        default: 
            assert(0); 
    } 
    return e; 
}

expr *newexpr(expr_t t) {                        /* lec 10 slide 24 */
    expr *e = (expr*) malloc (sizeof(expr));
    memset(e, 0, sizeof(expr));
    e->type = t;
    return e;
}

expr *newexpr_conststring(char *s) {
    expr *e = newexpr(conststring_e);
    e->strConst = strdup(s);
    return e;
}

/*TODO: write this function */
SymbolTableEntry * newtemp() {
    // code ...
    // i think this it the _t0 michalis had
}

expr *emit_iftableitem(expr *e) {
    if (e->type != tableitem_e) {
        return e;
    } else {
        expr *result = newexpr(var_e);
        result->sym = newtemp();
        emit(
            tablegetelem,           // iopcode op
            e,                      // expr* arg1
            e->index,               // expr* arg2
            result,                 // expr* result
            currQuad,               // unsigned label 
            e->sym->line            // unsigned line        /* changed these to match emit arguments */
        );
        return result;
    }
}

expr *newexpr_constnum(double i) {                          /* lec 10 slide 29 */
    expr *e = newexpr(costnum_e);
    e->numConst = i;
    return e;
}

unsigned int istempname(char *s) {                         /* lec 10 slide 37 */
    return *s == '_';
}

unsigned int istempexpr(expr *e) {                         /* lec 10 slide 37 */
    return e->sym && istempname(e->sym->name);
}

void patchlabel(unsigned quadNo, unsigned label) {          /* lec 11 slide 10 may need modification */
    assert(quadNo < currQuad && !quads[quadNo].label);
    quads[quadNo].label = label;
}

expr* newexpr_constbool(unsigned int b) {                   /* lec 11 slide 10 */
    expr *e = newexpr(constbool_e);
    e->boolConst = !!b;                                     /* !! is what? */
    return e;
}

unsigned nextquad(void) {                                   /* lec 11 slide 10 */
    return currQuad; 
}

#endif
