/**
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#include <stdio.h>
#include "quads.h"
#include "symbol_table.h"

extern unsigned int checkScope;
extern int yyparse();        
extern FILE* yyin;
extern int semantic_errors;

SymbolTable *symbol_table;
quad* quads = (quad*) 0;

int main(int argc, char **argv) {
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
    
    symbol_table = create_symbol_table();

    if (yyparse() == 0) {
        /* this wont be enough, we need to use stack */
        if (semantic_errors > 0) {
            fprintf(stderr, "\nParsing completed with %d semantic error(s).\n", semantic_errors);
        } else {
            printf("Parsing completed successfully.\n");
        }
    } else {
        fprintf(stderr, "Parsing failed.\n");
    }


    print_quads(stdout);

    free_symbol_table(symbol_table);
    if (argc == 2 && yyin) {
        fclose(yyin);
    }

    return 0;
}
