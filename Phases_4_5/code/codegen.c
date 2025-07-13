/**
 * HY-340 Project Phases 4 & 5 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#include "codegen.h"
#include "symbol_table.h"
#include "quads.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

unsigned vm_stack_top    = 0;
unsigned vm_stack_topsp  = 0;

#define INSTRUCTION_ARRAY_SIZE 1024 
#define GLOBAL_BASE  0
#define TOP          vm_stack_top     
#define TOPSP        vm_stack_topsp  

double*  numConsts       = NULL; 
unsigned totalNumConsts  = 0;
char**   stringConsts    = NULL;
unsigned totalStringConsts = 0;

instruction instructions[INSTRUCTION_ARRAY_SIZE];
unsigned int currInstruction = 0;

incomplete_jump *ijumps_head = NULL;

void patch_incomplete_jumps(void) {
    incomplete_jump *ij = ijumps_head;
    while (ij) {
        instructions[ij->instrNo].result.value = quads[ij->iaddress].taddress;
        ij = ij->next;
    }
}

void generate(opcode_t op, quad *q) {
    instruction t;
    t.opcode = op;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    make_operand(q->result, &t.result);
    q->taddress = nextinstructionlabel();
    emit_instruction(t);
}

/* Arithmetic/table/assign/NOP */
void generate_ADD           (quad *q) { generate(op_add,           q); }
void generate_SUB           (quad *q) { generate(op_sub,           q); }
void generate_MUL           (quad *q) { generate(op_mul,           q); }
void generate_DIV           (quad *q) { generate(op_div,           q); }
void generate_MOD           (quad *q) { generate(op_mod,           q); }
void generate_NEWTABLE      (quad *q) { generate(op_newtable,      q); }
void generate_TABLEGETELM   (quad *q) { generate(op_tablegetelem,  q); }
void generate_TABLESETELEM(quad *q)
{
    instruction t;
    t.opcode = op_tablesetelem;
    make_operand(q->result, &t.arg1);   /* the table   */
    make_operand(q->arg1,   &t.arg2);   /* the key     */
    make_operand(q->arg2,   &t.result); /* the value   */

    q->taddress = nextinstructionlabel();
    emit_instruction(t);
}
void generate_ASSIGN        (quad *q) { generate(op_assign,        q); }
void generate_NOP           (void)   { instruction t = { .opcode = op_nop }; emit_instruction(t); }

void generate_UMINUS(quad *q) {
    // fprintf(stderr, "[GEN] Generating uminus instruction for quad at line %d\n", q->line);
    generate(op_uminus, q);
}

static unsigned add_numconst(double x) {
    for (unsigned i=0; i<totalNumConsts; ++i)
        if (numConsts[i]==x) return i;
    numConsts = realloc(numConsts, (totalNumConsts+1)*sizeof *numConsts);
    numConsts[totalNumConsts] = x;
    return totalNumConsts++;
}

static unsigned add_strconst(const char* s) {
    for (unsigned i=0; i<totalStringConsts; ++i)
        if (strcmp(stringConsts[i],s)==0) return i;
    stringConsts = realloc(stringConsts,(totalStringConsts+1)*sizeof *stringConsts);
    stringConsts[totalStringConsts] = strdup(s);
    return totalStringConsts++;
}

