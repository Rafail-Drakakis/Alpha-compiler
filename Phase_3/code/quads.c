/**
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h> // uintptr_t
#include "quads.h"
#include "symbol_table.h"

unsigned programVarOffset = 0;
unsigned functionLocalOffset = 0;
unsigned formalArgOffset = 0;
unsigned scopeSpaceCounter = 1;
unsigned total = 0;
unsigned int currQuad = 0;
static unsigned tempcounter = 0;

extern unsigned int checkScope;
extern int yyparse();
extern FILE *yyin;

struct lc_stack_t {
   struct lc_stack_t* next;
   unsigned counter;
};

static struct lc_stack_t *lcs_top = 0;
static struct lc_stack_t *lcs_bottom = 0;

static unsigned loop_id_counter = 1;

unsigned loopcounter(void) {
    if (lcs_top != NULL) {
        return lcs_top->counter;
    } else {
        return 0;
    }
}

/**
 * we made push_loopcounter() and 
 * pop_loopcounter() scope-aware and 
 * we reset when entering/exiting functions
 * in enter_function_scope()
 */

void push_loopcounter(void) {
    struct lc_stack_t* new_node = malloc(sizeof(struct lc_stack_t));
    if (!new_node) {
        debug(1, "Memory allocation failed in push_loopcounter\n");
        exit(EXIT_FAILURE);
    }
    new_node->counter = loop_id_counter++;
    new_node->next = lcs_top;
    lcs_top = new_node;
}

void pop_loopcounter(void) {
    if (!lcs_top) return;
    struct lc_stack_t* temp = lcs_top;
    lcs_top = lcs_top->next;
    free(temp);
}

unsigned int currscope(void) {
    return checkScope;
}

void expand(void) {
    assert(total == currQuad);
    quad *p = NULL;
    
    if (!quads) {
        p = (quad *)malloc(NEW_SIZE);
        if (!p) {
            debug(1, "memory allocation failed in expand\n");
            exit(EXIT_FAILURE);
        }
        total = EXPAND_SIZE;
    }
    else {
        p = (quad *)malloc(NEW_SIZE);
        if (!p) {
            debug(1, "memory allocation failed in expand\n");
            exit(EXIT_FAILURE);
        }
        memcpy(p, quads, CURR_SIZE);
        free(quads);
        total += EXPAND_SIZE;
    }
    quads = p;
}

static const char *op_to_str(iopcode op) {
    static const char *name[] = {
        "assign", "add", "sub", "mul", "idiv", "mod",
        "uminus", "and", "or", "not",
        "if_eq", "if_noteq", "if_lesseq", "if_greatereq", "if_less", "if_greater",
        "jump", "call", "param", "ret", "getretval",
        "funcstart", "funcend", "tablecreate", "tablegetelem", "tablesetelem"
    };

    return name[op];
}

static const char *expr_to_str_buf(expr *e, char *buf, size_t bufsize) {
    if (!e) {
        snprintf(buf, bufsize, "nil");
        return buf;
    }

    /* 1. Types that do not depend on sym */
    switch (e->type) {
        case constnum_e:
            snprintf(buf, bufsize, "%.2f", e->numConst);
            return buf;
        case conststring_e:
            snprintf(buf, bufsize, "\"%s\"", e->strConst);
            return buf;
        case constbool_e:
            snprintf(buf, bufsize, "%s", e->boolConst ? "TRUE" : "FALSE");
            return buf;
        case nil_e:
            snprintf(buf, bufsize, "NIL");
            return buf;
        default:
            break;
    }
    
    if (!e->sym) {
        snprintf(buf, bufsize, "anonymous");
        return buf;
    }

    switch (e->type) {
    case var_e:
    case tableitem_e:
    case arithexpr_e:
    case assignexpr_e:
        snprintf(buf, bufsize, "%s", e->sym->name);
        break;
    case programfunc_e:
    case libraryfunc_e:
        snprintf(buf, bufsize, "%s()", e->sym->name);
        break;
    case newtable_e:
        snprintf(buf, bufsize, "[table]");
        break;
    default:
        snprintf(buf, bufsize, "UNKNOWN");
        break;
    }
    return buf;
}

