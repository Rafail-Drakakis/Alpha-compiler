#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>
#include "vm.h"

/* ---------- Global VM State ---------- */

avm_memcell stack[STACK_SIZE];

unsigned top    = STACK_SIZE - 1;  /* initially, stack is empty */
unsigned topsp  = 0;
unsigned pc     = 0;

instruction *code = NULL;
unsigned total_instructions = 0;

/* Constant pools loaded from files */
double* const_nums         = NULL;
unsigned total_const_nums  = 0;
char**  const_strs         = NULL;
unsigned total_const_strs  = 0;

static const char*  libfunc_names_arr[MAX_LIBFUNCS];
static library_func_t libfunc_ptrs_arr[MAX_LIBFUNCS];
static unsigned num_registered_libfuncs = 0;

typedef struct {
    char* name;
    unsigned addr;  /* taddress in the instructions[] array */
} userfunc_entry;

static userfunc_entry userfuncs[256];
static unsigned num_userfuncs = 0;

/* ---------- Helper Prototypes ---------- */

static unsigned hash_number(double key);
static unsigned hash_string(const char *key);
static unsigned hash_bool(unsigned char key);

/* Forward declarations for execute_XXX() functions: */
void execute_ADD(instruction *instr);
void execute_SUB(instruction *instr);
void execute_MUL(instruction *instr);
void execute_DIV(instruction *instr);
void execute_MOD(instruction *instr);
void execute_ASSIGN(instruction *instr);
void execute_JEQ(instruction *instr);
void execute_JNE(instruction *instr);
void execute_JLE(instruction *instr);
void execute_JLT(instruction *instr);
void execute_JGE(instruction *instr);
void execute_JGT(instruction *instr);
void execute_NEWTABLE(instruction *instr);
void execute_TABLEGETELM(instruction *instr);
void execute_TABLESETELEM(instruction *instr);
void execute_PUSHARG(instruction *instr);
void execute_CALLFUNC(instruction *instr);
void execute_GETRET(instruction *instr);
void execute_FUNCENTER(instruction *instr);
void execute_FUNCEXIT(instruction *instr);
void execute_AND(instruction *instr);
void execute_OR(instruction *instr);
void execute_NOT(instruction *instr);
void execute_NOP(instruction *instr);

avm_memcell* avm_translate_operand(vmarg *arg, avm_memcell *reg);

/* ---------- Error‐printing helper ---------- */
void avm_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[VM ERROR] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(EXIT_FAILURE);
}

/* ---------- Memory‐cell to String for Debugging ---------- */
char *avm_tostring(avm_memcell *m) {
    static char buffer[256];
    switch (m->type) {
        case number_m:
            snprintf(buffer, sizeof(buffer), "%.2f", m->data.numVal);
            return buffer;
        case string_m:
            return m->data.strVal ? m->data.strVal : strdup("<null str>");
        case bool_m:
            return m->data.boolVal ? "true" : "false";
        case nil_m:
            return "nil";
        case undef_m:
            return "undef";
        case table_m:
            return "[table]";
        case userfunc_m: {
            for (unsigned i = 0; i < num_userfuncs; ++i)
                if (userfuncs[i].addr == m->data.funcVal) {
                    snprintf(buffer, sizeof(buffer), "<userfunc:%s>", userfuncs[i].name);
                    return buffer;
                }
            return "<userfunc:unknown>";
        }
        case libfunc_m:
            return m->data.libfuncVal ? m->data.libfuncVal : "<libfunc:null>";
        default:
            return "<invalid cell>";
    }
}

/* ---------- Reference Counting for Tables ---------- */
void avm_table_inc_ref(avm_table *tbl) {
    if (!tbl) return;
    tbl->refCounter++;
}

