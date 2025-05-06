#include <stdio.h>
#include "quads.h"

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

void print_quads(FILE* f) {
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
            case div:
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