void emit(iopcode op, expr *arg1, expr *arg2, expr *result, unsigned label, unsigned line) {
    // Debug output for critical quads
    if (currQuad >= 48 && currQuad <= 55) {
        debug(1, "About to emit quad %d - op: %s\n", currQuad+1, op_to_str(op));
        if (arg1) debug(1, "  arg1 type: %d, addr: %p\n", arg1->type, (void*)arg1);
        if (result) {
            debug(1, "  result type: %d, addr: %p\n", result->type, (void*)result);
            if (result->sym) debug(1, "  result sym: %s\n", result->sym->name);
        }
    }
    
    if (arg1 && !arg1->sym &&
        (arg1->type != constnum_e &&
        arg1->type != conststring_e &&
        arg1->type != constbool_e &&
        arg1->type != newtable_e)) {
        debug(1, "Warning: Expression without symbol (type %d) at line %d\n", arg1->type, line);
        arg1->sym = newtemp();
    }

    if (arg2 && !arg2->sym &&
        (arg2->type != constnum_e &&
        arg2->type != conststring_e &&
        arg2->type != constbool_e &&
        arg2->type != newtable_e)) { 
        debug(1, "Warning: Expression without symbol (type %d) at line %d\n", arg2->type, line);
        arg2->sym = newtemp();
    }

    if (result && !result->sym) {
        // let's only warn if it's not an arithmetic or assign expression since temp results are expected here
        if (result->type != arithexpr_e 
            && result->type != assignexpr_e 
            && result->type != var_e 
            && result->type != boolexpr_e
            && result->type != tableitem_e && result->type != newtable_e) {
                    debug(1, "Warning: Result without symbol (type %d) at line %d\n", result->type, line);
        }
    result->sym = newtemp();
    }
    
    // Special handling for boolean expressions in assign operations
    if (op == assign && arg1 && arg1->type == boolexpr_e) {
        // If the boolean expression doesn't have a symbol, create a temporary one
        if (!arg1->sym) {
            debug(1, "Warning: Boolean expression without symbol in assign operation (line %d)\n", line);
            
            // Create a new temporary expression with a symbol
            expr *temp = newexpr(var_e);
            temp->sym = newtemp();
            
            // Create a boolean constant to assign to the temporary
            expr *boolval = newexpr_constbool(0);
            
            // Emit a separate quad for this assignment
            if (currQuad < total) {
                quad *q = quads + currQuad++;
                q->op = assign;
                q->arg1 = boolval;
                q->arg2 = NULL;
                q->result = temp;
                q->label = 0;
                q->line = line;
            }
            
            // Use this temporary instead of the original arg1
            arg1 = temp;
        }
    }
    
    // Safety check for NULL or nil expressions in critical operations
    if ((op == if_eq || op == if_noteq || op == if_lesseq || op == if_greatereq || 
         op == if_less || op == if_greater) && 
        (arg1 == NULL || arg2 == NULL || 
         (arg1 && arg1->type == nil_e) || 
         (arg2 && arg2->type == nil_e))) {
        debug(1, "Warning: Skipping unsafe boolean operation at line %d\n", line);
        return; // Skip this quad entirely
    }

    // Check for required arguments based on opcode
    switch (op) {
        // Operations requiring result
        case assign:
        case add:
        case sub:
        case mul:
        case idiv:
        case mod:
        case uminus:
        case and:
        case or:
        case not:
        case getretval:
        case tablecreate:
        case tablegetelem:
            if (result == NULL) {
                debug(1, "Error: NULL result in emit() for opcode that requires result (line %d)\n", line);
                return;
            }
            break;
        // Operations requiring arg1
        case if_eq:
        case if_noteq:
        case if_lesseq:
        case if_greatereq:
        case if_less:
        case if_greater:
        case param:
        case call:
            if (arg1 == NULL) {
                debug(1, "Error: NULL arg1 in emit() for opcode that requires arg1 (line %d)\n", line);
                return;
            }
            break;
        // Operations requiring both arg1 and arg2
        case tablesetelem:
            if (arg1 == NULL || arg2 == NULL) {
                fprintf(stderr, "Error: NULL arg1 or arg2 in emit() for opcode that requires both (line %d)\n", line);
                return;
            }
            break;
        default:
            break;
    }

    // ensure we have space for the new quad
    if (currQuad == total)
        expand();

    // create the new quad
    quad *q = quads + currQuad++;
    q->op = op;
    if (op == tablesetelem) {
        q->arg1 = arg2;  // index
        q->arg2 = arg1;  // value
    } else {
        q->arg1 = arg1;
        q->arg2 = arg2;
    }
    q->result = result;
    q->label = label;
    q->line = line;
}

