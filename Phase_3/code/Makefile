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
QUADS_C = quads.c
QUADS_H = quads.h

OBJS = parser.o lex.yy.o main.o symbol_table.o quads.o loop_control.o
EXE = intermediate

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

main.o: $(MAIN_C) $(SYMBOL_TABLE_H) $(QUADS_H) $(BISON_H)
	$(CC) $(CFLAGS) -c $(MAIN_C) -o main.o

symbol_table.o: $(SYMBOL_TABLE_C) $(SYMBOL_TABLE_H)
	$(CC) $(CFLAGS) -c $(SYMBOL_TABLE_C) -o symbol_table.o

loop_control.o: loop_control.c loop_control.h
	$(CC) $(CFLAGS) -c loop_control.c -o loop_control.o

quads.o: $(QUADS_C) $(QUADS_H)
	$(CC) $(CFLAGS) -c $(QUADS_C) -o quads.o

clean:
	rm -f $(OBJS) $(LEX_C) $(BISON_C) $(BISON_H) $(EXE) parser.tab.c parser.tab.h

