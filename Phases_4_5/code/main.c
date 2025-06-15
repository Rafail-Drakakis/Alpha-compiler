/*
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>

#include "quads.h"
#include "symbol_table.h"
#include "codegen.h"
#include "vm.h"

extern unsigned int checkScope;
extern int yyparse();        
extern FILE* yyin;
extern int semantic_errors;
extern unsigned total;
extern unsigned int currQuad;
extern unsigned int currInstruction;

SymbolTable *symbol_table;
quad* quads = (quad*) 0;

void debug(int level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (level > 0) {
        fprintf(stderr, "[DEBUG] ");
        vfprintf(stderr, fmt, args);
    }

    va_end(args);
}

// Signal handler for segmentation faults
void segfault_handler(int sig) {
    fprintf(stderr, "Caught segmentation fault! Current quad: %d\n", currQuad);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    // Add signal handler for segmentation faults
    signal(SIGSEGV, segfault_handler);
     
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [inputfile]\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror(argv[1]);
            return 1;
        }
    }
     
    // Initialize globals safely
    quads = NULL;
    total = 0;
    currQuad = 0;
     
    symbol_table = create_symbol_table();
    if (!symbol_table) {
        fprintf(stderr, "Failed to create symbol table\n");
        return 1;
    }

    // Register built-in library functions in global scope
    const char *builtins[] = { "typeof", "totalarguments", "argument", "print" };
    for (size_t i = 0; i < sizeof(builtins)/sizeof(*builtins); ++i) {
        SymbolTableEntry *sym = insert_symbol(
            symbol_table,
            builtins[i],
            LIBRARY_FUNCTION,   
            0,                  // line 0 (predefined)
            0                   // scope 0 = global
        );
        if (!sym) {
            fprintf(stderr, "Warning: could not register builtin %s\n", builtins[i]);
        } else {
            // Place it as a true global at the next available slot
            sym->space  = programvar;
            sym->offset = programVarOffset++;
        }
    }

    if (yyparse() == 0) {
        if (semantic_errors > 0) {
            fprintf(stderr, "\nParsing completed with %d semantic error(s).\n", semantic_errors);
        } else {
            printf("Parsing completed successfully.\n");
        }
    } else {
        fprintf(stderr, "Parsing failed.\n");
    }
 
    if (quads && currQuad > 0) {
        print_quads(stdout);

        generate_target_code();
        printf("\n------ Target VM Instructions ------\n");
        print_instructions(stdout);
        write_text("out.txt", currInstruction);
        write_binary("out.bin"  );
    } else {
        fprintf(stderr, "No quads to print or empty quads array\n");
    }

    vm_init();
    vm_run();
    avm_destroy();

    free_symbol_table(symbol_table);
    if (argc == 2 && yyin) {
        fclose(yyin);
    }
 
    return 0;
}
