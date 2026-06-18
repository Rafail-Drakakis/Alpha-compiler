/**
 * HY-340 Project Phases 4 & 5 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>
#include "vm.h"

/* ---------- Global VM State ---------- */

avm_memcell stack[STACK_SIZE];
static avm_memcell vm_reg[4];
#define AVM_TMP1 vm_reg[0]
#define AVM_TMP2 vm_reg[1]
#define AVM_TMP3 vm_reg[2]
#define AVM_TMP4 vm_reg[3]
#define MAX_EXEC_STEPS  10000000
#define AVM_STACKENV_SIZE 4

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

static unsigned enclosing_topsp = 0;
static unsigned program_var_count = 0;

static void ensure_expr_stack(void) {
    unsigned floor = program_var_count + 8;
    if (top < floor)
        top = STACK_SIZE - 1;
}

static void check_stack_bounds(void) {
    if (top <= program_var_count) {
        fprintf(stderr, "run time error: Stack overflow! (top=%u, globals=%u)\n",
                top, program_var_count);
        exit(EXIT_FAILURE);
    }
}

static void avm_memcell_copy(avm_memcell *dst, avm_memcell *src) {
    *dst = *src;
    if (src->type == string_m && src->data.strVal)
        dst->data.strVal = strdup(src->data.strVal);
    else if (src->type == table_m && src->data.tableVal) {
        dst->data.tableVal = src->data.tableVal;
        avm_table_inc_ref(dst->data.tableVal);
    }
}

void vm_register_userfunc(const char *name, unsigned addr) {
    if (num_userfuncs >= 256) return;
    for (unsigned i = 0; i < num_userfuncs; ++i)
        if (userfuncs[i].addr == addr) return;
    userfuncs[num_userfuncs].name = strdup(name);
    userfuncs[num_userfuncs].addr = addr;
    num_userfuncs++;
}

unsigned totalactuals(void) {
    if (topsp + 3 >= STACK_SIZE) return 0;
    if (stack[topsp + 3].type == number_m)
        return (unsigned)stack[topsp + 3].data.numVal;
    return 0;
}

static const char *opname[] = {
    "ADD","SUB","MUL","DIV","MOD",
    "NEWTABLE","TGET","TSET","ASSIGN","NOP",
    "JEQ","JNE","JGT","JGE","JLT","JLE",
    "PUSHARG","CALLFUNC","GETRET","UMINUS","FUNCENTER","FUNCEXIT"
};

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
void execute_NEG(instruction *instr);
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

/* ---------- VM Execution Helpers ---------- */
static inline int is_unconditional_jump(const vmarg *a1, const vmarg *a2) {
    return a1->type == label_a && a2->type == label_a;
}

static inline double to_number_or_die(avm_memcell *m, const char *opname)
{
    if (m->type == number_m)
        return m->data.numVal;

    if (m->type == bool_m)
        return (double)m->data.boolVal;

    avm_error("%s: operand '%s' is not comparable",
              opname, avm_tostring(m));
    /* avm_error exits, so return is never reached */
    return 0.0;
}
/* ------------------------------------------ */

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
static char *table_tostring_buf = NULL;
static size_t table_tostring_cap = 0;
static size_t table_tostring_len = 0;

static void tt_reset(void) {
    table_tostring_len = 0;
    if (table_tostring_cap == 0) {
        table_tostring_cap = 256;
        table_tostring_buf = malloc(table_tostring_cap);
    }
    if (table_tostring_buf) table_tostring_buf[0] = '\0';
}

static void tt_append(const char *s) {
    size_t slen = strlen(s);
    if (table_tostring_len + slen + 1 > table_tostring_cap) {
        while (table_tostring_len + slen + 1 > table_tostring_cap)
            table_tostring_cap *= 2;
        table_tostring_buf = realloc(table_tostring_buf, table_tostring_cap);
    }
    memcpy(table_tostring_buf + table_tostring_len, s, slen + 1);
    table_tostring_len += slen;
}

static char *format_number(double v) {
    static char nb[64];
    snprintf(nb, sizeof(nb), "%.3f", v);
    char *dot = strchr(nb, '.');
    if (dot) {
        char *end = nb + strlen(nb) - 1;
        while (end > dot && *end == '0') *end-- = '\0';
        if (end == dot) *end = '\0';
    }
    return nb;
}

typedef struct { avm_memcell *key; avm_memcell *value; } kv_pair;
static kv_pair collected[4096];
static unsigned collected_n;

static void collect_bucket_chain(avm_table_bucket *b) {
    while (b && collected_n < 4096) {
        collected[collected_n].key = b->key;
        collected[collected_n].value = b->value;
        collected_n++;
        b = b->next;
    }
}

static void collect_table_members(avm_table *tbl) {
    collected_n = 0;
    if (!tbl) return;
    for (unsigned i = 0; i < TABLE_HASHSIZE; ++i) {
        collect_bucket_chain(tbl->numIndexed[i]);
        collect_bucket_chain(tbl->strIndexed[i]);
        collect_bucket_chain(tbl->userfuncIndexed[i]);
        collect_bucket_chain(tbl->libfuncIndexed[i]);
        collect_bucket_chain(tbl->tableIndexed[i]);
    }
    collect_bucket_chain(tbl->boolIndexed[0]);
    collect_bucket_chain(tbl->boolIndexed[1]);
}

