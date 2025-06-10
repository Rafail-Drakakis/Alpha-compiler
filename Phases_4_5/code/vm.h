#ifndef VM_H
#define VM_H

#include <stdint.h>
#include "codegen.h"

#define STACK_SIZE    4096
#define TABLE_HASHSIZE  1024
#define MAX_LIBFUNCS   16

typedef enum {
  number_m,
  string_m,
  bool_m,
  table_m,
  userfunc_m,
  libfunc_m,
  nil_m,
  undef_m
} avm_memcell_t;

typedef struct avm_memcell {
  avm_memcell_t type;
  union {
    double     numVal;   /* for number_m */
    char*      strVal;   /* for string_m */
    unsigned char  boolVal;  /* for bool_m */
    struct avm_table* tableVal; /* for table_m */
    unsigned    funcVal;  /* for userfunc_m: instruction‐index */
    char*      libfuncVal; /* for libfunc_m */
  } data;
  unsigned    refCounter;  /* for tables only (ref‐counting) */
} avm_memcell;

/* A single bucket in the hash‐table for runtime tables. */
typedef struct avm_table_bucket {
  avm_memcell*   key;   /* must be number_m, string_m, or bool_m */
  avm_memcell*   value;  /* can be any memcell type */
  struct avm_table_bucket* next;
} avm_table_bucket;

/* A runtime table (associative array). */
typedef struct avm_table {
  unsigned      refCounter;
  avm_table_bucket*  numIndexed[TABLE_HASHSIZE];
  avm_table_bucket*  strIndexed[TABLE_HASHSIZE];
  avm_table_bucket*  boolIndexed[2];
} avm_table;

extern avm_memcell  stack[STACK_SIZE];
extern unsigned    top, topsp;
extern instruction*  code;    /* pointer to loaded instruction array */
extern unsigned    pc;     /* program counter: index into 'code' */
extern unsigned    total_instructions;
extern double*    const_nums;   /* array from out_numConsts.bin */
extern unsigned    total_const_nums;
extern char**     const_strs;   /* array from out_stringConsts.bin */
extern unsigned    total_const_strs;
typedef void (*library_func_t)(void);
extern const char*  libfunc_names[];
extern library_func_t libfunc_ptrs[];

void vm_init(void);
void vm_run(void);
void avm_error(const char *fmt, ...);
char *avm_tostring(avm_memcell *cell);
avm_table* avm_table_new(void);
void avm_table_inc_ref(avm_table *tbl);
void avm_table_dec_ref(avm_table *tbl);
avm_memcell* avm_tablegetelem(avm_table *tbl, avm_memcell *key);
void avm_tablesetelem(avm_table *tbl, avm_memcell *key, avm_memcell *value);
void execute_ADD(instruction *instr);
void execute_SUB(instruction *instr);
void execute_MUL(instruction *instr);
void execute_DIV(instruction *instr);
void execute_MOD(instruction *instr);
void execute_JEQ(instruction *instr);
void execute_JNE(instruction *instr);
void execute_JGT(instruction *instr);
void execute_JGE(instruction *instr);
void execute_JLT(instruction *instr);
void execute_JLE(instruction *instr);
void execute_ASSIGN(instruction *instr);
void execute_NEWTABLE(instruction *instr);
void execute_TABLEGETELM(instruction *instr);
void execute_TABLESETELEM(instruction *instr);
void execute_NOP(instruction *instr);
void execute_PUSHARG(instruction *instr);
void execute_CALLFUNC(instruction *instr);
void execute_GETRET(instruction *instr);
void execute_FUNCENTER(instruction *instr);
void execute_FUNCEXIT(instruction *instr);
void execute_AND(instruction *instr);
void execute_OR(instruction *instr);
void execute_NOT(instruction *instr);
void avm_registerlibfunc(const char *name, library_func_t ptr);
void avm_calllibfunc(const char *name);
void libfunc_typeof(void);
void libfunc_totalarguments(void);
void libfunc_argument(void);
void libfunc_print(void);
unsigned totalactuals(void);

avm_memcell* avm_translate_operand(vmarg *arg, avm_memcell *reg);

void avm_destroy(void);

#endif
