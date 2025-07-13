/**
 * HY-340 Project Phase 2 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

CC = gcc
CFLAGS = -g

LEX = flex
BISON = bison

LEX_FILE = al.l
BISON_FILE = parser.y

BISON_C = parser.c
BISON_H = parser.h

LEX_C = lex.yy.c

MAIN_C = main.c
SYMBOL_TABLE_C = symbol_table.c
SYMBOL_TABLE_H = symbol_table.h

OBJS = parser.o lex.yy.o main.o symbol_table.o
EXE = parser

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BISON_C) $(BISON_H): $(BISON_FILE)
	$(BISON) -d $(BISON_FILE) -o $(BISON_C) --defines=$(BISON_H)

$(LEX_C): $(LEX_FILE) $(BISON_H)
	$(LEX) -o $(LEX_C) $(LEX_FILE)

parser.o: $(BISON_C)
	$(CC) $(CFLAGS) -c $(BISON_C) -o parser.o

lex.yy.o: $(LEX_C)
	$(CC) $(CFLAGS) -c $(LEX_C) -o lex.yy.o

main.o: $(MAIN_C) $(SYMBOL_TABLE_H) $(BISON_H)
	$(CC) $(CFLAGS) -c $(MAIN_C)
		
symbol_table.o: $(SYMBOL_TABLE_C) $(SYMBOL_TABLE_H)
	$(CC) $(CFLAGS) -c $(SYMBOL_TABLE_C)

clean:
	rm -f $(OBJS) $(LEX_C) $(BISON_C) $(BISON_H) $(EXE)