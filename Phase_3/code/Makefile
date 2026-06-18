CC = gcc
CFLAGS = -g

LEX = flex
BISON = bison

LEX_FILE = al.l
BISON_FILE = parser.y
TEST_DIR = ../tests
OUT_DIR = $(TEST_DIR)/out
ACTUAL_DIR = $(OUT_DIR)/actual
EXPECTED_DIR = $(OUT_DIR)/expected

BISON_C = parser.c
BISON_H = parser.h

LEX_C = lex.yy.c

MAIN_C = main.c
SYMBOL_TABLE_C = symbol_table.c
SYMBOL_TABLE_H = symbol_table.h
QUADS_C = quads.c
QUADS_H = quads.h

OBJS = parser.o lex.yy.o main.o symbol_table.o quads.o
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

quads.o: $(QUADS_C) $(QUADS_H)
	$(CC) $(CFLAGS) -c $(QUADS_C) -o quads.o

run-tests: $(EXE)
	mkdir -p "$(ACTUAL_DIR)" "$(EXPECTED_DIR)"
	pass_count=0; fail_count=0; \
	for file in $(TEST_DIR)/*.asc; do \
		base=$$(basename "$$file" .asc); \
		actual="$(ACTUAL_DIR)/$$base.out"; \
		expected="$(EXPECTED_DIR)/$$base.out"; \
		rm -f quads.txt; \
		./$(EXE) "$$file" > "$$actual" 2>&1; \
		if echo "$$base" | grep -Eq "(^|_)error"; then \
			if ! grep -q "Error:" "$$actual"; then \
				echo "[FAIL] $$base (expected at least one error)"; \
				fail_count=$$((fail_count + 1)); \
			elif [ -f "$$expected" ] && ! diff -u "$$expected" "$$actual" > /dev/null; then \
				echo "[FAIL] $$base (output mismatch vs expected)"; \
				fail_count=$$((fail_count + 1)); \
			else \
				echo "[PASS] $$base"; \
				pass_count=$$((pass_count + 1)); \
			fi; \
		else \
			if grep -q "Error:" "$$actual"; then \
				echo "[FAIL] $$base (contains semantic/syntax errors)"; \
				fail_count=$$((fail_count + 1)); \
			elif [ ! -f quads.txt ]; then \
				echo "[FAIL] $$base (quads.txt not produced)"; \
				fail_count=$$((fail_count + 1)); \
			elif [ -f "$$expected" ] && ! diff -u "$$expected" "$$actual" > /dev/null; then \
				echo "[FAIL] $$base (output mismatch vs expected)"; \
				fail_count=$$((fail_count + 1)); \
			else \
				echo "[PASS] $$base"; \
				pass_count=$$((pass_count + 1)); \
			fi; \
		fi; \
	done; \
	echo "Summary: $$pass_count passed, $$fail_count failed"; \
	test $$fail_count -eq 0

.PHONY: all run-tests clean

clean:
	rm -f $(EXE) $(OBJS) $(BISON_C) $(BISON_H) $(LEX_C) quads.txt
	rm -rf "$(OUT_DIR)"

