/**
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#include <stdio.h>
#include "quads.h"
#include "symbol_table.h"

unsigned programVarOffset = 0;
unsigned functionLocalOffset = 0;
unsigned formalArgOffset = 0;
unsigned scopeSpaceCounter = 1;

unsigned total = 0;
unsigned int currQuad = 0;

static unsigned tempcounter = 0;
SymbolTable *symbol_table;

quad* quads = (quad*) 0;

void expand(void) {
    assert(total == currQuad);
    quad* p = (quad*) malloc(NEW_SIZE);
    if (quads) {
        memcpy(p, quads, CURR_SIZE);
        free(quads);
    }
    quads = p;
    total += EXPAND_SIZE;
}

void emit(iopcode op, expr* arg1, expr* arg2, expr* result, unsigned label, unsigned line) {
    if (currQuad == total) expand();

    quad* q = quads + currQuad++;
    q->op = op;
    q->arg1 = arg1;
    q->arg2 = arg2;
    q->result = result;
    q->label = label;
    q->line = line;
}

unsigned nextquad(void) {
    return currQuad;
}

void patchlabel(unsigned quadNo, unsigned label) {
    assert(quadNo < currQuad);
    quads[quadNo].label = label;
}

/* Scoping */

scopespace_t currscopespace(void) {
    if (scopeSpaceCounter == 1)
        return programvar;
    else if (scopeSpaceCounter % 2 == 0)
        return formalarg;
    else
        return functionlocal;
}

unsigned currscopeoffset(void) {
    switch (currscopespace()) {
        case programvar: return programVarOffset;
        case functionlocal: return functionLocalOffset;
        case formalarg: return formalArgOffset;
        default: assert(0);
    }
}

void inccurrscopeoffset(void) {
    switch (currscopespace()) {
        case programvar: ++programVarOffset; break;
        case functionlocal: ++functionLocalOffset; break;
        case formalarg: ++formalArgOffset; break;
        default: assert(0);
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

expr* newexpr(expr_t t) {
    expr* e = (expr*) malloc(sizeof(expr));
    memset(e, 0, sizeof(expr));
    e->type = t;
    return e;
}

expr* newexpr_constnum(double i) {
    expr* e = newexpr(constnum_e);
    e->numConst = i;
    return e;
}

expr* newexpr_conststring(char* s) {
    expr* e = newexpr(conststring_e);
    e->strConst = strdup(s);
    return e;
}

expr* newexpr_constbool(unsigned int b) {
    expr* e = newexpr(constbool_e);
    e->boolConst = !!b;
    return e;
}

char* newtempname(void) {
    char* name = malloc(10);
    sprintf(name, "_t%u", tempcounter++);
    return name;
}

SymbolTableEntry* newtemp(void) {
    char* name = newtempname();
    SymbolTableEntry* sym = lookup_symbol(symbol_table, name, currscope(), 0);
    if (!sym) {
        insert_symbol(symbol_table, name, TEMP_VAR, yylineno, currscope());
        sym = lookup_symbol(symbol_table, name, currscope(), 0);
    }
    return sym;
}

void resettemp(void) {
    tempcounter = 0;
}

unsigned int istempname(char* s) {
    return *s == '_';
}

unsigned int istempexpr(expr* e) {
    return e && e->sym && istempname(e->sym->name);
}

expr* lvalue_expr(SymbolTableEntry* sym) {
    assert(sym);
    expr* e = newexpr(var_e);
    e->sym = sym;

    switch (sym->type) {
        case var_s: e->type = var_e; break;
        case programfunc_s: e->type = programfunc_e; break;
        case libraryfunc_s: e->type = libraryfunc_e; break;
        default: assert(0);
    }

    return e;
}

expr* emit_iftableitem(expr* e) {
    if (e->type != tableitem_e) return e;

    expr* result = newexpr(var_e);
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
    if (!l1) return l2;
    if (!l2) return l1;

    int i = l1;
    while (quads[i].label)
        i = quads[i].label;
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

void make_stmt(stmt_t* s) {
    s->breaklist = s->contlist = 0;
}

static void print_expr(FILE* f, expr* e) {
    if (!e) {
        fprintf(f, "nil");
        return;
    }

    switch (e->type) {
        case var_e:
        case tableitem_e:
        case arithexpr_e:
        case assignexpr_e:
            fprintf(f, "%s", e->sym->name);
            break;
        case constnum_e:
            fprintf(f, "%.2f", e->numConst);
            break;
        case constbool_e:
            fprintf(f, "%s", e->boolConst ? "TRUE" : "FALSE");
            break;
        case conststring_e:
            fprintf(f, "\"%s\"", e->strConst);
            break;
        case nil_e:
            fprintf(f, "NIL");
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

void main(FILE* f) {
    fprintf(f, "------------------------------ Intermediate Code ------------------------------\n");

    for (int i = 0; i < currQuad; i++) {
        quad* q = quads + i;
        fprintf(f, "%-3d: ", i);

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
            case if_geatereq:
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
            default:
                fprintf(f, "[Unknown opcode]");
                break;
        }

        fprintf(f, "\n");
    }

    fprintf(f, "-------------------------------------------------------------------------------\n");
}