#include <stdio.h>
#include <stdlib.h>
#include "symbol_table.h"
#include "parser.h"

extern FILE *yyin;
SymbolTable *symbol_table;

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

    printf("Starting syntax analysis...\n");
    if (yyparse() == 0) {
        printf("Syntax analysis completed successfully.\n");
    } else {
        printf("Syntax analysis encountered errors.\n");
    }

    print_symbol_table(symbol_table);

    fclose(yyin);
    free_symbol_table(symbol_table);

    return EXIT_SUCCESS;
}