void avm_table_dec_ref(avm_table *tbl) {
    if (!tbl) return;
    if (--tbl->refCounter == 0) {
        /* Free all buckets */
        for (unsigned i = 0; i < TABLE_HASHSIZE; ++i) {
            avm_table_bucket *b = tbl->numIndexed[i];
            while (b) {
                avm_table_bucket *next = b->next;
                /* free key & value if needed */
                if (b->key) {
                    if (b->key->type == string_m && b->key->data.strVal) free(b->key->data.strVal);
                    free(b->key);
                }
                if (b->value) {
                    /* if it's a table, decrement its refcount */
                    if (b->value->type == table_m && b->value->data.tableVal)
                        avm_table_dec_ref(b->value->data.tableVal);
                    if (b->value->type == string_m && b->value->data.strVal) free(b->value->data.strVal);
                    free(b->value);
                }
                free(b);
                b = next;
            }
            b = tbl->strIndexed[i];
            while (b) {
                avm_table_bucket *next = b->next;
                if (b->key) {
                    if (b->key->type == string_m && b->key->data.strVal) free(b->key->data.strVal);
                    free(b->key);
                }
                if (b->value) {
                    if (b->value->type == table_m && b->value->data.tableVal)
                        avm_table_dec_ref(b->value->data.tableVal);
                    if (b->value->type == string_m && b->value->data.strVal) free(b->value->data.strVal);
                    free(b->value);
                }
                free(b);
                b = next;
            }
        }
        avm_table_bucket *b = tbl->boolIndexed[0];
        while (b) {
            avm_table_bucket *next = b->next;
            if (b->key) free(b->key);
            if (b->value) {
                if (b->value->type == table_m && b->value->data.tableVal)
                    avm_table_dec_ref(b->value->data.tableVal);
                if (b->value->type == string_m && b->value->data.strVal) free(b->value->data.strVal);
                free(b->value);
            }
            free(b);
            b = next;
        }
        b = tbl->boolIndexed[1];
        while (b) {
            avm_table_bucket *next = b->next;
            if (b->key) free(b->key);
            if (b->value) {
                if (b->value->type == table_m && b->value->data.tableVal)
                    avm_table_dec_ref(b->value->data.tableVal);
                if (b->value->type == string_m && b->value->data.strVal) free(b->value->data.strVal);
                free(b->value);
            }
            free(b);
            b = next;
        }
        free(tbl);
    }
}

avm_table* avm_table_new(void) {
    avm_table *tbl = malloc(sizeof(avm_table));
    tbl->refCounter = 0;
    for (unsigned i = 0; i < TABLE_HASHSIZE; ++i)
        tbl->numIndexed[i] = tbl->strIndexed[i] = NULL;
    tbl->boolIndexed[0] = tbl->boolIndexed[1] = NULL;
    return tbl;
}

/* ---------- Hash functions ---------- */
static unsigned hash_number(double key) {
    uint64_t bits;
    memcpy(&bits, &key, sizeof(bits));
    return (unsigned)((bits ^ (bits >> 32)) % TABLE_HASHSIZE);
}
static unsigned hash_string(const char *key) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + (unsigned)c; /* hash * 33 + c */
    return (unsigned)(hash % TABLE_HASHSIZE);
}
static unsigned hash_bool(unsigned char key) {
    return (unsigned)(key % 2);
}

/* ---------- Table get/set element ---------- */
avm_memcell* avm_tablegetelem(avm_table *tbl, avm_memcell *key) {
    if (!tbl || !key) return NULL;
    unsigned idx;
    avm_table_bucket *b = NULL;
    if (key->type == number_m) {
        idx = hash_number(key->data.numVal);
        b = tbl->numIndexed[idx];
    } else if (key->type == string_m) {
        idx = hash_string(key->data.strVal);
        b = tbl->strIndexed[idx];
    } else if (key->type == bool_m) {
        idx = hash_bool(key->data.boolVal);
        b = tbl->boolIndexed[idx];
    } else {
        return NULL; /* nil/undef cannot index a table */
    }
    while (b) {
        /* compare keys */
        if (key->type == number_m && b->key->data.numVal == key->data.numVal)
            return b->value;
        else if (key->type == string_m && strcmp(b->key->data.strVal, key->data.strVal) == 0)
            return b->value;
        else if (key->type == bool_m && b->key->data.boolVal == key->data.boolVal)
            return b->value;
        b = b->next;
    }
    return NULL; /* not found */
}