static int is_compact_array(void) {
    if (collected_n == 0) return 1;
    for (unsigned i = 0; i < collected_n; ++i) {
        if (collected[i].key->type != number_m) return 0;
        double k = collected[i].key->data.numVal;
        if (k < 0 || k != (double)(unsigned)k) return 0;
    }
    for (unsigned i = 0; i < collected_n; ++i) {
        unsigned want = i;
        int found = 0;
        for (unsigned j = 0; j < collected_n; ++j) {
            if ((unsigned)collected[j].key->data.numVal == want) { found = 1; break; }
        }
        if (!found) return 0;
    }
    return 1;
}

static avm_memcell *value_at_index(unsigned idx) {
    for (unsigned i = 0; i < collected_n; ++i)
        if (collected[i].key->type == number_m &&
            (unsigned)collected[i].key->data.numVal == idx)
            return collected[i].value;
    return NULL;
}

static void table_tostring_rec(avm_table *tbl);

static int table_single_userfunc(avm_table *tbl, unsigned *funcAddr) {
    collect_table_members(tbl);
    unsigned found = 0;
    unsigned addr = 0;
    for (unsigned i = 0; i < collected_n; ++i) {
        if (collected[i].value->type != userfunc_m)
            continue;
        if (found)
            return 0;
        addr = collected[i].value->data.funcVal;
        found = 1;
    }
    if (!found)
        return 0;
    *funcAddr = addr;
    return 1;
}

static void cell_tostring_rec(avm_memcell *m) {
    if (!m) { tt_append("nil"); return; }
    switch (m->type) {
        case number_m:
            tt_append(format_number(m->data.numVal));
            break;
        case string_m:
            tt_append(m->data.strVal ? m->data.strVal : "");
            break;
        case bool_m:
            tt_append(m->data.boolVal ? "true" : "false");
            break;
        case nil_m:
            tt_append("nil");
            break;
        case undef_m:
            tt_append("undef");
            break;
        case table_m:
            table_tostring_rec(m->data.tableVal);
            break;
        case userfunc_m: {
            for (unsigned i = 0; i < num_userfuncs; ++i)
                if (userfuncs[i].addr == m->data.funcVal) {
                    tt_append(userfuncs[i].name);
                    tt_append("()");
                    return;
                }
            tt_append("<userfunc:unknown>");
            break;
        }
        case libfunc_m:
            tt_append("lib::");
            tt_append(m->data.libfuncVal ? m->data.libfuncVal : "null");
            break;
        default:
            tt_append("<invalid>");
            break;
    }
}

static void table_tostring_rec(avm_table *tbl) {
    collect_table_members(tbl);
    tt_append("[ ");
    if (is_compact_array()) {
        for (unsigned i = 0; i < collected_n; ++i) {
            avm_memcell *val = value_at_index(i);
            if (i) tt_append(", ");
            if (val) cell_tostring_rec(val);
            else tt_append("nil");
        }
    } else {
        for (unsigned i = 0; i < collected_n; ++i) {
            if (i) tt_append(", ");
            tt_append("{ ");
            cell_tostring_rec(collected[i].key);
            tt_append(" : ");
            cell_tostring_rec(collected[i].value);
            tt_append(" }");
        }
    }
    tt_append(", ]");
}

char *avm_tostring(avm_memcell *m) {
    static char buffer[256];
    if (!m) return "nil";
    switch (m->type) {
        case number_m:
            return format_number(m->data.numVal);
        case string_m:
            return m->data.strVal ? m->data.strVal : "";
        case bool_m:
            return m->data.boolVal ? "true" : "false";
        case nil_m:
            return "nil";
        case undef_m:
            return "undef";
        case table_m:
            tt_reset();
            table_tostring_rec(m->data.tableVal);
            return table_tostring_buf ? table_tostring_buf : "[ ]";
        case userfunc_m: {
            for (unsigned i = 0; i < num_userfuncs; ++i)
                if (userfuncs[i].addr == m->data.funcVal) {
                    snprintf(buffer, sizeof(buffer), "%s()", userfuncs[i].name);
                    return buffer;
                }
            return "<userfunc:unknown>";
        }
        case libfunc_m: {
            snprintf(buffer, sizeof(buffer), "lib::%s",
                     m->data.libfuncVal ? m->data.libfuncVal : "null");
            return buffer;
        }
        default:
            return "<invalid cell>";
    }
}

/* ---------- Reference Counting for Tables ---------- */
void avm_table_inc_ref(avm_table *tbl) {
    if (!tbl) return;
    tbl->refCounter++;
}

static void free_bucket_chain(avm_table_bucket *b) {
    while (b) {
        avm_table_bucket *next = b->next;
        if (b->key) {
            if (b->key->type == string_m && b->key->data.strVal)
                free(b->key->data.strVal);
            if (b->key->type == libfunc_m && b->key->data.libfuncVal)
                free(b->key->data.libfuncVal);
            free(b->key);
        }
        if (b->value) {
            if (b->value->type == table_m && b->value->data.tableVal)
                avm_table_dec_ref(b->value->data.tableVal);
            if (b->value->type == string_m && b->value->data.strVal)
                free(b->value->data.strVal);
            free(b->value);
        }
        free(b);
        b = next;
    }
}

void avm_table_dec_ref(avm_table *tbl) {
    if (!tbl) return;
    if (--tbl->refCounter == 0) {
        for (unsigned i = 0; i < TABLE_HASHSIZE; ++i) {
            free_bucket_chain(tbl->numIndexed[i]);
            free_bucket_chain(tbl->strIndexed[i]);
            free_bucket_chain(tbl->userfuncIndexed[i]);
            free_bucket_chain(tbl->libfuncIndexed[i]);
            free_bucket_chain(tbl->tableIndexed[i]);
        }
        free_bucket_chain(tbl->boolIndexed[0]);
        free_bucket_chain(tbl->boolIndexed[1]);
        free(tbl);
    }
}

