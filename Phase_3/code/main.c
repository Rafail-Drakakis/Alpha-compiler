/**
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
#include <stdlib.h>

#include "quads.h"
#include "symbol_table.h"
 
extern unsigned int checkScope;
extern int yyparse();        
extern FILE* yyin;
extern int semantic_errors;
extern unsigned total;
extern unsigned int currQuad;
 
SymbolTable *symbol_table;
quad* quads = (quad*) 0;
 
void debug(int level, const char* fmt, ...) {
    static int debug_enabled = -1;
    va_list args;
    va_start(args, fmt);

    if (debug_enabled == -1) {
        debug_enabled = (getenv("ALPHA_DEBUG") != NULL);
    }

    if (debug_enabled && level > 0) {
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
 
    int parse_ok = (yyparse() == 0);
    int semantic_ok = (semantic_errors == 0);

    if (parse_ok) {
        if (!semantic_ok) {
            fprintf(stderr, "\nParsing completed with %d semantic error(s).\n", semantic_errors);
        } else {
            FILE *quad_out = fopen("quads.txt", "w");
            if (!quad_out) {
                perror("quads.txt");
                free_symbol_table(symbol_table);
                if (argc == 2 && yyin) {
                    fclose(yyin);
                }
                return 1;
            }

            print_quads(quad_out);
            fclose(quad_out);
            printf("Parsing completed successfully. Wrote quads to quads.txt\n");
        }
    } else {
        fprintf(stderr, "Parsing failed.\n");
    }
 
    free_symbol_table(symbol_table);
    if (argc == 2 && yyin) {
        fclose(yyin);
    }
 
    return 0;
} 