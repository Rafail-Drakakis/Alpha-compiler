/**
 * HY-340 Project Phase 2 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#include <stdio.h>
#include <stdlib.h>
#include "symbol_table.h"
#include "parser.h"

extern FILE *yyin;
SymbolTable *symbol_table;

static const char *library_functions[] = {
    "print",
    "input",
    "objectmemberkeys",
    "objecttotalmembers",
    "objectcopy",
    "totalarguments",
    "argument",
    "typeof",
    "strtonum",
    "sqrt",
    "cos",
    "sin"
};

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Usage: %s <input_file> [output_file]\n", argv[0]);
        return EXIT_FAILURE;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("Error opening input file");
        return EXIT_FAILURE;
    }

    if (argc == 3) {
        freopen(argv[2], "w", stdout);
    }

    symbol_table = create_symbol_table();

    for (int k = 0; k < (sizeof(library_functions) / sizeof(library_functions[0])); k++) {
        insert_symbol(symbol_table, library_functions[k], LIBRARY_FUNCTION, 0, 0);
    }    
    
    print_symbol_table(symbol_table);

    fclose(yyin);
    free_symbol_table(symbol_table);

    return EXIT_SUCCESS;
}