void avm_tablesetelem(avm_table *tbl, avm_memcell *key, avm_memcell *value) {
    if (!tbl || !key) return;
    unsigned idx;
    avm_table_bucket **bucket_array = NULL;
    avm_table_bucket **head = NULL;
    if (key->type == number_m) {
        idx = hash_number(key->data.numVal);
        head = &tbl->numIndexed[idx];
    } else if (key->type == string_m) {
        idx = hash_string(key->data.strVal);
        head = &tbl->strIndexed[idx];
    } else if (key->type == bool_m) {
        idx = hash_bool(key->data.boolVal);
        head = &tbl->boolIndexed[idx];
    } else {
        return; /* nil/undef cannot index */
    }

    /* If value is nil, delete any existing bucket */
    if (value->type == nil_m) {
        avm_table_bucket *prev = NULL, *cur = *head;
        while (cur) {
            if ((key->type == number_m && cur->key->data.numVal == key->data.numVal) ||
                (key->type == string_m && strcmp(cur->key->data.strVal, key->data.strVal) == 0) ||
                (key->type == bool_m && cur->key->data.boolVal == key->data.boolVal)) {
                /* Found it: remove from chain and free */
                if (prev) prev->next = cur->next;
                else *head = cur->next;
                if (cur->value && cur->value->type == table_m)
                    avm_table_dec_ref(cur->value->data.tableVal);
                free(cur->value);
                free(cur->key);
                free(cur);
                return;
            }
            prev = cur;
            cur = cur->next;
        }
        return;
    }

    /* Otherwise, insert or overwrite */
    avm_table_bucket *cur = *head;
    while (cur) {
        if ((key->type == number_m && cur->key->data.numVal == key->data.numVal) ||
            (key->type == string_m && strcmp(cur->key->data.strVal, key->data.strVal) == 0) ||
            (key->type == bool_m && cur->key->data.boolVal == key->data.boolVal)) {
            /* Overwrite the value */
            if (cur->value->type == table_m)
                avm_table_dec_ref(cur->value->data.tableVal);
            free(cur->value);
            cur->value = malloc(sizeof(avm_memcell));
            *cur->value = *value;
            if (value->type == string_m)
                cur->value->data.strVal = strdup(value->data.strVal);
            if (value->type == table_m) {
                cur->value->data.tableVal = value->data.tableVal;
                avm_table_inc_ref(cur->value->data.tableVal);
            }
            return;
        }
        cur = cur->next;
    }

    avm_table_bucket *newb = malloc(sizeof(avm_table_bucket));
    newb->key = malloc(sizeof(avm_memcell));
    *newb->key = *key;
    if (key->type == string_m)
        newb->key->data.strVal = strdup(key->data.strVal);
    newb->value = malloc(sizeof(avm_memcell));
    *newb->value = *value;
    if (value->type == string_m)
        newb->value->data.strVal = strdup(value->data.strVal);
    if (value->type == table_m) {
        newb->value->data.tableVal = value->data.tableVal;
        avm_table_inc_ref(newb->value->data.tableVal);
    }
    newb->next = *head;
    *head = newb;
}

static avm_memcell *retval_reg = NULL;

avm_memcell* avm_translate_operand(vmarg *arg, avm_memcell *reg) {
    switch (arg->type) {
        case number_a:
            reg->type = number_m;
            reg->data.numVal = const_nums[arg->value];
            return reg;
        case string_a:
            reg->type = string_m;
            reg->data.strVal = strdup(const_strs[arg->value]);
            return reg;
        case bool_a:
            reg->type = bool_m;
            reg->data.boolVal = (unsigned char)arg->value;
            return reg;
        case global_a:
            return &stack[0 + arg->value];            /* globals start at index 0 */
        case local_a:
            return &stack[top + 1 + arg->value];      /* ‘top’ is the current top‐of‐stack */
        case formal_a:
            return &stack[topsp - 1 - arg->value];    /* ‘topsp’ is where formals begin */        
        case retval_a:
            return retval_reg;
        case label_a:
            reg->type = number_m;
            reg->data.numVal = (double)arg->value;
            return reg;
        default:
            avm_error("Invalid operand type %d in avm_translate_operand()", arg->type);
            return NULL;
    }
}

/* ---------- Built‐in Library Functions Registration ---------- */
void avm_registerlibfunc(const char *name, library_func_t ptr) {
    if (num_registered_libfuncs >= MAX_LIBFUNCS)
        avm_error("Too many library functions registered!");
    libfunc_names_arr[num_registered_libfuncs] = strdup(name);
    libfunc_ptrs_arr[num_registered_libfuncs] = ptr;
    num_registered_libfuncs++;
}

