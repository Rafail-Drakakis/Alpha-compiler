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
    // First, check for a definition in the current scope
    SymbolTableEntry *existing_local = lookup_symbol(table, name, scope);
    if (existing_local) {
        fprintf(stderr, "Error: Symbol '%s' already defined in scope %u at line %u.\n", name, scope, line);
        return;
    }
    // For any insertion into a non-global scope, verify that we're not shadowing a library function.
    if (scope != 0) {
        SymbolTableEntry *global_entry = lookup_symbol_global(table, name);
        if (global_entry && global_entry->type == LIBRARY_FUNCTION) {
            fprintf(stderr, "Error: Cannot redeclare library function '%s' in a local scope (line %u).\n", name, line);
            return;
        }
    }
    SymbolTableEntry *entry = create_entry(name, type, line, scope);
    if (table->head == NULL) {
        table->head = entry;
    } else {
        SymbolTableEntry *current = table->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = entry;
    }
}

// This function looks up a symbol in the given scope.
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

// This function looks up a symbol in the global scope (scope == 0).
SymbolTableEntry *lookup_symbol_global(SymbolTable *table, const char *name) {
    SymbolTableEntry *current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0 && current->scope == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void print_symbol_table(SymbolTable *table) {
    SymbolTableEntry *current = table->head;
    
    unsigned int max_scope = 0;
    SymbolTableEntry *tmp = current;
    while (tmp) {
        if (tmp->scope > max_scope)
            max_scope = tmp->scope;
        tmp = tmp->next;
    }

    for (unsigned int scope = 0; scope <= max_scope; scope++) {
        int scope_has_entries = 0;

        tmp = table->head;
        while (tmp) {
            if (tmp->scope == scope) {
                scope_has_entries = 1;
                break;
            }
            tmp = tmp->next;
        }

        if (!scope_has_entries) continue;

        printf("----------   Scope #%u   ----------\n", scope);

        tmp = table->head;
        while (tmp) {
            if (tmp->scope == scope) {
                const char *type_str;
                switch (tmp->type) {
                    case GLOBAL: type_str = "global variable"; break;
                    case LOCAL_VAR: type_str = "local variable"; break;
                    case ARGUMENT: type_str = "formal argument"; break;
                    case LIBRARY_FUNCTION: type_str = "library function"; break;
                    case USER_FUNCTION: type_str = "user function"; break;
                    default: type_str = "unknown"; break;
                }

                printf("\"%s\"\t[%s] (line %u) (scope %u)\n",
                       tmp->name, type_str, tmp->line_number, tmp->scope);
            }
            tmp = tmp->next;
        }

        printf("\n");
    }
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
