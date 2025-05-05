/**
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#include <assert.h>
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
    expr* next;             /* Just to make trivial s-lists. */
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
    p->arg1 =   arg1; 
    p->arg2 =   arg2; 
    p->result = result; 
    p->label =  label; 
    p->line =   line;
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