avm_table* avm_table_new(void) {
    avm_table *tbl = malloc(sizeof(avm_table));
    tbl->refCounter = 0;
    for (unsigned i = 0; i < TABLE_HASHSIZE; ++i) {
        tbl->numIndexed[i] = NULL;
        tbl->strIndexed[i] = NULL;
        tbl->userfuncIndexed[i] = NULL;
        tbl->libfuncIndexed[i] = NULL;
        tbl->tableIndexed[i] = NULL;
    }
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
static unsigned hash_userfunc(unsigned funcVal) {
    return (unsigned)(funcVal % TABLE_HASHSIZE);
}
static unsigned hash_libfunc(const char *name) {
    return hash_string(name);
}
static unsigned hash_table_ptr(avm_table *tbl) {
    uintptr_t v = (uintptr_t)tbl;
    return (unsigned)((v ^ (v >> 16)) % TABLE_HASHSIZE);
}

static int table_keys_equal(avm_memcell *key, avm_memcell *bkey) {
    if (key->type != bkey->type) return 0;
    switch (key->type) {
        case number_m:
            return bkey->data.numVal == key->data.numVal;
        case string_m:
            return strcmp(bkey->data.strVal, key->data.strVal) == 0;
        case bool_m:
            return bkey->data.boolVal == key->data.boolVal;
        case userfunc_m:
            return bkey->data.funcVal == key->data.funcVal;
        case libfunc_m:
            return strcmp(bkey->data.libfuncVal, key->data.libfuncVal) == 0;
        case table_m:
            return bkey->data.tableVal == key->data.tableVal;
        default:
            return 0;
    }
}

static avm_table_bucket **table_bucket_head(avm_table *tbl, avm_memcell *key) {
    if (key->type == number_m)
        return &tbl->numIndexed[hash_number(key->data.numVal)];
    if (key->type == string_m)
        return &tbl->strIndexed[hash_string(key->data.strVal)];
    if (key->type == bool_m)
        return &tbl->boolIndexed[hash_bool(key->data.boolVal)];
    if (key->type == userfunc_m)
        return &tbl->userfuncIndexed[hash_userfunc(key->data.funcVal)];
    if (key->type == libfunc_m)
        return &tbl->libfuncIndexed[hash_libfunc(key->data.libfuncVal)];
    if (key->type == table_m)
        return &tbl->tableIndexed[hash_table_ptr(key->data.tableVal)];
    return NULL;
}

static void table_key_copy(avm_memcell *dst, avm_memcell *src) {
    *dst = *src;
    if (src->type == string_m)
        dst->data.strVal = strdup(src->data.strVal);
    else if (src->type == libfunc_m)
        dst->data.libfuncVal = strdup(src->data.libfuncVal);
}

static void table_value_copy(avm_memcell *dst, avm_memcell *src) {
    *dst = *src;
    if (src->type == string_m)
        dst->data.strVal = strdup(src->data.strVal);
    if (src->type == table_m) {
        dst->data.tableVal = src->data.tableVal;
        avm_table_inc_ref(dst->data.tableVal);
    }
}

/* ---------- Table get/set element ---------- */
static avm_memcell* avm_tablegetelem_chain(avm_table_bucket *b, avm_memcell *key) {
    for (; b; b = b->next) {
        if (table_keys_equal(key, b->key))
            return b->value;
    }
    return NULL;
}

avm_memcell* avm_tablegetelem(avm_table *tbl, avm_memcell *key) {
    if (!tbl || !key) return NULL;
    avm_table_bucket **head = table_bucket_head(tbl, key);
    if (!head) return NULL;
    avm_memcell *found = avm_tablegetelem_chain(*head, key);
    if (found) return found;
    if (key->type == libfunc_m && key->data.libfuncVal) {
        avm_memcell strkey = { .type = string_m, .data.strVal = key->data.libfuncVal };
        head = table_bucket_head(tbl, &strkey);
        if (head) return avm_tablegetelem_chain(*head, &strkey);
    }
    return NULL;
}

void avm_tablesetelem(avm_table *tbl, avm_memcell *key, avm_memcell *value) {
    if (!tbl || !key) return;
    avm_table_bucket **head = table_bucket_head(tbl, key);
    if (!head) return;

    if (value->type == nil_m) {
        avm_table_bucket *prev = NULL, *cur = *head;
        while (cur) {
            if (table_keys_equal(key, cur->key)) {
                if (prev) prev->next = cur->next;
                else *head = cur->next;
                if (cur->value && cur->value->type == table_m)
                    avm_table_dec_ref(cur->value->data.tableVal);
                if (cur->value && cur->value->type == string_m && cur->value->data.strVal)
                    free(cur->value->data.strVal);
                free(cur->value);
                if (cur->key->type == string_m && cur->key->data.strVal)
                    free(cur->key->data.strVal);
                if (cur->key->type == libfunc_m && cur->key->data.libfuncVal)
                    free(cur->key->data.libfuncVal);
                free(cur->key);
                free(cur);
                return;
            }
            prev = cur;
            cur = cur->next;
        }
        return;
    }

    for (avm_table_bucket *cur = *head; cur; cur = cur->next) {
        if (table_keys_equal(key, cur->key)) {
            if (cur->value->type == table_m)
                avm_table_dec_ref(cur->value->data.tableVal);
            if (cur->value->type == string_m && cur->value->data.strVal)
                free(cur->value->data.strVal);
            free(cur->value);
            cur->value = malloc(sizeof(avm_memcell));
            table_value_copy(cur->value, value);
            return;
        }
    }

    avm_table_bucket *newb = malloc(sizeof(avm_table_bucket));
    newb->key = malloc(sizeof(avm_memcell));
    table_key_copy(newb->key, key);
    newb->value = malloc(sizeof(avm_memcell));
    table_value_copy(newb->value, value);
    newb->next = *head;
    *head = newb;
}

static avm_memcell *retval_reg = NULL;

avm_memcell* avm_translate_operand(vmarg *arg, avm_memcell *reg)
{
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

        case userfunc_a:
            reg->type = userfunc_m;
            reg->data.funcVal = (unsigned)arg->value;
            return reg;

        case nil_a:                        
            reg->type = nil_m;
            return reg;

        case global_a:
            return &stack[arg->value];            /* globals start at 0 */

        case local_a:
            if (topsp < arg->value + 1)
                avm_error("Invalid local offset %d (topsp=%u)", arg->value, topsp);
            return &stack[topsp - arg->value - 1];

        case formal_a: {
            unsigned nargs = totalactuals();
            if (arg->value >= nargs) {
                fprintf(stderr, "run time error: Stack overflow!\n");
                exit(EXIT_FAILURE);
            }
            return &stack[topsp + AVM_STACKENV_SIZE + arg->value];
        }

        case retval_a:
            return retval_reg;

        case label_a:
            reg->type       = number_m;
            reg->data.numVal= (double)arg->value;
            return reg;

        default:
            avm_error("Invalid operand type %d in avm_translate_operand()",
                      arg->type);
            return NULL;          /* never reached – avm_error exits */
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

static avm_memcell *libfunc_getarg(unsigned i) {
    return &stack[topsp + 1 + i];
}

void libfunc_typeof(void) {
    avm_memcell *arg = libfunc_getarg(0);
    const char *res = NULL;
    switch (arg->type) {
        case number_m:  res = "number";          break;
        case string_m:  res = "string";          break;
        case bool_m:    res = "boolean";         break;
        case table_m:   res = "table";           break;
        case userfunc_m:res = "userfunction";    break;
        case libfunc_m: res = "libraryfunction"; break;
        case nil_m:     res = "nil";             break;
        case undef_m:   res = "undef";           break;
        default:        res = "unknown";         break;
    }
    retval_reg->type = string_m;
    retval_reg->data.strVal = strdup(res);
}

void libfunc_print(void) {
    avm_memcell *nCell = &stack[topsp];
    if (nCell->type != number_m ||
        nCell->data.numVal < 0 ||
        topsp + (unsigned)nCell->data.numVal >= STACK_SIZE) {
        avm_error("print: invalid number of arguments (%g)", nCell->data.numVal);
    }
    unsigned n = (unsigned)nCell->data.numVal;
    for (unsigned i = 1; i <= n; ++i) {
        fputs(avm_tostring(&stack[topsp + i]), stdout);
        if (i < n) putchar(' ');
    }
    putchar('\n');
    retval_reg->type = nil_m;
}

void libfunc_totalarguments(void) {
    if (enclosing_topsp + 3 >= STACK_SIZE ||
        stack[enclosing_topsp].type != number_m ||
        stack[enclosing_topsp + 3].type != number_m) {
        retval_reg->type = nil_m;
        return;
    }
    retval_reg->type = number_m;
    retval_reg->data.numVal = stack[enclosing_topsp + 3].data.numVal;
}

void libfunc_argument(void) {
    avm_memcell *idxCell = libfunc_getarg(0);
    if (idxCell->type != number_m) {
        avm_error("argument: index is not a number");
    }
    int idx = (int)idxCell->data.numVal;
    if (idx < 0) {
        retval_reg->type = nil_m;
        return;
    }
    unsigned nargs = 0;
    if (enclosing_topsp + 3 < STACK_SIZE &&
        stack[enclosing_topsp + 3].type == number_m) {
        nargs = (unsigned)stack[enclosing_topsp + 3].data.numVal;
    }
    if ((unsigned)idx >= nargs) {
        retval_reg->type = nil_m;
        return;
    }
    unsigned frame = enclosing_topsp;
    avm_memcell *argCell = &stack[frame + AVM_STACKENV_SIZE + (unsigned)idx];
    if (argCell->type == undef_m || argCell->type == nil_m) {
        retval_reg->type = nil_m;
    } else {
        avm_memcell_copy(retval_reg, argCell);
    }
}

static unsigned count_table_members(avm_table *tbl) {
    unsigned n = 0;
    collect_table_members(tbl);
    return collected_n;
}

void libfunc_objecttotalmembers(void) {
    avm_memcell *arg = libfunc_getarg(0);
    if (arg->type != table_m) {
        retval_reg->type = nil_m;
        return;
    }
    retval_reg->type = number_m;
    retval_reg->data.numVal = (double)count_table_members(arg->data.tableVal);
}

void libfunc_objectmemberkeys(void) {
    avm_memcell *arg = libfunc_getarg(0);
    if (arg->type != table_m) {
        retval_reg->type = nil_m;
        return;
    }
    avm_table *keys_tbl = avm_table_new();
    avm_table_inc_ref(keys_tbl);
    collect_table_members(arg->data.tableVal);
    for (unsigned i = 0; i < collected_n; ++i) {
        avm_memcell idx = { .type = number_m, .data.numVal = (double)i };
        avm_tablesetelem(keys_tbl, &idx, collected[i].key);
    }
    retval_reg->type = table_m;
    retval_reg->data.tableVal = keys_tbl;
}

static void copy_bucket_chain(avm_table *dst, avm_table_bucket *b) {
    while (b) {
        avm_tablesetelem(dst, b->key, b->value);
        b = b->next;
    }
}

void libfunc_objectcopy(void) {
    avm_memcell *arg = libfunc_getarg(0);
    if (arg->type != table_m) {
        retval_reg->type = nil_m;
        return;
    }
    avm_table *src = arg->data.tableVal;
    avm_table *dst = avm_table_new();
    avm_table_inc_ref(dst);
    for (unsigned i = 0; i < TABLE_HASHSIZE; ++i) {
        copy_bucket_chain(dst, src->numIndexed[i]);
        copy_bucket_chain(dst, src->strIndexed[i]);
        copy_bucket_chain(dst, src->userfuncIndexed[i]);
        copy_bucket_chain(dst, src->libfuncIndexed[i]);
        copy_bucket_chain(dst, src->tableIndexed[i]);
    }
    copy_bucket_chain(dst, src->boolIndexed[0]);
    copy_bucket_chain(dst, src->boolIndexed[1]);
    retval_reg->type = table_m;
    retval_reg->data.tableVal = dst;
}

void libfunc_strtonum(void) {
    avm_memcell *arg = libfunc_getarg(0);
    if (arg->type != string_m || !arg->data.strVal) {
        fprintf(stderr, "Failed to convert parameter to number, in strtonum\n");
        retval_reg->type = nil_m;
        return;
    }
    char *end = NULL;
    double v = strtod(arg->data.strVal, &end);
    while (end && *end == ' ') end++;
    if (end == arg->data.strVal || *end != '\0') {
        fprintf(stderr, "Failed to convert parameter to number, in strtonum\n");
        retval_reg->type = nil_m;
        return;
    }
    retval_reg->type = number_m;
    retval_reg->data.numVal = v;
}

void libfunc_sqrt(void) {
    avm_memcell *arg = libfunc_getarg(0);
    if (arg->type != number_m || arg->data.numVal < 0) {
        retval_reg->type = nil_m;
        return;
    }
    retval_reg->type = number_m;
    retval_reg->data.numVal = sqrt(arg->data.numVal);
}

void libfunc_cos(void) {
    avm_memcell *arg = libfunc_getarg(0);
    if (arg->type != number_m) { retval_reg->type = nil_m; return; }
    retval_reg->type = number_m;
    retval_reg->data.numVal = cos(arg->data.numVal * 3.141592653589793 / 180.0);
}

void libfunc_sin(void) {
    avm_memcell *arg = libfunc_getarg(0);
    if (arg->type != number_m) { retval_reg->type = nil_m; return; }
    retval_reg->type = number_m;
    retval_reg->data.numVal = sin(arg->data.numVal * 3.141592653589793 / 180.0);
}

void libfunc_input(void) {
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin)) {
        retval_reg->type = nil_m;
        return;
    }
    size_t len = strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = '\0';

    if (!strcmp(buf, "true")) {
        retval_reg->type = bool_m;
        retval_reg->data.boolVal = 1;
        return;
    }
    if (!strcmp(buf, "false")) {
        retval_reg->type = bool_m;
        retval_reg->data.boolVal = 0;
        return;
    }
    char *end = NULL;
    double v = strtod(buf, &end);
    while (end && *end == ' ') end++;
    if (end != buf && *end == '\0') {
        retval_reg->type = number_m;
        retval_reg->data.numVal = v;
        return;
    }
    retval_reg->type = string_m;
    retval_reg->data.strVal = strdup(buf);
}

