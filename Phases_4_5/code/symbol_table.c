/**
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#include "symbol_table.h"
#include "quads.h" 

unsigned global_offset = 0;
unsigned local_offset = 0;
unsigned formal_offset = 0;

void assign_space_and_offset(SymbolTableEntry* sym) {
    if (sym->scope == 0) {
        sym->space = programvar;
        sym->offset = global_offset++;
    } else if (sym->type == ARGUMENT) {
        sym->space = formalarg;
        sym->offset = formal_offset++;
    } else {
        sym->space = functionlocal;
        sym->offset = local_offset++;
    }
}

static const char *get_symbol_type_str(SymbolType symbol_type) {
    switch (symbol_type) {
        case GLOBAL:            return "global variable";
        case LOCAL_VAR:         return "local variable";
        case ARGUMENT:          return "formal argument";
        case LIBRARY_FUNCTION:  return "library function";
        case USER_FUNCTION:     return "user function";
        case TEMP_VAR:          return "temporary variable";
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
static SymbolTableEntry *lookup_visible_var(SymbolTable *symbol_table, const char *name, unsigned scope) {
    for (int s = (int)scope - 1; s >= 0; --s)
        for (SymbolTableEntry *current = symbol_table->head; current; current = current->next)
            if (current->scope == (unsigned)s &&
                (current->type == GLOBAL || current->type == LOCAL_VAR || current->type == ARGUMENT) && strcmp(current->name, name) == 0)
                return current;
    return NULL;
}

SymbolTableEntry* insert_symbol(SymbolTable *table,
                                const char *name,
                                SymbolType type,
                                unsigned int line,
                                unsigned int scope)
{
    // 1) LOCAL_VAR may not shadow any visible variable in the same or outer scopes
    if (type == LOCAL_VAR) {
        if (lookup_visible_var(table, name, scope)) {
            // duplicate in local scope or outer → fail quietly
            return NULL;
        }
    }

    // 2) In the *same* scope, disallow redeclaring anything except
    //    allow two ARGUMENT entries if they appear on different lines
    for (SymbolTableEntry *cur = table->head; cur; cur = cur->next) {
        if (cur->scope == scope && strcmp(cur->name, name) == 0) {
            if (type == ARGUMENT && cur->type == ARGUMENT && cur->line_number != line) {
                // two different formal args with same name on different lines? unlikely but allow
                break;
            }
            fprintf(stderr,
                    "Error: Symbol '%s' already defined in scope %u (line %u).\n",
                    name, scope, line);
            return NULL;
        }
    }

    // 3) Never let a local or argument shadow a library function in global scope
    if (scope != 0) {
        SymbolTableEntry *g = lookup_symbol_global(table, name);
        if (g && g->type == LIBRARY_FUNCTION) {
            fprintf(stderr,
                    "Error: Cannot redeclare library function '%s' (line %u).\n",
                    name, line);
            return NULL;
        }
    }

    // 4) All checks passed — create & link the new entry
    SymbolTableEntry *entry = create_entry(name, type, line, scope);
    assign_space_and_offset(entry);

    // append at tail for predictable ordering
    if (!table->head) {
        table->head = entry;
    } else {
        SymbolTableEntry *t = table->head;
        while (t->next) t = t->next;
        t->next = entry;
    }

    return entry;
}

SymbolTableEntry *lookup_symbol(SymbolTable *symbol_table, const char *name, unsigned int current_scope, int is_function_context) {
    for (int scope = (int)current_scope; scope >= 0; --scope)
        for (SymbolTableEntry *current = symbol_table->head; current; current = current->next)
            if (current->scope == (unsigned)scope && strcmp(current->name, name) == 0) {

                if (current->type == USER_FUNCTION || current->type == LIBRARY_FUNCTION)
                    return current;

                if (current->type == GLOBAL || current->type == LOCAL_VAR || current->type == ARGUMENT || current->type == TEMP_VAR) {

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
            //free(entry_to_delete->name); 
            //free(entry_to_delete);
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

/* based on lec 10 slide 32 (custom) */

void comperror(char* format, ...) {             
    va_list args;
    va_start(args, format);

    fprintf(stderr, "Compiler Error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);
    exit(EXIT_FAILURE);
}

/**
 * Use this function to check correct use of of expression in arithmetic 
 */

void check_arith (expr* e, const char* context) {
    if ( e->type == constbool_e ||
    e->type == conststring_e    ||
    e->type == nil_e            ||
    e->type == newtable_e       ||
    e->type == programfunc_e    ||
    e->type == libraryfunc_e    ||
    e->type == boolexpr_e )
    comperror("Illegal expr used in %s!", context); 
}
