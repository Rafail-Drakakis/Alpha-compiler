LEX = flex
CFLAGS = -Wall -g

LEX_FILE = al.l
LEX_C = lex.yy.c
MAIN_C = main.c
SYMBOL_TABLE_C = symbol_table.c
SYMBOL_TABLE_H = symbol_table.h
OBJS = lex.yy.o main.o symbol_table.o
EXE = parser

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

lex.yy.c: $(LEX_FILE)
	$(LEX) $(LEX_FILE)

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c lex.yy.c

main.o: main.c $(SYMBOL_TABLE_H)
	$(CC) $(CFLAGS) -c main.c

symbol_table.o: symbol_table.c $(SYMBOL_TABLE_H)
	$(CC) $(CFLAGS) -c symbol_table.c

clean:
	rm -f $(OBJS) $(LEX_C) $(EXE)

