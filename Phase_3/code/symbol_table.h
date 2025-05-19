/**
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct expr;

typedef enum {
    GLOBAL,
    LOCAL_VAR,
    ARGUMENT,
    LIBRARY_FUNCTION,
    USER_FUNCTION,
    TEMP_VAR 
} SymbolType;

typedef enum scopespace_t { 
    programvar, 
    functionlocal, 
    formalarg 
} scopespace_t;

typedef enum symbol_t { 
    var_s, 
    programfunc_s, 
    libraryfunc_s 
} symbol_t; 

typedef struct SymbolTableEntry {
    char *name;
    SymbolType type;
    unsigned int line_number;
    unsigned int scope;
    int is_active;
    struct SymbolTableEntry *next;
    
    scopespace_t space;             // Originating scope space. 
    unsigned int offset;            // Offset in scope space.

} SymbolTableEntry;

typedef struct SymbolTable {
    SymbolTableEntry *head;
} SymbolTable;

SymbolTable *create_symbol_table();
SymbolTableEntry *create_entry(const char *name, SymbolType type, unsigned int line, unsigned int scope);
SymbolTableEntry* insert_symbol(SymbolTable *table, const char *name, SymbolType type, unsigned int line, unsigned int scope);
SymbolTableEntry *lookup_symbol(SymbolTable *table, const char *name, unsigned int scope, int is_function_context);
SymbolTableEntry *lookup_symbol_global(SymbolTable *table, const char *name);
void print_symbol_table(SymbolTable *table);
void free_symbol_table(SymbolTable *table);
void deactivate_entries_from_curr_scope(SymbolTable*, unsigned int scope);

void comperror(char* format, ...);
void check_arith(struct expr* e, const char* context);

#endif
