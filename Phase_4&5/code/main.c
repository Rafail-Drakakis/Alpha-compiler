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

#include "quads.h"
#include "symbol_table.h"
#include "codegen.h" 

extern unsigned int checkScope;
extern int yyparse();        
extern FILE* yyin;
extern int semantic_errors;
extern unsigned total;
extern unsigned int currQuad;
 
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
    } else {
        fprintf(stderr, "No quads to print or empty quads array\n");
    }
 
    free_symbol_table(symbol_table);
    if (argc == 2 && yyin) {
        fclose(yyin);
    }
 
    return 0;
} 