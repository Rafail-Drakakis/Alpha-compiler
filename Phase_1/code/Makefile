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