void make_operand(expr *e, vmarg *arg) {
    if (!e) {
        arg->type  = label_a;
        arg->value = 0;
        return;
    }

    switch (e->type) {
        case constnum_e:
            arg->type  = number_a;
            arg->value = add_numconst(e->numConst);
            break;
        case conststring_e:
            arg->type  = string_a;
            arg->value = add_strconst(e->strConst);
            break;
        case constbool_e:
            arg->type  = bool_a;
            arg->value = e->boolConst;
            break;
        case var_e:
        case arithexpr_e:
        case assignexpr_e:
        case boolexpr_e:
        case newtable_e: 
        case tableitem_e: {
            /* guard against missing symbol */
            if (!e->sym) {
                arg->type  = label_a;
                arg->value = 0;
                break;
            }
            SymbolTableEntry *sym = e->sym;
            unsigned offset = sym->offset;
	          // printf("PARAM for symbol '%s' with offset %u and space %d\n", sym->name, offset, sym->space);
            switch (sym->space) {
                case programvar:
                    arg->type  = global_a;
                    arg->value = GLOBAL_BASE + offset;
                    break;
                case formalarg:
                    arg->type  = formal_a;
                    arg->value = TOPSP - offset;
                    break;
                case functionlocal:
                    arg->type  = local_a;
                    arg->value = TOP + offset + 1;
                    break;
            }
            break;
        }
        case nil_e:
            arg->type = nil_a;
            break;
        case call_e:
            arg->type  = retval_a;
            arg->value = 0;
            break;
        case programfunc_e:
        case libraryfunc_e: {
          SymbolTableEntry *sym = e->sym;
          arg->type  = global_a;
          arg->value = GLOBAL_BASE + sym->offset;
          break;
        }
        default:
            arg->type  = label_a;
            arg->value = 0;
    }
}

void generate_FUNCSTART(quad *q) {
    q->taddress = nextinstructionlabel();

    /* 1) push old TOPSP */
    instruction t1 = { .opcode = op_pusharg };
    make_retvaloperand(&t1.arg1);           /* encode old TOPSP */
    emit_instruction(t1);

    /* 2) set TOPSP = TOP */
    instruction t2 = { .opcode = op_assign };
    make_operand(NULL, &t2.arg1);           /* no-op src */
    make_retvaloperand(&t2.arg2);            /* dest = old retval slot */
    emit_instruction(t2);

    unsigned locals = (unsigned)q->arg1->numConst;
    for (unsigned i = 0; i < locals; ++i) {
        instruction t = { .opcode = op_nop };
        emit_instruction(t);  
    }
}

void generate_FUNCEND(quad *q) {
    q->taddress = nextinstructionlabel();

    /* 1) restore TOP = TOPSP */
    instruction t1 = { .opcode = op_assign };
    make_retvaloperand(&t1.arg1);      /* arg1 = old TOPSP value */
    reset_operand(&t1.arg2);
    emit_instruction(t1);

    /* 2) pop old TOPSP (incomplete jump back address) */
    instruction t2 = { .opcode = op_nop }; 
    emit_instruction(t2);              /* or op_poptops if defined */

    /* 3) return via incomplete jump */
    instruction t3 = { .opcode = op_jne }; /* any conditional unused */
    t3.result.type  = label_a;
    if (q->label < nextinstructionlabel()) {
        t3.result.value = quads[q->label].taddress;
    } else {
        add_incomplete_jump(nextinstructionlabel(), q->label);
    }
    emit_instruction(t3);
}

void generate_target_code(void) {
    currInstruction = 0;   /* clear out any old instructions */
    ijumps_head   = NULL; 
    for (unsigned i = 0; i < currQuad; ++i) {
        quad *q = &quads[i];
        switch (q->op) {
          /* Arithmetic */
          case add:     generate_ADD(q);  break;
          case sub:     generate_SUB(q);  break;
          case mul:     generate_MUL(q);  break;
          case idiv:    generate_DIV(q);  break;
          case mod:     generate_MOD(q);  break;
	  case uminus:  generate_UMINUS(q); break;
          /* Table / Assign */
          case tablecreate:
            generate_NEWTABLE(q); break;
          case tablegetelem:
            generate_TABLEGETELM(q); break;
          case tablesetelem:
            generate_TABLESETELEM(q); break;
          case assign:
            generate_ASSIGN(q); break;
          /* Jumps & Relational */
          case jump:
            generate_JUMP(q); break;
          case if_eq:
            generate_IF_EQ(q); break;
          case if_noteq:
            generate_IF_NOTEQ(q); break;
          case if_greater:
            generate_IF_GREATER(q); break;
          case if_greatereq:
            generate_IF_GREATEREQ(q); break;
          case if_less:
            generate_IF_LESS(q); break;
          case if_lesseq:
            generate_IF_LESSEQ(q); break;
          /* Boolean */
          case and:
            generate_AND(q); break;
          case or:
            generate_OR(q); break;
          case not:
            generate_NOT(q); break;
          /* Calls */
          case param:
            generate_PARAM(q); break;
          case call:
            generate_CALL(q); break;
          case getretval:
            generate_GETRETVAL(q); break;
          /* Functions */
          case funcstart:
            generate_FUNCSTART(q); break;
          case funcend:
            generate_FUNCEND(q);   break;
          default:
            generate_NOP();
        }
    }
    patch_incomplete_jumps();
}