void avm_calllibfunc(const char *name) {
    for (unsigned i = 0; i < num_registered_libfuncs; ++i) {
        if (strcmp(name, libfunc_names_arr[i]) == 0) {
            libfunc_ptrs_arr[i]();
            return;
        }
    }
    avm_error("Unknown library function '%s'", name);
}

void libfunc_typeof(void) {
    avm_memcell *arg = &stack[topsp + 1];
    if (!arg) { avm_error("typeof: missing argument"); return; }
    const char *res = NULL;
    switch (arg->type) {
        case number_m:  res = "number";   break;
        case string_m:  res = "string";   break;
        case bool_m:    res = "bool";     break;
        case table_m:   res = "table";    break;
        case userfunc_m:res = "userfunc"; break;
        case libfunc_m: res = "libfunc";  break;
        case nil_m:     res = "nil";      break;
        case undef_m:   res = "undef";    break;
        default:        res = "unknown";  break;
    }
    /* Push return value into retval_reg */
    retval_reg->type = string_m;
    retval_reg->data.strVal = strdup(res);
}

void libfunc_print(void) {
    avm_memcell *nCell = &stack[topsp];

    // Check if the first cell is a number and a valid count
    if (nCell->type != number_m || 
        nCell->data.numVal < 0 || 
        nCell->data.numVal > STACK_SIZE - topsp - 1) {
        avm_error("print: invalid number of arguments (%g)", nCell->data.numVal);
        retval_reg->type = nil_m;
        return;
    }

    unsigned n = (unsigned) nCell->data.numVal;

    for (unsigned i = 1; i <= n; ++i) {
        avm_memcell *arg = &stack[topsp + i];
        char *s = avm_tostring(arg);
        fputs(s, stdout);
        if (i < n) putchar(' ');
    }

    putchar('\n');

    retval_reg->type = nil_m;  // print returns nil
    top = topsp;               // pop args off the stack
}

void libfunc_totalarguments(void) {
    if (pc == 0) {
        avm_error("totalarguments: called outside any function");
        retval_reg->type = nil_m;
        return;
    }
    double n = 0;
    if (stack[topsp].type == number_m) {
        n = stack[topsp].data.numVal;
    }
    retval_reg->type = number_m;
    retval_reg->data.numVal = n;
}

void libfunc_argument(void) {
    avm_memcell *idxCell = &stack[topsp + 1];
    if (idxCell->type != number_m) {
        avm_error("argument: index is not a number");
        retval_reg->type = nil_m;
        return;
    }
    int idx = (int)idxCell->data.numVal;
    if (idx < 0) {
        retval_reg->type = nil_m;
        return;
    }
    /* The caller’s actual arguments begin at TOPSP+2. */
    avm_memcell *argCell = &stack[topsp + 2 + idx];
    if (argCell->type == undef_m || argCell->type == nil_m) {
        retval_reg->type = nil_m;
    } else {
        /* Copy the value into retval_reg */
        *retval_reg = *argCell;
        if (argCell->type == string_m)
            retval_reg->data.strVal = strdup(argCell->data.strVal);
        else if (argCell->type == table_m)
            avm_table_inc_ref(argCell->data.tableVal), retval_reg->data.tableVal = argCell->data.tableVal;
    }
}

void execute_PUSHARG(instruction *instr) {
    avm_memcell *source = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-1]);
    if (!source) {
        avm_error("PUSHARG: cannot translate argument");
        return;
    }
    stack[top--] = *source;
}

/* ---------- Function‐Call Mechanics   ---------- */
void execute_CALLFUNC(instruction *instr) {
    avm_memcell *funcCell = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-1]);
    if (!funcCell) avm_error("CALLFUNC: cannot translate operand");

    if (funcCell->type == userfunc_m) {
        /* -- existing userfunc handling unchanged -- */
        /* push old_topsp, old_top, ret_addr; set topsp; jump... */
    }
    else if (funcCell->type == libfunc_m) {
        /* Library-call frame: stack[topsp] == arg-count */
        unsigned saved_topsp = topsp;
        topsp = top + 1;                         /* first param is the count */
        avm_calllibfunc(funcCell->data.libfuncVal);
        topsp = saved_topsp;                     /* restore previous frame */
        return;
    }
    else {
        avm_error("CALLFUNC: trying to call a non-function type '%s'", avm_tostring(funcCell));
    }
}