/* ---------- Function‐Call Mechanics ‐ PUSHARG ---------- */
void execute_PUSHARG(instruction *instr) {
    ensure_expr_stack();
    check_stack_bounds();
    avm_memcell *source = avm_translate_operand(&instr->arg1,
                                                &AVM_TMP1);
    if (!source)
        avm_error("PUSHARG: cannot translate argument");
    avm_memcell_copy(&stack[top], source);
    top--;
}

static void enter_userfunc(unsigned funcAddr) {
    check_stack_bounds();
    unsigned saved_top = top;
    unsigned saved_topsp = topsp;
    unsigned ret_pc = pc;

    stack[top--] = (avm_memcell){ .type = number_m, .data.numVal = (double)ret_pc };
    stack[top--] = (avm_memcell){ .type = number_m, .data.numVal = (double)saved_topsp };
    stack[top--] = (avm_memcell){ .type = number_m, .data.numVal = (double)saved_top };

    enclosing_topsp = top + 1;
    topsp = top + 1;
    pc    = funcAddr;
}

void execute_CALLFUNC(instruction *instr)
{
    avm_memcell *func =
        avm_translate_operand(&instr->arg1, &AVM_TMP1);
    if (!func)
        avm_error("CALLFUNC: cannot translate operand");

    if (func->type == libfunc_m || func->type == string_m) {
        const char *name = (func->type == libfunc_m)
                            ? func->data.libfuncVal
                            : func->data.strVal;

        unsigned saved_topsp = topsp;
        unsigned n = 0;

        enclosing_topsp = topsp;
        topsp = top + 1;
        if (stack[topsp].type == number_m && stack[topsp].data.numVal >= 0)
            n = (unsigned)stack[topsp].data.numVal;
        avm_calllibfunc(name);
        top = topsp + n;
        topsp = saved_topsp;
        ensure_expr_stack();
        return;
    }

    if (func->type == userfunc_m) {
        enter_userfunc(func->data.funcVal);
        return;
    }

    if (func->type == table_m) {
        avm_memcell key = { .type = string_m, .data.strVal = "()" };
        avm_memcell *method = avm_tablegetelem(func->data.tableVal, &key);
        unsigned callable = 0;
        if (method && method->type == userfunc_m)
            callable = method->data.funcVal;
        else if (!table_single_userfunc(func->data.tableVal, &callable))
            callable = 0;

        if (callable) {
            if (method && method->type == userfunc_m) {
                check_stack_bounds();
                unsigned n = 0;
                if (stack[top + 1].type == number_m)
                    n = (unsigned)stack[top + 1].data.numVal;
                /* Reserve one slot below; args stay at the same absolute indices. */
                top--;
                stack[top + 1].type = number_m;
                stack[top + 1].data.numVal = (double)(n + 1);
                avm_memcell_copy(&stack[top + 2], func);
            }
            enter_userfunc(callable);
            return;
        }
    }

    avm_error("call: cannot bind '%s' to function!", avm_tostring(func));
}

