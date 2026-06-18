CC = gcc
CFLAGS = -g
FLEX = flex
BISON = bison

LEX_FILE = al.l
BISON_FILE = parser.y
OUTPUT = parser
TEST_DIR = ../tests
WORKING_DIR = $(TEST_DIR)/Working
ERRORS_DIR = $(TEST_DIR)/Errors
OUT_DIR = $(TEST_DIR)/out
ACTUAL_DIR = $(OUT_DIR)/actual
EXPECTED_DIR = $(OUT_DIR)/expected

BISON_C = parser.c
BISON_H = parser.h
LEX_C = lex.yy.c

OBJS = parser.o lex.yy.o main.o symbol_table.o

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(OBJS)

$(BISON_C) $(BISON_H): $(BISON_FILE)
	$(BISON) -d $(BISON_FILE) -o $(BISON_C) --defines=$(BISON_H)

$(LEX_C): $(LEX_FILE) $(BISON_H)
	$(FLEX) -o $(LEX_C) $(LEX_FILE)

parser.o: $(BISON_C)
	$(CC) $(CFLAGS) -c $(BISON_C) -o parser.o

lex.yy.o: $(LEX_C)
	$(CC) $(CFLAGS) -c $(LEX_C) -o lex.yy.o

main.o: main.c symbol_table.h $(BISON_H)
	$(CC) $(CFLAGS) -c main.c

symbol_table.o: symbol_table.c symbol_table.h
	$(CC) $(CFLAGS) -c symbol_table.c

run-tests: $(OUTPUT)
	mkdir -p "$(ACTUAL_DIR)" "$(EXPECTED_DIR)"
	pass_count=0; fail_count=0; \
	for file in $(WORKING_DIR)/*.asc; do \
		base=$$(basename "$$file" .asc); \
		actual="$(ACTUAL_DIR)/$$base.out"; \
		expected="$(EXPECTED_DIR)/$$base.out"; \
		./$(OUTPUT) "$$file" > "$$actual" 2>&1; \
		if grep -q "Error:" "$$actual"; then \
			echo "[FAIL] Working/$$base (contains semantic/syntax errors)"; \
			fail_count=$$((fail_count + 1)); \
		elif [ -f "$$expected" ] && ! diff -u "$$expected" "$$actual" > /dev/null; then \
			echo "[FAIL] Working/$$base (output mismatch vs expected)"; \
			fail_count=$$((fail_count + 1)); \
		else \
			echo "[PASS] Working/$$base"; \
			pass_count=$$((pass_count + 1)); \
		fi; \
	done; \
	for file in $(ERRORS_DIR)/*.asc; do \
		base=$$(basename "$$file" .asc); \
		actual="$(ACTUAL_DIR)/$$base.out"; \
		expected="$(EXPECTED_DIR)/$$base.out"; \
		./$(OUTPUT) "$$file" > "$$actual" 2>&1; \
		if ! grep -q "Error:" "$$actual"; then \
			echo "[FAIL] Errors/$$base (expected at least one error)"; \
			fail_count=$$((fail_count + 1)); \
		elif [ -f "$$expected" ] && ! diff -u "$$expected" "$$actual" > /dev/null; then \
			echo "[FAIL] Errors/$$base (output mismatch vs expected)"; \
			fail_count=$$((fail_count + 1)); \
		else \
			echo "[PASS] Errors/$$base"; \
			pass_count=$$((pass_count + 1)); \
		fi; \
	done; \
	echo "Summary: $$pass_count passed, $$fail_count failed"; \
	test $$fail_count -eq 0

.PHONY: all run-tests clean

clean:
	rm -f $(OUTPUT) $(OBJS) $(BISON_C) $(BISON_H) $(LEX_C)
	rm -rf "$(OUT_DIR)"
