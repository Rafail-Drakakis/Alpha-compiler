/**
 * HY-340 Project Phase 2 2024-2025
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

typedef enum {
    GLOBAL,
    LOCAL_VAR,
    ARGUMENT,
    LIBRARY_FUNCTION,
    USER_FUNCTION
} SymbolType;

typedef struct SymbolTableEntry {
    char *name;
    SymbolType type;
    unsigned int line_number;
    unsigned int scope;
    int is_active;
    struct SymbolTableEntry *next;
} SymbolTableEntry;

typedef struct SymbolTable {
    SymbolTableEntry *head;
} SymbolTable;

SymbolTable *create_symbol_table();
SymbolTableEntry *create_entry(const char *name, SymbolType type, unsigned int line, unsigned int scope);
void insert_symbol(SymbolTable *table, const char *name, SymbolType type, unsigned int line, unsigned int scope);
SymbolTableEntry *lookup_symbol(SymbolTable *table, const char *name, unsigned int scope, int is_function_context);
SymbolTableEntry *lookup_symbol_global(SymbolTable *table, const char *name);
void print_symbol_table(SymbolTable *table);
void free_symbol_table(SymbolTable *table);

void deactivate_entries_from_curr_scope(SymbolTable*, unsigned int scope);

#endif
