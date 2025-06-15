CC         := gcc
CFLAGS     := -g

LEX        := flex
BISON      := bison

BISON_FILE := parser.y
BISON_C    := parser.c
BISON_H    := parser.h

LEX_FILE   := al.l
LEX_C      := lex.yy.c

MAIN_C     := main.c
ST_C       := symbol_table.c
ST_H       := symbol_table.h
QUADS_C    := quads.c
QUADS_H    := quads.h
CODEGEN_C  := codegen.c
CODEGEN_H  := codegen.h
VM_C       := vm.c
VM_H       := vm.h

OBJS := parser.o \
        lex.yy.o \
        main.o \
        symbol_table.o \
        quads.o \
        codegen.o \
        vm.o

TARGET := final

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

# Bison: produce parser.c and parser.h
$(BISON_C) $(BISON_H): $(BISON_FILE)
	$(BISON) -d $< -o $(BISON_C) --defines=$(BISON_H)

# Flex: produce lex.yy.c (depends on parser.h)
$(LEX_C): $(LEX_FILE) $(BISON_H)
	$(LEX) -o $@ $<

# Compile rules
parser.o: $(BISON_C) $(BISON_H)
	$(CC) $(CFLAGS) -c $(BISON_C) -o $@

lex.yy.o: $(LEX_C) $(BISON_H)
	$(CC) $(CFLAGS) -c $(LEX_C) -o $@

main.o: $(MAIN_C) $(ST_H) $(QUADS_H) $(CODEGEN_H) $(VM_H)
	$(CC) $(CFLAGS) -c $< -o $@

symbol_table.o: $(ST_C) $(ST_H) $(QUADS_H)
	$(CC) $(CFLAGS) -c $< -o $@

quads.o: $(QUADS_C) $(QUADS_H) $(ST_H)
	$(CC) $(CFLAGS) -c $< -o $@

codegen.o: $(CODEGEN_C) $(CODEGEN_H) $(QUADS_H) $(ST_H)
	$(CC) $(CFLAGS) -c $< -o $@

vm.o: $(VM_C) $(VM_H) $(CODEGEN_H)
	$(CC) $(CFLAGS) -c $(VM_C) -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(LEX_C) $(BISON_C) $(BISON_H) out.txt out.bin
