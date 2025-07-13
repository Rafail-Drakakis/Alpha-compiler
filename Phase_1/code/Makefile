/**
 * HY-340 Project Phase 1 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

CC = gcc
FLEX = flex
LEX_FILE = al.l
OUTPUT = lexer

all: $(OUTPUT)
	./$(OUTPUT) test.txt output.txt

$(OUTPUT): lex.yy.c
	$(CC) -o $(OUTPUT) lex.yy.c -lfl

lex.yy.c: $(LEX_FILE)
	$(FLEX) $(LEX_FILE)

clean:
	rm -f $(OUTPUT) lex.yy.c output.txt