void execute_GETRET(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &stack[STACK_SIZE-2]);
    if (!lv) avm_error("GETRET: cannot translate LHS");
    /* Copy retval_reg into lv */
    *lv = *retval_reg;
    if (retval_reg->type == string_m)
        lv->data.strVal = strdup(retval_reg->data.strVal);
    else if (retval_reg->type == table_m)
        avm_table_inc_ref(retval_reg->data.tableVal), lv->data.tableVal = retval_reg->data.tableVal;
}

void execute_FUNCENTER(instruction *instr) {
    avm_memcell *numLocalsCell = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-3]);
    if (numLocalsCell->type != number_m)
        avm_error("FUNCENTER: #locals is not a number");
    unsigned numLocals = (unsigned)numLocalsCell->data.numVal;
    for (unsigned i = 0; i < numLocals; ++i) {
        stack[top--].type = undef_m;
    }
}

void execute_FUNCEXIT(instruction *instr) {
    top = topsp;
    double ret_addr = stack[top].data.numVal;    /* old return address */
    double old_top  = stack[top+1].data.numVal;
    double old_tops = stack[top+2].data.numVal;
    top   = (unsigned)old_top;
    topsp = (unsigned)old_tops;
    pc    = (unsigned)ret_addr;
}

/* ---------- Arithmetic / Table / Assign / NOP ---------- */

void execute_ADD(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &stack[STACK_SIZE-3]);
    avm_memcell *r  = avm_translate_operand(&instr->arg1,   &stack[STACK_SIZE-2]);
    avm_memcell *l  = avm_translate_operand(&instr->arg2,   &stack[STACK_SIZE-1]);
    if (l->type != number_m || r->type != number_m)
        avm_error("ADD: non‐numeric operands '%s' + '%s'", avm_tostring(l), avm_tostring(r));
    lv->type = number_m;
    lv->data.numVal = l->data.numVal + r->data.numVal;
}

void execute_SUB(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &stack[STACK_SIZE-3]);
    avm_memcell *r  = avm_translate_operand(&instr->arg1,   &stack[STACK_SIZE-2]);
    avm_memcell *l  = avm_translate_operand(&instr->arg2,   &stack[STACK_SIZE-1]);
    if (l->type != number_m || r->type != number_m)
        avm_error("SUB: non‐numeric operands '%s' - '%s'", avm_tostring(l), avm_tostring(r));
    lv->type = number_m;
    lv->data.numVal = l->data.numVal - r->data.numVal;
}

void execute_MUL(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &stack[STACK_SIZE-3]);
    avm_memcell *r  = avm_translate_operand(&instr->arg1,   &stack[STACK_SIZE-2]);
    avm_memcell *l  = avm_translate_operand(&instr->arg2,   &stack[STACK_SIZE-1]);
    if (l->type != number_m || r->type != number_m)
        avm_error("MUL: non‐numeric operands '%s' * '%s'", avm_tostring(l), avm_tostring(r));
    lv->type = number_m;
    lv->data.numVal = l->data.numVal * r->data.numVal;
}

void execute_DIV(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &stack[STACK_SIZE-3]);
    avm_memcell *r  = avm_translate_operand(&instr->arg1,   &stack[STACK_SIZE-2]);
    avm_memcell *l  = avm_translate_operand(&instr->arg2,   &stack[STACK_SIZE-1]);
    if (l->type != number_m || r->type != number_m)
        avm_error("DIV: non‐numeric operands '%s' / '%s'", avm_tostring(l), avm_tostring(r));
    if (r->data.numVal == 0)
        avm_error("DIV: division by zero");
    lv->type = number_m;
    lv->data.numVal = l->data.numVal / r->data.numVal;
}

