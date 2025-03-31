#include "symbol_table.h"

SymbolTable *create_symbol_table() {
    SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));
    if (!table) {
        fprintf(stderr, "Memory allocation failed for Symbol Table.\n");
        exit(EXIT_FAILURE);
    }
    table->head = NULL;
    return table;
}

SymbolTableEntry *create_entry(const char *name, SymbolType type, unsigned int line, unsigned int scope) {
    SymbolTableEntry *entry = (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
    if (!entry) {
        fprintf(stderr, "Memory allocation failed for Symbol Table Entry.\n");
        exit(EXIT_FAILURE);
    }
    entry->name = strdup(name);
    entry->type = type;
    entry->line_number = line;
    entry->scope = scope;
    entry->next = NULL;
    return entry;
}

void insert_symbol(SymbolTable *table, const char *name, SymbolType type, unsigned int line, unsigned int scope) {
    SymbolTableEntry *existing = lookup_symbol(table, name, scope);
    if (existing) {
        fprintf(stderr, "Warning: Symbol '%s' already defined in scope %d at line %d.\n", name, scope, line);
        return;
    }

    SymbolTableEntry *entry = create_entry(name, type, line, scope);
    entry->next = table->head;
    table->head = entry;
}

SymbolTableEntry *lookup_symbol(SymbolTable *table, const char *name, unsigned int scope) {
    SymbolTableEntry *current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0 && current->scope == scope) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void print_symbol_table(SymbolTable *table) {
    SymbolTableEntry *current = table->head;
    printf("------------------ Symbol Table ----------------\n");
    printf("| Name          | Type         | Line  | Scope |\n");
    printf("------------------------------------------------\n");

    while (current) {
        const char *type_str;
        switch (current->type) {
            case GLOBAL: type_str = "GLOBAL"; break;
            case LOCAL_VAR: type_str = "LOCAL"; break;
            case ARGUMENT: type_str = "ARGUMENT"; break;
            case LIBRARY_FUNCTION: type_str = "LIB_FUNC"; break;
            case USER_FUNCTION: type_str = "USER_FUNC"; break;
            default: type_str = "UNKNOWN"; break;
        }
        printf("| %-12s | %-11s | %-5u | %-5u |\n", current->name, type_str, current->line_number, current->scope);
        current = current->next;
    }
    printf("------------------------------------------------\n");
}

void free_symbol_table(SymbolTable *table) {
    SymbolTableEntry *current = table->head;
    while (current) {
        SymbolTableEntry *next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
    free(table);
}