void execute_GETRET(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &AVM_TMP2);
    if (!lv) avm_error("GETRET: cannot translate LHS");
    /* Copy retval_reg into lv */
    *lv = *retval_reg;
    if (retval_reg->type == string_m)
        lv->data.strVal = strdup(retval_reg->data.strVal);
    else if (retval_reg->type == table_m)
        avm_table_inc_ref(retval_reg->data.tableVal), lv->data.tableVal = retval_reg->data.tableVal;
}

void execute_FUNCENTER(instruction *instr) {
    avm_memcell *numLocalsCell = avm_translate_operand(&instr->arg1, &AVM_TMP3);
    if (numLocalsCell->type != number_m)
        avm_error("FUNCENTER: #locals is not a number");
    unsigned numLocals = (unsigned)numLocalsCell->data.numVal;
    top = topsp - 1;
    for (unsigned i = 0; i < numLocals; ++i) {
        check_stack_bounds();
        stack[top--].type = undef_m;
    }
}

void execute_FUNCEXIT(instruction *instr) {
    (void)instr;
    unsigned ftops = topsp;
    if (ftops + 3 >= STACK_SIZE)
        avm_error("FUNCEXIT: corrupted stack frame");
    if (stack[ftops].type != number_m ||
        stack[ftops + 1].type != number_m ||
        stack[ftops + 2].type != number_m ||
        stack[ftops + 3].type != number_m)
        avm_error("FUNCEXIT: corrupted stack frame");
    unsigned old_top = top;
    unsigned saved_top_val = (unsigned)stack[ftops].data.numVal;
    unsigned nargs = (unsigned)stack[ftops + 3].data.numVal;
    top   = saved_top_val + nargs + 1;
    topsp = (unsigned)stack[ftops + 1].data.numVal;
    pc    = (unsigned)stack[ftops + 2].data.numVal;
    enclosing_topsp = topsp;
    while (old_top < top)
        stack[old_top++].type = undef_m;
    ensure_expr_stack();
}