void execute_MOD(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &stack[STACK_SIZE-3]);
    // avm_memcell *r  = avm_translate_operand(&instr->arg1,   &stack[STACK_SIZE-2]);
    // avm_memcell *l  = avm_translate_operand(&instr->arg2,   &stack[STACK_SIZE-1]);
    avm_memcell *l  = avm_translate_operand(&instr->arg1,   &stack[STACK_SIZE-2]);
    avm_memcell *r  = avm_translate_operand(&instr->arg2,   &stack[STACK_SIZE-1]);
    if (l->type != number_m || r->type != number_m)
        avm_error("MOD: non‐numeric operands '%s' %% '%s'", avm_tostring(l), avm_tostring(r));
    if (r->data.numVal == 0)
        avm_error("MOD: modulo by zero");
    lv->type = number_m;
    lv->data.numVal = fmod(l->data.numVal, r->data.numVal);
}

void execute_ASSIGN(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &stack[STACK_SIZE-2]);
    avm_memcell *rv = avm_translate_operand(&instr->arg1,   &stack[STACK_SIZE-1]);
    if (lv == rv) return;
    if (lv->type == string_m && lv->data.strVal) {
        free(lv->data.strVal);
    }
    if (lv->type == table_m && lv->data.tableVal) {
        avm_table_dec_ref(lv->data.tableVal);
    }
    *lv = *rv;
    if (rv->type == string_m && rv->data.strVal)
        lv->data.strVal = strdup(rv->data.strVal);
    else if (rv->type == table_m) {
        avm_table_inc_ref(rv->data.tableVal);
        lv->data.tableVal = rv->data.tableVal;
    }
}

void execute_NEWTABLE(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &stack[STACK_SIZE-1]);
    lv->type = table_m;
    lv->data.tableVal = avm_table_new();
    avm_table_inc_ref(lv->data.tableVal);
}

void execute_TABLEGETELM(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &stack[STACK_SIZE-1]);
    avm_memcell *tbl = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-2]);
    avm_memcell *key = avm_translate_operand(&instr->arg2, &stack[STACK_SIZE-3]);
    if (tbl->type != table_m)
        avm_error("TABLEGETELM: first operand is not a table '%s'", avm_tostring(tbl));
    avm_memcell *cell = avm_tablegetelem(tbl->data.tableVal, key);
    if (!cell) {
        lv->type = nil_m;
    } else {
        *lv = *cell;
        if (cell->type == string_m && cell->data.strVal)
            lv->data.strVal = strdup(cell->data.strVal);
        else if (cell->type == table_m)
            avm_table_inc_ref(cell->data.tableVal), lv->data.tableVal = cell->data.tableVal;
    }
}

void execute_TABLESETELEM(instruction *instr) {
    avm_memcell *tbl = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-3]);
    avm_memcell *key = avm_translate_operand(&instr->arg2, &stack[STACK_SIZE-2]);
    avm_memcell *val = avm_translate_operand(&instr->result, &stack[STACK_SIZE-1]);
    if (tbl->type != table_m)
        avm_error("TABLESETELEM: first operand is not a table '%s'", avm_tostring(tbl));
    avm_tablesetelem(tbl->data.tableVal, key, val);
}

void execute_NOP(instruction *instr) {
    /* do nothing */
}

#define CHECK_NUMERIC_OP(l, r, op) \
    if ((l)->type != number_m || (r)->type != number_m) \
        avm_error("%s: non‐numeric operands '%s' and '%s'", op, avm_tostring(l), avm_tostring(r))

void execute_JEQ(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-2]);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &stack[STACK_SIZE-1]);
    int result = 0;
    if (l->type == nil_m && r->type == nil_m) result = 1;
    else if (l->type == bool_m && r->type == bool_m) result = (l->data.boolVal == r->data.boolVal);
    else if (l->type == number_m && r->type == number_m) result = (l->data.numVal == r->data.numVal);
    else if (l->type == string_m && r->type == string_m) result = (strcmp(l->data.strVal, r->data.strVal) == 0);
    else if (l->type == table_m && r->type == table_m) result = (l->data.tableVal == r->data.tableVal);
    else if (l->type == userfunc_m && r->type == userfunc_m) result = (l->data.funcVal == r->data.funcVal);
    else if (l->type == libfunc_m && r->type == libfunc_m) result = (strcmp(l->data.libfuncVal, r->data.libfuncVal) == 0);
    if (!result) pc = instr->result.value - 1; /* skip to label if false */
}

