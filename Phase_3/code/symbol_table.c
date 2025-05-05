#include "symbol_table.h"

static const char *get_symbol_type_str(SymbolType symbol_type) {
    switch (symbol_type) {
        case GLOBAL:            return "global variable";
        case LOCAL_VAR:         return "local variable";
        case ARGUMENT:          return "formal argument";
        case LIBRARY_FUNCTION:  return "library function";
        case USER_FUNCTION:     return "user function";
        default:                return "unknown";
    }
}

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

/* Search from scope-1 to 0 for a variable with the same name */
static SymbolTableEntry *lookup_visible_var(SymbolTable *symbol_table, const char *name, unsigned scope) {                                   /* ψάχνουμε από scope-1 ως 0 */
    for (int s = (int)scope - 1; s >= 0; --s)
        for (SymbolTableEntry *current = symbol_table->head; current; current = current->next)
            if (current->scope == (unsigned)s &&
                (current->type == GLOBAL || current->type == LOCAL_VAR || current->type == ARGUMENT) && strcmp(current->name, name) == 0)
                return current;
    return NULL;
}

void insert_symbol(SymbolTable *symbol_table, const char *name, SymbolType type, unsigned int line, unsigned int scope) {
{
    if (type == LOCAL_VAR) {                               
        if (lookup_visible_var(symbol_table, name, scope))          
            return;                                        
    }

    for (SymbolTableEntry *current = symbol_table->head; current; current = current->next)
        if (current->scope == scope && strcmp(current->name, name) == 0) {
            int allow_duplicate = (current->type == ARGUMENT && type == ARGUMENT && current->line_number != line);
            if (!allow_duplicate) {
                fprintf(stderr, "Error: Symbol '%s' already defined in scope %u at line %u.\n", name, scope, line);
                return;
            }
        }

        if (scope != 0) {
            SymbolTableEntry *global_entry = lookup_symbol_global(symbol_table, name);
            if (global_entry && global_entry->type == LIBRARY_FUNCTION) {
                fprintf(stderr, "Error: Cannot redeclare library function '%s' (line %u).\n", name, line);
                return;
            }
        }

        SymbolTableEntry *new_entry = create_entry(name, type, line, scope);

        if (!symbol_table->head) symbol_table->head = new_entry;
        else {
            SymbolTableEntry *current = symbol_table->head;
            while (current->next) current = current->next;
            current->next = new_entry;
        }
    }
}

SymbolTableEntry *lookup_symbol(SymbolTable *symbol_table, const char *name, unsigned int current_scope, int is_function_context) {
    for (int scope = (int)current_scope; scope >= 0; --scope)
        for (SymbolTableEntry *current = symbol_table->head; current; current = current->next)
            if (current->scope == (unsigned)scope && strcmp(current->name, name) == 0) {

                if (current->type == USER_FUNCTION || current->type == LIBRARY_FUNCTION)
                    return current;

                if (current->type == GLOBAL || current->type == LOCAL_VAR || current->type == ARGUMENT) {

                    if (scope == (int)current_scope || scope == 0)
                        return current;

                    if (is_function_context) {
                        int other_scope = 0;
                        for (int t = (int)current_scope; t > scope && !other_scope; --t)
                            for (SymbolTableEntry *x = symbol_table->head; x; x = x->next)
                                if (x->scope == (unsigned)t && x->type == USER_FUNCTION) {
                                    other_scope = 1; break;
                                }
                        if (!other_scope) return current;
                        printf("Error: Symbol '%s' defined at line %u in enclosing function.\n", current->name, current->line_number);
                    }
                }
            }
    return NULL;
}

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

static int compare_by_line(const void *a, const void *b) {
    const SymbolTableEntry *entry_a = *(const SymbolTableEntry * const *)a;
    const SymbolTableEntry *entry_b = *(const SymbolTableEntry * const *)b;
    if (entry_a->line_number < entry_b->line_number) return -1;
    if (entry_a->line_number > entry_b->line_number) return  1;
    return 0;
}

void print_symbol_table(SymbolTable *symbol_table) {
    unsigned int max_scope = 0;
    for (SymbolTableEntry *current = symbol_table->head; 
        current; current = current->next)
        if (current->scope > max_scope) 
        max_scope = current->scope;

    for (unsigned int scope = 0; scope <= max_scope; ++scope) {
        size_t num_entries = 0;
        for (SymbolTableEntry *current = symbol_table->head; current; current = current->next)
            if (current->scope == scope) ++num_entries;
        if (!num_entries) continue;

        SymbolTableEntry **entries_array = malloc(num_entries * sizeof *entries_array);
        size_t i = 0;
        for (SymbolTableEntry *current = symbol_table->head; current; current = current->next)
            if (current->scope == scope) entries_array[i++] = current;

        qsort(entries_array, num_entries, sizeof *entries_array, compare_by_line);

        printf("-----------     Scope #%u     -----------\n", scope);
        for (i = 0; i < num_entries; ++i)
            printf("\"%s\" [%s] (line %u) (scope %u)\n",
                   entries_array[i]->name, get_symbol_type_str(entries_array[i]->type),
                   entries_array[i]->line_number, entries_array[i]->scope);
        putchar('\n');

        free(entries_array);
    }
}

void deactivate_entries_from_curr_scope(SymbolTable *symbol_table, unsigned int scope) {
    SymbolTableEntry *current = symbol_table->head, *previous = NULL;
    while (current) {
        int removable = (current->scope == scope) &&
                        current->type != USER_FUNCTION &&
                        current->type != LOCAL_VAR   &&
                        current->type != ARGUMENT;

        if (removable && scope != 0 && current->type != LIBRARY_FUNCTION) {
            SymbolTableEntry *entry_to_delete = current;
            if (!previous) symbol_table->head = current->next;
            else previous->next = current->next;
            current = current->next;
            free(entry_to_delete->name); 
            free(entry_to_delete);
        } else { previous = current; current = current->next; }
    }
}

void free_symbol_table(SymbolTable *symbol_table) {
    SymbolTableEntry *current = symbol_table->head;
    while (current) { 
        SymbolTableEntry *next = current->next; 
        free(current->name); 
        free(current); current = next; 
    }
    free(symbol_table);
}