/* ---------- Arithmetic / Table / Assign / NOP ---------- */

void execute_ADD(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &AVM_TMP3);
    avm_memcell *r  = avm_translate_operand(&instr->arg1,   &AVM_TMP2);
    avm_memcell *l  = avm_translate_operand(&instr->arg2,   &AVM_TMP1);
    if (l->type != number_m || r->type != number_m)
        avm_error("ADD: non‐numeric operands '%s' + '%s'", avm_tostring(l), avm_tostring(r));
    lv->type = number_m;
    lv->data.numVal = l->data.numVal + r->data.numVal;
}

void execute_SUB(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &AVM_TMP3);
    avm_memcell *l  = avm_translate_operand(&instr->arg1,   &AVM_TMP2);  // left
    avm_memcell *r  = avm_translate_operand(&instr->arg2,   &AVM_TMP1);  // right

    if (l->type != number_m || r->type != number_m)
        avm_error("SUB: non‐numeric operands '%s' - '%s'", avm_tostring(l), avm_tostring(r));
    
    lv->type = number_m;
    lv->data.numVal = l->data.numVal - r->data.numVal;

    // fprintf(stderr, "[SUB] %g - %g = %g\n", l->data.numVal, r->data.numVal, lv->data.numVal);
}

void execute_MUL(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &AVM_TMP3);
    avm_memcell *r  = avm_translate_operand(&instr->arg1,   &AVM_TMP2);
    avm_memcell *l  = avm_translate_operand(&instr->arg2,   &AVM_TMP1);
    if (l->type != number_m || r->type != number_m)
        avm_error("MUL: non‐numeric operands '%s' * '%s'", avm_tostring(l), avm_tostring(r));
    lv->type = number_m;
    lv->data.numVal = l->data.numVal * r->data.numVal;
}

void execute_DIV(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &AVM_TMP3);
    avm_memcell *l  = avm_translate_operand(&instr->arg1,   &AVM_TMP2);  // left (dividend)
    avm_memcell *r  = avm_translate_operand(&instr->arg2,   &AVM_TMP1);  // right (divisor)
    
    if (l->type != number_m || r->type != number_m)
        avm_error("DIV: non‐numeric operands '%s' / '%s'", avm_tostring(l), avm_tostring(r));
    
    if ((int)r->data.numVal == 0)
        avm_error("DIV: division by zero");

    double left  = l->data.numVal;
    double right = r->data.numVal;

    if (right == 0.0)
        avm_error("DIV: division by zero");

    lv->type = number_m;
    lv->data.numVal = left / right;
}

void execute_MOD(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &AVM_TMP3);
    avm_memcell *arg1 = avm_translate_operand(&instr->arg1, &AVM_TMP2);
    avm_memcell *arg2 = avm_translate_operand(&instr->arg2, &AVM_TMP1);
    
    if (arg1->type != number_m || arg2->type != number_m) {
        avm_error("MOD: non-numeric operands '%s' %% '%s'", avm_tostring(arg1), avm_tostring(arg2));
    }

    int dividend = (int)arg1->data.numVal;
    int divisor  = (int)arg2->data.numVal;
    
    if (divisor == 0) {
        avm_error("MOD: modulo by zero");
    }
    
    int result = dividend % divisor;

    // fprintf(stderr, "[MOD] %d %% %d = %d\n", dividend, divisor, result);

    lv->type = number_m;
    lv->data.numVal = (double)result;
}