void execute_JNE(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-2]);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &stack[STACK_SIZE-1]);
    int result = 0;
    if (l->type == nil_m && r->type == nil_m) result = 1;
    else if (l->type == bool_m && r->type == bool_m) result = (l->data.boolVal == r->data.boolVal);
    else if (l->type == number_m && r->type == number_m) result = (l->data.numVal == r->data.numVal);
    else if (l->type == string_m && r->type == string_m) result = (strcmp(l->data.strVal, r->data.strVal) == 0);
    else if (l->type == table_m && r->type == table_m) result = (l->data.tableVal == r->data.tableVal);
    else if (l->type == userfunc_m && r->type == userfunc_m) result = (l->data.funcVal == r->data.funcVal);
    else if (l->type == libfunc_m && r->type == libfunc_m) result = (strcmp(l->data.libfuncVal, r->data.libfuncVal) == 0);
    if (result) pc = instr->result.value - 1; /* skip if equal */
}

void execute_JLE(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-2]);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &stack[STACK_SIZE-1]);
    CHECK_NUMERIC_OP(l, r, "JLE");
    if (l->data.numVal > r->data.numVal) pc = instr->result.value - 1;
}

void execute_JLT(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-2]);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &stack[STACK_SIZE-1]);
    CHECK_NUMERIC_OP(l, r, "JLT");
    if (l->data.numVal >= r->data.numVal) pc = instr->result.value - 1;
}

void execute_JGE(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-2]);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &stack[STACK_SIZE-1]);
    CHECK_NUMERIC_OP(l, r, "JGE");
    if (l->data.numVal < r->data.numVal) pc = instr->result.value - 1;
}

void execute_JGT(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-2]);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &stack[STACK_SIZE-1]);
    CHECK_NUMERIC_OP(l, r, "JGT");
    if (l->data.numVal <= r->data.numVal) pc = instr->result.value - 1;
}

void execute_AND(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-2]);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &stack[STACK_SIZE-1]);
    if (l->type != bool_m || r->type != bool_m)
        avm_error("AND: non‐bool operands '%s' and '%s'", avm_tostring(l), avm_tostring(r));
    if (!(l->data.boolVal && r->data.boolVal)) pc = instr->result.value - 1;
}

void execute_OR(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-2]);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &stack[STACK_SIZE-1]);
    if (l->type != bool_m || r->type != bool_m)
        avm_error("OR: non‐bool operands '%s' and '%s'", avm_tostring(l), avm_tostring(r));
    if (!(l->data.boolVal || r->data.boolVal)) pc = instr->result.value - 1;
}

void execute_NOT(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &stack[STACK_SIZE-1]);
    if (l->type != bool_m)
        avm_error("NOT: non‐bool operand '%s'", avm_tostring(l));
    if (!l->data.boolVal) pc = instr->result.value - 1;
}

void execute_NEG(instruction *instr) {
    static avm_memcell lv_temp, rv_temp;
    avm_memcell *lv = avm_translate_operand(&instr->result, &lv_temp);
    avm_memcell *rv = avm_translate_operand(&instr->arg1, &rv_temp);
    if (rv->type != number_m) {
        avm_error("NEG: operand is not a number '%s'", avm_tostring(rv));
        return;
    }
    lv->type = number_m;
    lv->data.numVal = -rv->data.numVal;
}

static void load_numConsts(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) avm_error("Cannot open %s", filename);
    uint32_t count;
    fread(&count, sizeof(count), 1, fp);
    total_const_nums = count;
    const_nums = malloc(count * sizeof(double));
    fread(const_nums, sizeof(double), count, fp);
    fclose(fp);
}

static void load_stringConsts(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) avm_error("Cannot open %s", filename);
    uint32_t count;
    fread(&count, sizeof(count), 1, fp);
    total_const_strs = count;
    const_strs = malloc(count * sizeof(char*));
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t len;
        fread(&len, sizeof(len), 1, fp);
        char *buf = malloc(len+1);
        fread(buf, sizeof(char), len, fp);
        buf[len] = '\0';
        const_strs[i] = buf;
    }
    fclose(fp);
}

static void load_instructions(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) avm_error("Cannot open %s", filename);
    uint32_t instr_count;
    fread(&instr_count, sizeof(instr_count), 1, fp);
    total_instructions = instr_count;
    code = malloc(instr_count * sizeof(instruction));
    fread(code, sizeof(instruction), instr_count, fp);
    fclose(fp);
}

