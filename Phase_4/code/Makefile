CC         := gcc
CFLAGS     := -g

LEX        := flex
BISON      := bison

LEX_FILE   := al.l
BISON_FILE := parser.y

BISON_C     := parser.c
BISON_H     := parser.h

LEX_C       := lex.yy.c

MAIN_C        := main.c
SYMBOL_TABLE_C := symbol_table.c
SYMBOL_TABLE_H := symbol_table.h
QUADS_C        := quads.c
QUADS_H        := quads.h
CODEGEN_C      := codegen.c
CODEGEN_H      := codegen.h

OBJS := parser.o \
        lex.yy.o \
        main.o \
        symbol_table.o \
        quads.o \
        codegen.o

EXE := target

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Bison: produce parser.c and parser.h
$(BISON_C) $(BISON_H): $(BISON_FILE)
	$(BISON) -d $< -o $(BISON_C) --defines=$(BISON_H)

# Flex: produce lex.yy.c (must happen after parser.h exists)
$(LEX_C): $(LEX_FILE) $(BISON_H)
	$(LEX) -o $@ $<

# Compile rules
parser.o: $(BISON_C) $(BISON_H)
	$(CC) $(CFLAGS) -c $(BISON_C) -o $@

lex.yy.o: $(LEX_C) $(BISON_H)
	$(CC) $(CFLAGS) -c $(LEX_C) -o $@

main.o: $(MAIN_C) $(SYMBOL_TABLE_H) $(QUADS_H) $(CODEGEN_H) $(BISON_H)
	$(CC) $(CFLAGS) -c $< -o $@

symbol_table.o: $(SYMBOL_TABLE_C) $(SYMBOL_TABLE_H) $(QUADS_H)
	$(CC) $(CFLAGS) -c $< -o $@

quads.o: $(QUADS_C) $(QUADS_H) $(SYMBOL_TABLE_H)
	$(CC) $(CFLAGS) -c $< -o $@

codegen.o: $(CODEGEN_C) $(CODEGEN_H) $(QUADS_H) $(SYMBOL_TABLE_H)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXE) \
	      $(LEX_C) $(BISON_C) $(BISON_H)