void execute_ASSIGN(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &AVM_TMP2);
    avm_memcell *rv = avm_translate_operand(&instr->arg1,   &AVM_TMP1);
    if (!lv || !rv) avm_error("ASSIGN: invalid operand");
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
    avm_memcell *lv = avm_translate_operand(&instr->result, &AVM_TMP1);
    lv->type = table_m;
    lv->data.tableVal = avm_table_new();
    avm_table_inc_ref(lv->data.tableVal);
}

void execute_TABLEGETELM(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, &AVM_TMP1);
    avm_memcell *tbl = avm_translate_operand(&instr->arg1, &AVM_TMP2);
    avm_memcell *key = avm_translate_operand(&instr->arg2, &AVM_TMP3);
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
    avm_memcell *tbl = avm_translate_operand(&instr->arg1, &AVM_TMP3);
    avm_memcell *key = avm_translate_operand(&instr->arg2, &AVM_TMP2);
    avm_memcell *val = avm_translate_operand(&instr->result, &AVM_TMP1);
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
    avm_memcell *l = avm_translate_operand(&instr->arg1, &AVM_TMP2);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &AVM_TMP1);
    int result = 0;
    if (l->type == nil_m && r->type == nil_m) result = 1;
    else if (l->type == bool_m && r->type == bool_m) result = (l->data.boolVal == r->data.boolVal);
    else if (l->type == number_m && r->type == number_m) result = (l->data.numVal == r->data.numVal);
    else if (l->type == string_m && r->type == string_m) result = (strcmp(l->data.strVal, r->data.strVal) == 0);
    else if (l->type == table_m && r->type == table_m) result = (l->data.tableVal == r->data.tableVal);
    else if (l->type == userfunc_m && r->type == userfunc_m) result = (l->data.funcVal == r->data.funcVal);
    else if (l->type == libfunc_m && r->type == libfunc_m) result = (strcmp(l->data.libfuncVal, r->data.libfuncVal) == 0);
    if (result) pc = instr->result.value; /* skip to label if equal */
}

/* ---------- Helper: deep / pointer equality for every Alpha type ---------- */
static int avm_cells_equal(avm_memcell *l, avm_memcell *r)
{
    if (l->type != r->type)
        return 0;                                   /* different kinds → unequal */

    switch (l->type)
    {
        case number_m:   return l->data.numVal  == r->data.numVal;
        case string_m:   return strcmp(l->data.strVal, r->data.strVal) == 0;
        case bool_m:     return l->data.boolVal == r->data.boolVal;
        case nil_m:      return 1;                  /* nil == nil */
        case table_m:    return l->data.tableVal  == r->data.tableVal;
        case userfunc_m: return l->data.funcVal   == r->data.funcVal;
        case libfunc_m:  return strcmp(l->data.libfuncVal, r->data.libfuncVal) == 0;
        case undef_m:    /* falls through */
        default:         return 0;                  /* treat undefined as unequal */
    }
}

/* ---------- JNE  (jump if NOT equal)  ---------- */
void execute_JNE(instruction *instr)
{
    /* optimisation: code-generator emits   JNE label,label   for “jump always” */
    if (is_unconditional_jump(&instr->arg1, &instr->arg2)) {
        pc = instr->result.value;        /* pc already ++, so no “-1” here     */
        return;
    }

    /* normal case -‐ translate operands */
    avm_memcell *l = avm_translate_operand(&instr->arg1, &AVM_TMP2);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &AVM_TMP1);

    if (!avm_cells_equal(l, r))   
        pc = instr->result.value;  
}

static inline int relop_numbers(avm_memcell *lv, avm_memcell *rv,
                                double *lnum, double *rnum)
{
    if (lv->type == nil_m || lv->type == undef_m ||
        rv->type == nil_m || rv->type == undef_m)
        return 0;
    if (lv->type == number_m)
        *lnum = lv->data.numVal;
    else if (lv->type == bool_m)
        *lnum = (double)lv->data.boolVal;
    else
        return 0;
    if (rv->type == number_m)
        *rnum = rv->data.numVal;
    else if (rv->type == bool_m)
        *rnum = (double)rv->data.boolVal;
    else
        return 0;
    return 1;
}

void execute_JLE(instruction *instr)
{
    if (is_unconditional_jump(&instr->arg1, &instr->arg2)) {
        pc = instr->result.value;
        return;
    }

    avm_memcell *lv = avm_translate_operand(&instr->arg1, &AVM_TMP2);
    avm_memcell *rv = avm_translate_operand(&instr->arg2, &AVM_TMP1);

    double lnum, rnum;
    if (relop_numbers(lv, rv, &lnum, &rnum) && lnum <= rnum)
        pc = instr->result.value;
}

void execute_JLT(instruction *instr)
{
    if (is_unconditional_jump(&instr->arg1, &instr->arg2)) {
        pc = instr->result.value;
        return;
    }

    avm_memcell *lv = avm_translate_operand(&instr->arg1, &AVM_TMP2);
    avm_memcell *rv = avm_translate_operand(&instr->arg2, &AVM_TMP1);

    double lnum, rnum;
    if (relop_numbers(lv, rv, &lnum, &rnum) && lnum < rnum)
        pc = instr->result.value;
}