unsigned nextquad(void) {
    return currQuad;
}

void patchlabel(unsigned quadNo, unsigned label) {
    // Add comprehensive safety checks
    if (quadNo >= currQuad) {
        fprintf(stderr, "Error: patchlabel: quadNo (%u) >= currQuad (%u) at line %d\n", quadNo, currQuad, yylineno);
        return;
    }
    
    // Make sure quads array is valid
    if (!quads) {
        fprintf(stderr, "Error: quads array is NULL in patchlabel\n");
        return;
    }
    
    debug(1, "Patching quad %u with label %u\n", quadNo, label);
    quads[quadNo].label = label;
}

/* Scoping */

scopespace_t currscopespace(void) {
    if (scopeSpaceCounter == 1) {
        return programvar;
    } else if (scopeSpaceCounter % 2 == 0) {
        return formalarg;
    } else {
        return functionlocal;
    }
}

unsigned currscopeoffset(void) {
    switch (currscopespace())
    {
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

void inccurrscopeoffset(void) {
    switch (currscopespace())
    {
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

void enterscopespace(void) {
    ++scopeSpaceCounter;
}

void exitscopespace(void) {
    assert(scopeSpaceCounter > 1);
    --scopeSpaceCounter;
}

/* Expressions */

expr *newexpr(expr_t t) {
    expr *e = (expr *)malloc(sizeof(expr));
    if (!e) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
    memset(e, 0, sizeof(expr));
    e->type = t;
    e->sym = NULL;
    
    // for nil expressions, ensure they have safe default values
    if (t == nil_e) {
        e->sym = newtemp();
    }
    // FOR newtable_e, ALWAYS ASSIGN A SYMBOL!
    if (t == newtable_e) {
        e->sym = newtemp();
    }
    return e;
}

expr *newexpr_constnum(double i) {
    expr *e = newexpr(constnum_e);
    e->numConst = i;
    return e;
}

expr* newexpr_conststring(char* s) {
    if (!s || ((uintptr_t)s) < 0x1000) { // Super-low address = likely error
        fprintf(stderr, "BUG: newexpr_conststring called with bad pointer %p\n", s);
        exit(1); // or exit(1)
    }
    expr* e = newexpr(conststring_e);
    e->strConst = strdup(s);
    return e;
}


expr *newexpr_constbool(unsigned int b) {
    expr *e = newexpr(constbool_e);
    e->boolConst = !!b;
    // make sure it has a symbol to prevent issues
    if (!e->sym) {
        e->sym = newtemp();
    }
    return e;
}

char *newtempname(void) {
    char *name = malloc(16);
    sprintf(name, "_%u", tempcounter++);
    return name;
}

SymbolTableEntry *newtemp(void) {
    char *name = newtempname();
    SymbolTableEntry *sym = lookup_symbol(symbol_table, name, currscope(), 0);
    if (!sym)
    {
        insert_symbol(symbol_table, name, TEMP_VAR, yylineno, currscope());
        sym = lookup_symbol(symbol_table, name, currscope(), 0);
    }
    return sym;
}

void resettemp(void) {
    tempcounter = 0;
}

unsigned int istempname(char *s) {
    return *s == '_';
}

unsigned int istempexpr(expr *e) {
    return e && e->sym && istempname(e->sym->name);
}

expr *lvalue_expr(SymbolTableEntry *sym) {
    if (!sym) {
        fprintf(stderr, "FATAL: NULL symbol passed to lvalue_expr (likely undeclared variable) at line %d\n", yylineno);
        assert(0);
    }
    expr *e = newexpr(var_e);
    e->sym = sym;

    switch (sym->type) {
        case GLOBAL:
        case LOCAL_VAR:
        case ARGUMENT:
        case TEMP_VAR:
            e->type = var_e;
            break;
        case USER_FUNCTION:
            e->type = programfunc_e;
            break;
        case LIBRARY_FUNCTION:
            e->type = libraryfunc_e;
            break;
        default:
            fprintf(stderr, "Error: Unknown symbol type %d for symbol '%s' at line %d\n", sym->type, sym->name, sym->line_number);
            assert(0); // for debugging
    }

    return e;
}

/* added make_call_expr & create_expr_list (not sure if they can be replaced by existing functions) */

expr *make_call_expr(expr *func_expr, expr *args) {
    // Defensive check for NULL function expression
    if (!func_expr) {
        fprintf(stderr, "Warning: NULL function expression in make_call_expr\n");
        return newexpr(nil_e);
    }
    
    // Create a new call expression
    expr *call_expr = newexpr(call_e);
    
    // Safely copy the symbol if available
    if (func_expr->sym) {
        call_expr->sym = func_expr->sym;
    } else {
        // Create a temporary symbol if none exists
        call_expr->sym = newtemp();
        fprintf(stderr, "Warning: Function expression has no symbol, created temp\n");
    }
    
    // Safely handle arguments
    call_expr->args = args;
    
    return call_expr;
}

expr *create_expr_list(expr *head, expr *tail) {
    if (!head) {
        return tail;
    }
    head->next = tail;
    return head;
}

expr *emit_iftableitem(expr *e) {
    if (!e) {
        fprintf(stderr, "FATAL: NULL expr passed to emit_iftableitem!\n");
        return newexpr(nil_e); // Return safe nil instead of exiting
    }

    // Skip invalid expressions
    if ((uintptr_t)e < 4096 || ((uintptr_t)e & 0xF) != 0) {
        fprintf(stderr, "FATAL: Invalid expr pointer: %p\n", (void *)e);
        return newexpr(nil_e); // Return safe nil instead of exiting
    }

    if (e->type != tableitem_e){
        return e;
    }

    if (!e->index) {
        fprintf(stderr, "FATAL: expr->index is NULL in emit_iftableitem!\n");
        return newexpr(nil_e); // Return safe nil instead of exiting
    }

    expr *result = newexpr(var_e);
    result->sym = newtemp();
    emit(tablegetelem, e, e->index, result, 0, yylineno);
    return result;
}

/* Break / Continue management */

int newlist(int quadNo) {
    quads[quadNo].label = 0;
    return quadNo;
}

int mergelist(int l1, int l2) {
    if (!l1){
        return l2;
    }
    if (!l2){
        return l1;
    }
    int i = l1;
    while (quads[i].label){
        i = quads[i].label;
    }
    quads[i].label = l2;
    return l1;
}

void patchlist(int list, int label) {
    while (list) {
        int next = quads[list].label;
        quads[list].label = label;
        list = next;
    }
}

void make_stmt(stmt_t *s) {
    s->breaklist = s->contlist = 0;
}

static void print_expr(FILE *f, expr *e) {
    if (!e) {
        fprintf(f, "nil");
        return;
    }

    // Handle nil_e expressions specially
    if (e->type == nil_e) {
        fprintf(f, "NIL");
        return;
    }

    /* 2. Types that do not depend on sym */
    switch (e->type) {
    case constnum_e:
        fprintf(f, "%.2f", e->numConst);
        return;
    case conststring_e:
        fprintf(f, "\"%s\"", e->strConst);
        return;
    case constbool_e:
        fprintf(f, "%s", e->boolConst ? "TRUE" : "FALSE");
        return;
    case nil_e:
        fprintf(f, "NIL");
        return;
    default:
        break;
    }

    if (!e->sym){ 
        fprintf(f, "anonymous");
        return;
    }

    switch (e->type) {
    case var_e:
    case tableitem_e:
    case arithexpr_e:
    case assignexpr_e:
        fprintf(f, "%s", e->sym->name);
        break;

    case programfunc_e:
    case libraryfunc_e:
        fprintf(f, "%s()", e->sym->name);
        break;

    case newtable_e:
        fprintf(f, "[table]");
        break;

    default:
        fprintf(f, "UNKNOWN");
        break;
    }
}

void print_quads(FILE *f) {
    fprintf(f, "------------------------------ Intermediate Code ------------------------------\n");

    for (unsigned i = 0; i < currQuad; ++i) {
        quad *q = quads + i;
        fprintf(f, "%-3u: ", i);

        switch (q->op) {
            case assign:
                print_expr(f, q->result);
                fprintf(f, " := ");
                print_expr(f, q->arg1);
                break;
            case add:
                print_expr(f, q->result);
                fprintf(f, " := ");
                print_expr(f, q->arg1);
                fprintf(f, " + ");
                print_expr(f, q->arg2);
                break;
            case sub:
                print_expr(f, q->result);
                fprintf(f, " := ");
                print_expr(f, q->arg1);
                fprintf(f, " - ");
                print_expr(f, q->arg2);
                break;
            case mul:
                print_expr(f, q->result);
                fprintf(f, " := ");
                print_expr(f, q->arg1);
                fprintf(f, " * ");
                print_expr(f, q->arg2);
                break;
            case idiv:
                print_expr(f, q->result);
                fprintf(f, " := ");
                print_expr(f, q->arg1);
                fprintf(f, " / ");
                print_expr(f, q->arg2);
                break;
            case mod:
                print_expr(f, q->result);
                fprintf(f, " := ");
                print_expr(f, q->arg1);
                fprintf(f, " %% ");
                print_expr(f, q->arg2);
                break;
            case uminus:
                print_expr(f, q->result);
                fprintf(f, " := -");
                print_expr(f, q->arg1);
                break;
            case and:
                print_expr(f, q->result);
                fprintf(f, " := ");
                print_expr(f, q->arg1);
                fprintf(f, " and ");
                print_expr(f, q->arg2);
                break;
            case or:
                print_expr(f, q->result);
                fprintf(f, " := ");
                print_expr(f, q->arg1);
                fprintf(f, " or ");
                print_expr(f, q->arg2);
                break;
            case not:
                print_expr(f, q->result);
                fprintf(f, " := not ");
                print_expr(f, q->arg1);
                break;
            case if_eq:
                fprintf(f, "IF ");
                print_expr(f, q->arg1);
                fprintf(f, " == ");
                print_expr(f, q->arg2);
                fprintf(f, " THEN jump to %u", q->label);
                break;
            case if_noteq:
                fprintf(f, "IF ");
                print_expr(f, q->arg1);
                fprintf(f, " != ");
                print_expr(f, q->arg2);
                fprintf(f, " THEN jump to %u", q->label);
                break;
            case if_lesseq:
                fprintf(f, "IF ");
                print_expr(f, q->arg1);
                fprintf(f, " <= ");
                print_expr(f, q->arg2);
                fprintf(f, " THEN jump to %u", q->label);
                break;
            case if_greatereq:
                fprintf(f, "IF ");
                print_expr(f, q->arg1);
                fprintf(f, " >= ");
                print_expr(f, q->arg2);
                fprintf(f, " THEN jump to %u", q->label);
                break;
            case if_less:
                fprintf(f, "IF ");
                print_expr(f, q->arg1);
                fprintf(f, " < ");
                print_expr(f, q->arg2);
                fprintf(f, " THEN jump to %u", q->label);
                break;
            case if_greater:
                fprintf(f, "IF ");
                print_expr(f, q->arg1);
                fprintf(f, " > ");
                print_expr(f, q->arg2);
                fprintf(f, " THEN jump to %u", q->label);
                break;
            case call:
                fprintf(f, "CALL ");
                print_expr(f, q->arg1);
                break;
            case param:
                fprintf(f, "PARAM ");
                print_expr(f, q->arg1);
                break;
            case ret:
                fprintf(f, "RETURN ");
                print_expr(f, q->arg1);
                break;
            case getretval:
                print_expr(f, q->result);
                fprintf(f, " := RETVAL");
                break;
            case funcstart:
                fprintf(f, "FUNCSTART ");
                print_expr(f, q->result);
                break;
            case funcend:
                fprintf(f, "FUNCEND ");
                print_expr(f, q->result);
                break;
            case tablecreate:
                print_expr(f, q->result);
                fprintf(f, " := TABLECREATE");
                break;
            case tablegetelem:
                print_expr(f, q->result);
                fprintf(f, " := ");
                print_expr(f, q->arg1);
                fprintf(f, "[");
                print_expr(f, q->arg2);
                fprintf(f, "]");
                break;
            case tablesetelem:
                print_expr(f, q->arg1);
                fprintf(f, "[");
                print_expr(f, q->arg2);
                fprintf(f, "] := ");
                print_expr(f, q->result);
                break;
            case jump:
                fprintf(f, "jump to %u", q->label);
                break;
            default:
                fprintf(f, "[Unknown opcode]");
                break;
        }

        fprintf(f, "\n");
    }

    fprintf(f, "\n%-6s %-12s %-20s %-20s %-20s %-5s\n", "quad#", "opcode", "result", "arg1", "arg2", "label");

    for (unsigned i = 0; i < currQuad; ++i) {
        quad *q = quads + i;

        char res_buf[64], arg1_buf[64], arg2_buf[64];
        const char *res_str  = q->result ? expr_to_str_buf(q->result, res_buf, sizeof(res_buf)) : "nil";
        const char *arg1_str = q->arg1   ? expr_to_str_buf(q->arg1, arg1_buf, sizeof(arg1_buf)) : "nil";
        const char *arg2_str = q->arg2   ? expr_to_str_buf(q->arg2, arg2_buf, sizeof(arg2_buf)) : "nil";

        fprintf(f, "%-6u %-12s %-20s %-20s %-20s %-5u   [line %u]\n",
            i + 1,
            op_to_str(q->op),
            res_str,
            arg1_str,
            arg2_str,
            q->label,
            q->line
        );
    }
}


expr* convert_to_value(expr* bool_expr) {
    if (bool_expr->type != boolexpr_e){
        return bool_expr;
    }
    expr* result = newexpr(var_e);
    result->sym = newtemp();

    unsigned trueLabel = nextquad();
    emit(assign, newexpr_constbool(1), NULL, result, 0, yylineno);

    unsigned jumpLabel = nextquad();
    emit(jump, NULL, NULL, NULL, 0, yylineno);

    unsigned falseLabel = nextquad();
    emit(assign, newexpr_constbool(0), NULL, result, 0, yylineno);

    patchlist(bool_expr->truelist, trueLabel);
    patchlist(bool_expr->falselist, falseLabel);
    patchlabel(jumpLabel, nextquad());

    return result;
}

expr* convert_to_bool(expr* e) {
    if (e->type == boolexpr_e) return e;

    expr* bool_expr = newexpr(boolexpr_e);
    emit(if_eq, e, newexpr_constbool(1), NULL, nextquad() + 2, yylineno);
    emit(jump, NULL, NULL, NULL, nextquad() + 2, yylineno);

    bool_expr->truelist = newlist(nextquad() - 2);
    bool_expr->falselist = newlist(nextquad() - 1);
    return bool_expr;
}

expr* make_not(expr* e) {
    if (e->type != boolexpr_e){
        e = convert_to_bool(e);  // emits if_eq/jump to produce true/false lists
    }
    expr* r = newexpr(boolexpr_e);
    r->truelist = e->falselist;
    r->falselist = e->truelist;
    return r;
}

expr* make_or(expr* e1, expr* e2) {
    patchlist(e1->falselist, nextquad());  // If e1 is false, evaluate e2

    expr* result = newexpr(boolexpr_e);
    result->truelist = mergelist(e1->truelist, e2->truelist);
    result->falselist = e2->falselist;

    return result;
}

expr* make_and(expr* e1, expr* e2) {
    patchlist(e1->truelist, nextquad());

    expr* result = newexpr(boolexpr_e);
    result->truelist = e2->truelist;
    result->falselist = mergelist(e1->falselist, e2->falselist);

    return result;
}

expr* make_eq_neq(expr* e1, expr* e2, iopcode op) {
    if (!e1) e1 = newexpr(nil_e);
    if (!e2) e2 = newexpr(nil_e);

    if (e1->type == boolexpr_e){
        e1 = convert_to_value(e1);
    }
    if (e2->type == boolexpr_e){
        e2 = convert_to_value(e2);
    }
    expr* r = newexpr(boolexpr_e);
    r->sym = newtemp();

    if (e1->type == nil_e || e2->type == nil_e) {
        emit(assign, newexpr_constbool(0), NULL, r, 0, yylineno);
    } else {
        emit(op, e1, e2, NULL, nextquad() + 2, yylineno);
        emit(jump, NULL, NULL, NULL, nextquad() + 1, yylineno);

        r->truelist = newlist(nextquad() - 2);
        r->falselist = newlist(nextquad() - 1);
    }
    return r;
}
