#ifndef CODEGEN_H
#define CODEGEN_H

#include "quads.h"

extern double*    numConsts;
extern unsigned   totalNumConsts;
extern char**     stringConsts;
extern unsigned   totalStringConsts;

typedef enum {
    label_a,    /* branch targets & fallbacks */
    number_a,   /* constant number */
    string_a,   /* constant string */
    bool_a,     /* constant boolean */
    global_a,   /* global variable */
    formal_a,   /* function formal argument */
    local_a,    /* function-local var */
    retval_a,    /* function return-value slot */
    nil_a
} vmarg_t;

/* One VM argument */
typedef struct {
    vmarg_t type;
    int     value;
} vmarg;

/* Final-code opcodes */
typedef enum {
    op_add, op_sub, op_mul, op_div, op_mod,
    op_newtable, op_tablegetelem, op_tablesetelem,
    op_assign, op_nop,
    op_jeq, op_jne, op_jgt, op_jge, op_jlt, op_jle,
    op_pusharg, op_callfunc, op_getretval,
    op_uminus
} opcode_t;

/* One final instruction */
typedef struct {
    opcode_t opcode;
    vmarg    arg1, arg2, result;
} instruction;

/* Incomplete-jump chain */
typedef struct incomplete_jump {
    int instrNo;
    int iaddress;
    struct incomplete_jump *next;
} incomplete_jump;

/* Arrays and globals */
extern instruction     instructions[];
extern incomplete_jump *ijumps_head;
extern unsigned int currInstruction;

/* Helper routines */
int  nextinstructionlabel(void);
void emit_instruction(instruction t);
void reset_operand(vmarg *arg);
void make_booloperand(vmarg *arg, int boolean);
void make_retvaloperand(vmarg *arg);
void add_incomplete_jump(int instrNo, int iaddress);
void make_operand(expr *e, vmarg *arg);

/* Entry points */
void patch_incomplete_jumps(void);
void generate(opcode_t op, quad *q);

/* Per-op generators */
void generate_ADD      (quad *q);
void generate_SUB      (quad *q);
void generate_MUL      (quad *q);
void generate_DIV      (quad *q);
void generate_MOD      (quad *q);
void generate_NEWTABLE (quad *q);
void generate_TABLEGETELM  (quad *q);
void generate_TABLESETELEM (quad *q);
void generate_ASSIGN       (quad *q);
void generate_NOP          (void);
void generate_UMINUS       (quad *q);

/* Jumps & relational */
void generate_relational(opcode_t op, quad *q);
void generate_JUMP       (quad *q);
void generate_IF_EQ      (quad *q);
void generate_IF_NOTEQ   (quad *q);
void generate_IF_GREATER (quad *q);
void generate_IF_GREATEREQ(quad *q);
void generate_IF_LESS    (quad *q);
void generate_IF_LESSEQ  (quad *q);

/* Boolean shortcuts */
void generate_NOT(quad *q);
void generate_OR (quad *q);
void generate_AND(quad *q);

/* Function-call mechanics */
void generate_PARAM    (quad *q);
void generate_CALL     (quad *q);
void generate_GETRETVAL(quad *q);

void generate_FUNCSTART(quad *q);
void generate_FUNCEND(quad *q);
void generate_RET(quad *q);

void generate_target_code(void);

/* Print/write */
void print_instructions(FILE* out);
void write_text(const char *filename, unsigned int instr_count);
void write_binary(const char *filename, unsigned int instr_count);

void write_numConsts(const char *filename);
void write_stringConsts(const char *filename);
void write_vmarg(FILE *out, vmarg *arg);
void emit_params_rev(expr *args);

#endif /* CODEGEN_H */