void execute_JGE(instruction *instr)
{
    if (is_unconditional_jump(&instr->arg1, &instr->arg2)) {
        pc = instr->result.value;
        return;
    }

    avm_memcell *lv = avm_translate_operand(&instr->arg1, &AVM_TMP2);
    avm_memcell *rv = avm_translate_operand(&instr->arg2, &AVM_TMP1);

    double lnum, rnum;
    if (relop_numbers(lv, rv, &lnum, &rnum) && lnum >= rnum)
        pc = instr->result.value;
}

void execute_JGT(instruction *instr)
{
    if (is_unconditional_jump(&instr->arg1, &instr->arg2)) {
        pc = instr->result.value;
        return;
    }

    avm_memcell *lv = avm_translate_operand(&instr->arg1, &AVM_TMP2);
    avm_memcell *rv = avm_translate_operand(&instr->arg2, &AVM_TMP1);

    double lnum, rnum;
    if (relop_numbers(lv, rv, &lnum, &rnum) && lnum > rnum)
        pc = instr->result.value;
}

void execute_AND(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &AVM_TMP2);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &AVM_TMP1);
    if (l->type != bool_m || r->type != bool_m)
        avm_error("AND: non‐bool operands '%s' and '%s'", avm_tostring(l), avm_tostring(r));
    if (!(l->data.boolVal && r->data.boolVal)) pc = instr->result.value;
}

void execute_OR(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &AVM_TMP2);
    avm_memcell *r = avm_translate_operand(&instr->arg2, &AVM_TMP1);
    if (l->type != bool_m || r->type != bool_m)
        avm_error("OR: non‐bool operands '%s' and '%s'", avm_tostring(l), avm_tostring(r));
    if (!(l->data.boolVal || r->data.boolVal)) pc = instr->result.value;
}

void execute_NOT(instruction *instr) {
    avm_memcell *l = avm_translate_operand(&instr->arg1, &AVM_TMP1);
    if (l->type != bool_m)
        avm_error("NOT: non‐bool operand '%s'", avm_tostring(l));
    if (!l->data.boolVal) pc = instr->result.value;
}

void execute_NEG(instruction *instr) {
    static avm_memcell lv_temp, rv_temp;

    avm_memcell *rv = avm_translate_operand(&instr->arg1, &rv_temp);
    avm_memcell *lv = avm_translate_operand(&instr->result, &lv_temp);

    if (!rv || rv->type != number_m) {
        avm_error("NEG: operand is not a number '%s'", avm_tostring(rv));
        return;
    }
    if (!lv) {
        avm_error("NEG: invalid result target");
        return;
    }

    lv->type = number_m;
    lv->data.numVal = -rv->data.numVal;
}

static void load_binary(const char *filename) {
    FILE *fp = fopen(filename,"rb");
    if(!fp) avm_error("Cannot open %s", filename);

    uint32_t count;

    // 1) instructions
    fread(&count, sizeof(count), 1, fp);
    total_instructions = count;
    code = malloc(count * sizeof(instruction));
    fread(code, sizeof(instruction), count, fp);

    // 2) numConsts
    fread(&count, sizeof(count), 1, fp);
    total_const_nums = count;
    const_nums = malloc(count * sizeof(double));
    fread(const_nums, sizeof(double), count, fp);

    // 3) stringConsts
    fread(&count, sizeof(count), 1, fp);
    total_const_strs = count;
    const_strs = malloc(count * sizeof(char*));
    for(uint32_t i = 0; i < count; ++i) {
        uint32_t len;
        fread(&len, sizeof(len), 1, fp);
        char *buf = malloc(len+1);
        fread(buf, sizeof(char), len, fp);
        buf[len] = '\0';
        const_strs[i] = buf;
    }

    if (fread(&program_var_count, sizeof(program_var_count), 1, fp) != 1)
        program_var_count = 0;

    fclose(fp);
}

static void register_libfuncs(void) {
    avm_registerlibfunc("print",               libfunc_print);
    avm_registerlibfunc("input",               libfunc_input);
    avm_registerlibfunc("objectmemberkeys",    libfunc_objectmemberkeys);
    avm_registerlibfunc("objecttotalmembers",  libfunc_objecttotalmembers);
    avm_registerlibfunc("objectcopy",          libfunc_objectcopy);
    avm_registerlibfunc("totalarguments",      libfunc_totalarguments);
    avm_registerlibfunc("argument",            libfunc_argument);
    avm_registerlibfunc("typeof",              libfunc_typeof);
    avm_registerlibfunc("strtonum",            libfunc_strtonum);
    avm_registerlibfunc("sqrt",                libfunc_sqrt);
    avm_registerlibfunc("cos",                 libfunc_cos);
    avm_registerlibfunc("sin",                 libfunc_sin);
}

void vm_init(void) {
    load_binary("out.abc");

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
        avm_memcell *cell = &stack[i];
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
    free(table_tostring_buf);
    table_tostring_buf = NULL;
    table_tostring_cap = 0;
    table_tostring_len = 0;

    for (unsigned i = 0; i < num_userfuncs; ++i) {
        free(userfuncs[i].name);
    }

    for (unsigned i = 0; i < num_registered_libfuncs; ++i){
        free((char*)libfunc_names_arr[i]);
    }
}

void vm_run(void) {
    unsigned long long steps = 0; 

    while (pc < total_instructions) {

        if (++steps > MAX_EXEC_STEPS) {
            fprintf(stderr, "infinite loop\n");
            fflush(stderr); 
            exit(EXIT_FAILURE);
        }

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
            case op_funcenter:      execute_FUNCENTER(instr);     break;
            case op_funcexit:       execute_FUNCEXIT(instr);      break;
            default:
                avm_error("Unknown opcode %d at pc=%u", instr->opcode, pc-1);
        }
    }
}