/* Relational and unconditional jumps */
void generate_relational(opcode_t op, quad *q) {
    instruction t;
    t.opcode = op;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    t.result.type = label_a;

    if (q->label < nextinstructionlabel()) {
        t.result.value = quads[q->label].taddress;
    } else {
        add_incomplete_jump(nextinstructionlabel(), q->label);
    }

    q->taddress = nextinstructionlabel();
    emit_instruction(t);
}

void generate_JUMP       (quad *q) { generate_relational(op_jne,    q); }
void generate_IF_EQ      (quad *q) { generate_relational(op_jeq,    q); }
void generate_IF_NOTEQ   (quad *q) { generate_relational(op_jne,    q); }
void generate_IF_GREATER (quad *q) { generate_relational(op_jgt,    q); }
void generate_IF_GREATEREQ(quad *q){ generate_relational(op_jge,    q); }
void generate_IF_LESS    (quad *q) { generate_relational(op_jlt,    q); }
void generate_IF_LESSEQ  (quad *q) { generate_relational(op_jle,    q); }

/* Logical operators */
void generate_NOT(quad *q) {
    q->taddress = nextinstructionlabel();
    instruction t;

    t.opcode = op_jeq;
    make_operand(q->arg1, &t.arg1);
    make_booloperand(&t.arg2, 0);
    t.result.type  = label_a;
    t.result.value = nextinstructionlabel() + 3;
    emit_instruction(t);

    t.opcode = op_assign;
    make_booloperand(&t.arg1, 0);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_instruction(t);

    t.opcode = op_jne;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type  = label_a;
    t.result.value = nextinstructionlabel() + 2;
    emit_instruction(t);

    t.opcode = op_assign;
    make_booloperand(&t.arg1, 1);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_instruction(t);
}

void generate_OR(quad *q) {
    q->taddress = nextinstructionlabel();
    instruction t;

    t.opcode = op_jeq;
    make_operand(q->arg1, &t.arg1);
    make_booloperand(&t.arg2, 1);
    t.result.type  = label_a;
    t.result.value = nextinstructionlabel() + 4;
    emit_instruction(t);

    make_operand(q->arg2, &t.arg1);
    t.result.value = nextinstructionlabel() + 3;
    emit_instruction(t);

    t.opcode = op_assign;
    make_booloperand(&t.arg1, 0);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_instruction(t);

    t.opcode = op_jne;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type  = label_a;
    t.result.value = nextinstructionlabel() + 2;
    emit_instruction(t);

    t.opcode = op_assign;
    make_booloperand(&t.arg1, 1);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_instruction(t);
}

void generate_AND(quad *q) {
    q->taddress = nextinstructionlabel();
    instruction t;

    t.opcode = op_jeq;
    make_operand(q->arg1, &t.arg1);
    make_booloperand(&t.arg2, 0);
    t.result.type  = label_a;
    t.result.value = nextinstructionlabel() + 4;
    emit_instruction(t);

    make_operand(q->arg2, &t.arg1);
    t.result.value = nextinstructionlabel() + 3;
    emit_instruction(t);

    t.opcode = op_assign;
    make_booloperand(&t.arg1, 1);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_instruction(t);

    t.opcode = op_jne;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type  = label_a;
    t.result.value = nextinstructionlabel() + 2;
    emit_instruction(t);

    t.opcode = op_assign;
    make_booloperand(&t.arg1, 0);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_instruction(t);
}

/* Function calls */
void generate_PARAM(quad *q) {
    q->taddress = nextinstructionlabel();
    instruction t = { .opcode = op_pusharg };
    make_operand(q->arg1, &t.arg1);

    // printf("PARAM for symbol '%s' at VM addr type=%d, val=%d\n",
    //        q->arg1->sym ? q->arg1->sym->name : "NULL",
    //        t.arg1.type, t.arg1.value);    
    emit_instruction(t);
}

