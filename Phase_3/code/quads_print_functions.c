#include "quads.h" 

/* some cases are going to have subcases so we will need helper functions for print_quads */

void print_quads(FILE * f) {
    fprintf(f, "------------------------------ Intermediate Code ------------------------------\n");
    for(int i = 0; i < currQuad; i++){
        switch ((quads+i)->op) {
        case assign:
            /* code */
            break;
        case add:
            /* code */
            break;
        case sub:
            /* code */
            break;
        case mul:
            /* code */
            break;
        case div:
            /* code */
            break;
        case mod:
            /* code */
            break;
        case uminus:
            /* code */
            break;
        case and:
            /* code */
            break;
        case or:
            /* code */
            break;
        case not:
            /* code */
            break;
        case if_eq:
            /* code */
            break;
        case if_noteq:
            /* code */
            break;
        case if_lesseq:
            /* code */
            break;
        case if_geatereq:
            /* code */
            break;
        case if_less:
            /* code */
            break;
        case if_greater:
            /* code */
            break;
        case call:
            /* code */
            break;
        case param:
            /* code */
            break;
        case ret:
            /* code */
            break;
        case getretval:
            /* code */
            break;
        case funcstart:
            /* code */
            break;
        case funcend:
            /* code */
            break;
        case tablecreate:
            /* code */
            break;
        case tablegetelem:
            /* code */
            break;
        case tablesetelem:
            /* code */
            break;

        default:
            /* should not reach this */
            break;
        }
    }
}