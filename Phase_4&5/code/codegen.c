#include "codegen.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#define INSTRUCTION_ARRAY_SIZE 1024 

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
void generate_ADD   (quad *q) { generate(op_add,           q); }
void generate_SUB   (quad *q) { generate(op_sub,           q); }
void generate_MUL   (quad *q) { generate(op_mul,           q); }
void generate_DIV   (quad *q) { generate(op_div,           q); }
void generate_MOD   (quad *q) { generate(op_mod,           q); }
void generate_NEWTABLE     (quad *q) { generate(op_newtable,      q); }
void generate_TABLEGETELM  (quad *q) { generate(op_tablegetelem,  q); }
void generate_TABLESETELEM (quad *q) { generate(op_tablesetelem,  q); }
void generate_ASSIGN       (quad *q) { generate(op_assign,        q); }
void generate_NOP          (void)   { instruction t = { .opcode = op_nop }; emit_instruction(t); }

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

int nextinstructionlabel(void) {
    return currInstruction;
}

void emit_instruction(instruction t) {
    assert(currInstruction < INSTRUCTION_ARRAY_SIZE);
    instructions[currInstruction++] = t;
}

void make_operand(void *expr, vmarg *arg) {
    arg->type = label_a;
    arg->value = 0;
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

void write_binary(const char *filename, unsigned int instr_count) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) { perror("Cannot open binary file"); exit(1); }
    fwrite(&instr_count, sizeof(instr_count), 1, fp);
    fwrite(instructions, sizeof(instruction), instr_count, fp);
    fclose(fp);
}