void generate_CALL(quad *q) {
    q->taddress = nextinstructionlabel();
    instruction t = { .opcode = op_callfunc };
    make_operand(q->arg1, &t.arg1);
    emit_instruction(t);
}

void generate_GETRETVAL(quad *q) {
    q->taddress = nextinstructionlabel();
    instruction t = { .opcode = op_assign };
    make_operand(q->result, &t.result);
    make_retvaloperand(&t.arg1);
    emit_instruction(t);
}

void generate_RET(quad *q) {
    q->taddress = nextinstructionlabel();
    assert(q->result);
    instruction t = { .opcode = op_assign };
    make_operand(q->result, &t.arg1);
    make_retvaloperand(&t.result);
    reset_operand(&t.arg2);
    emit_instruction(t);
}

int nextinstructionlabel(void) {
    return currInstruction;
}

void emit_instruction(instruction t) {
    assert(currInstruction < INSTRUCTION_ARRAY_SIZE);
    instructions[currInstruction++] = t;
}

void reset_operand(vmarg *arg) {
    arg->type = label_a;
    arg->value = 0;
}

void make_booloperand(vmarg *arg, int boolean) {
    arg->type = label_a;
    arg->value = boolean;
}

void make_retvaloperand(vmarg *arg) {
    arg->type = label_a;
    arg->value = -1;
}

void add_incomplete_jump(int instrNo, int iaddress) {
    incomplete_jump* j = malloc(sizeof(incomplete_jump));
    j->instrNo = instrNo;
    j->iaddress = iaddress;
    j->next = ijumps_head;
    ijumps_head = j;
}

/* PRINT/WRITE CODE */
const char* opcode_names[] = {
    "ADD", "SUB", "MUL", "DIV", "MOD",
    "NEWTABLE", "TABLEGETELM", "TABLESETELEM", "ASSIGN", "NOP",
    "JEQ", "JNE", "JGT", "JGE", "JLT", "JLE",
    "PUSHARG", "CALLFUNC" /*We will add more*/
};

void print_instructions(FILE* out) {
    for (unsigned i = 0; i < currInstruction; ++i) {
        instruction* t = &instructions[i];
        fprintf(out, "%3u: %-12s ", i, opcode_names[t->opcode]);
        fprintf(out, "arg1=(%d,%d) ", t->arg1.type, t->arg1.value);
        fprintf(out, "arg2=(%d,%d) ", t->arg2.type, t->arg2.value);
        fprintf(out, "result=(%d,%d)\n", t->result.type, t->result.value);
    }
}

void write_text(const char *filename, unsigned int instr_count) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("Cannot open text file"); exit(1); }
    for (unsigned int i = 0; i < instr_count; ++i) {
        instruction* t = &instructions[i];
        fprintf(fp, "%3u: %-12s arg1=(%d,%d) arg2=(%d,%d) result=(%d,%d)\n",
            i, opcode_names[t->opcode],
            t->arg1.type, t->arg1.value,
            t->arg2.type, t->arg2.value,
            t->result.type, t->result.value
        );
    }
    fclose(fp);
}

void write_binary(const char *filename) {
    FILE *fp = fopen(filename,"wb");
    if(!fp) { perror("Cannot open output file"); exit(1); }

    // 1) instructions
    uint32_t instr_count = currInstruction;
    fwrite(&instr_count, sizeof(instr_count), 1, fp);
    fwrite(instructions, sizeof(instruction), instr_count, fp);

    // 2) numConsts
    fwrite(&totalNumConsts, sizeof(totalNumConsts), 1, fp);
    fwrite(numConsts, sizeof(double), totalNumConsts, fp);

    // 3) stringConsts
    fwrite(&totalStringConsts, sizeof(totalStringConsts), 1, fp);
    for(uint32_t i = 0; i < totalStringConsts; ++i){
        uint32_t len = (uint32_t)strlen(stringConsts[i]);
        fwrite(&len, sizeof(len), 1, fp);
        fwrite(stringConsts[i], sizeof(char), len, fp);
    }

    fclose(fp);
}
