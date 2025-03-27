LEX = flex

LEX_FILE = al.l
LEX_C = lex.yy.c
MAIN_C = main.c
SYMBOL_TABLE_C = symbol_table.c
SYMBOL_TABLE_H = symbol_table.h
OBJS = lex.yy.o main.o symbol_table.o
EXE = parser

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS)

lex.yy.c: $(LEX_FILE)
	$(LEX) $(LEX_FILE)

lex.yy.o: lex.yy.c
	$(CC) -c lex.yy.c

main.o: main.c $(SYMBOL_TABLE_H)
	$(CC) -c main.c

symbol_table.o: symbol_table.c $(SYMBOL_TABLE_H)
	$(CC) -c symbol_table.c

clean:
	rm -f $(OBJS) $(LEX_C) $(EXE)