static void register_libfuncs(void) {
    avm_registerlibfunc("typeof",         libfunc_typeof);
    avm_registerlibfunc("totalarguments", libfunc_totalarguments);
    avm_registerlibfunc("argument",       libfunc_argument);
    avm_registerlibfunc("print", libfunc_print);
}

void vm_init(void) {
    load_numConsts("out_numConsts.bin");
    load_stringConsts("out_stringConsts.bin");
    load_instructions("out_instructions.bin");

    /* Initialize everything to undef */
    for (unsigned i = 0; i < STACK_SIZE; ++i)
        stack[i].type = undef_m;
    top   = STACK_SIZE - 1;
    topsp = 0;
    pc    = 0;

    /* A single dedicated “retval” register */
    retval_reg = malloc(sizeof(avm_memcell));
    retval_reg->type = undef_m;

    /* Register all built-in library functions: */
    register_libfuncs();

    for (unsigned i = 0; i < num_registered_libfuncs; ++i) {
        avm_memcell *cell = &stack[0 + i];
        cell->type = libfunc_m;
        // strdup so the VM’s tostring() logic will pick up the name
        cell->data.libfuncVal = strdup(libfunc_names_arr[i]);
    }
}

bool pointer_in_array(void **arr, unsigned size, void *ptr) {
    for (unsigned i = 0; i < size; ++i) {
        if (arr[i] == ptr) return true;
    }
    return false;
}

void avm_destroy(void) {
    void *freed_ptrs[total_const_strs + STACK_SIZE];    // to store freed pointers
    unsigned freed_count = 0;

    for (unsigned i = 0; i < total_const_strs; ++i) {
        if (const_strs[i]) {
            free(const_strs[i]);                        // free const_strs string
            freed_ptrs[freed_count++] = const_strs[i];  // record freed pointers
            const_strs[i] = NULL;
        }
    }
    free(const_strs);
    free(const_nums);
    free(code);

    // now we free stack strings only if not freed before
    for (unsigned i = 0; i < STACK_SIZE; ++i) {
        if (stack[i].type == table_m && stack[i].data.tableVal) {
            avm_table_dec_ref(stack[i].data.tableVal);
            stack[i].data.tableVal = NULL;
        }
        else if (stack[i].type == string_m && stack[i].data.strVal) {
            if (!pointer_in_array(freed_ptrs, freed_count, stack[i].data.strVal)) {
                free(stack[i].data.strVal);
                freed_ptrs[freed_count++] = stack[i].data.strVal;
            }
            stack[i].data.strVal = NULL;
        }
    }

    free(retval_reg);

    for (unsigned i = 0; i < num_userfuncs; ++i) {
        free(userfuncs[i].name);
    }

    for (unsigned i = 0; i < num_registered_libfuncs; ++i){
        free((char*)libfunc_names_arr[i]);
    }
}

void vm_run(void) {
    while (pc < total_instructions) {
        instruction *instr = &code[pc++];
        switch (instr->opcode) {
            case op_add:            execute_ADD(instr);           break;
            case op_sub:            execute_SUB(instr);           break;
            case op_mul:            execute_MUL(instr);           break;
            case op_div:            execute_DIV(instr);           break;
            case op_mod:            execute_MOD(instr);           break;
            case op_newtable:       execute_NEWTABLE(instr);      break;
            case op_tablegetelem:   execute_TABLEGETELM(instr);   break;
            case op_tablesetelem:   execute_TABLESETELEM(instr);  break;
            case op_assign:         execute_ASSIGN(instr);        break;
            case op_nop:            execute_NOP(instr);           break;
            case op_jeq:            execute_JEQ(instr);           break;
            case op_jne:            execute_JNE(instr);           break;
            case op_jle:            execute_JLE(instr);           break;
            case op_jlt:            execute_JLT(instr);           break;
            case op_jge:            execute_JGE(instr);           break;
            case op_jgt:            execute_JGT(instr);           break;
            case op_pusharg:        execute_PUSHARG(instr);       break;
            case op_callfunc:       execute_CALLFUNC(instr);      break;
            case op_getretval:      execute_GETRET(instr);        break;
            case op_uminus:         execute_NEG(instr);           break;
            default:
                avm_error("Unknown opcode %d at pc=%u", instr->opcode, pc-1);
        }
    }
}
