#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    GLOBAL,
    LOCAL_VAR,  // Renamed to avoid conflict with YACC token
    ARGUMENT,
    LIBRARY_FUNCTION,
    USER_FUNCTION
} SymbolType;

typedef struct SymbolTableEntry {
    char *name;
    SymbolType type;
    unsigned int line_number;
    unsigned int scope;
    struct SymbolTableEntry *next;
} SymbolTableEntry;

typedef struct SymbolTable {
    SymbolTableEntry *head;
} SymbolTable;

SymbolTable *create_symbol_table();
SymbolTableEntry *create_entry(const char *name, SymbolType type, unsigned int line, unsigned int scope);
void insert_symbol(SymbolTable *table, const char *name, SymbolType type, unsigned int line, unsigned int scope);
SymbolTableEntry *lookup_symbol(SymbolTable *table, const char *name, unsigned int scope);
SymbolTableEntry *lookup_symbol_global(SymbolTable *table, const char *name);
void print_symbol_table(SymbolTable *table);
void free_symbol_table(SymbolTable *table);

#endif
