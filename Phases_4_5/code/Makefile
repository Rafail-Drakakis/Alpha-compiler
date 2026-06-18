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
TEST_DIR := ../tests
OUT_DIR := $(TEST_DIR)/out
TIMEOUT  := timeout
ERR_TMO  := 10s
STD_TMO  := 30s

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

run-tests: $(TARGET)
	mkdir -p "$(OUT_DIR)"
	pass_count=0; fail_count=0; \
	for file in $(TEST_DIR)/*.asc; do \
		base=$$(basename "$$file"); \
		out_file="$(OUT_DIR)/$${base%.asc}.out"; \
		tmo="$(STD_TMO)"; \
		if echo "$$base" | grep -q '_err_'; then tmo="$(ERR_TMO)"; fi; \
		if [ "$$base" = "16_libfuncs_bonus1.asc" ]; then \
			printf '3\n3.4\ntrue\nfalse\np[\n4.4a\n"gap gap gap"\nexit\n' | \
			$(TIMEOUT) $$tmo ./$(TARGET) "$$file" > "$$out_file" 2>&1; ec=$$?; \
		elif [ "$$base" = "17_libfuncs_bonus2_part2_tables_catch_self_reference_error.asc" ]; then \
			printf 'n\n' | \
			$(TIMEOUT) $$tmo ./$(TARGET) "$$file" > "$$out_file" 2>&1; ec=$$?; \
		else \
			$(TIMEOUT) $$tmo ./$(TARGET) "$$file" > "$$out_file" 2>&1; ec=$$?; \
		fi; \
		if echo "$$base" | grep -q '_err_'; then \
			if [ $$ec -ne 0 ]; then \
				echo "[PASS] $$base (expected failure, exit=$$ec)"; \
				pass_count=$$((pass_count + 1)); \
			else \
				echo "[FAIL] $$base (expected failure, exit=$$ec)"; \
				fail_count=$$((fail_count + 1)); \
			fi; \
		else \
			if [ $$ec -eq 0 ]; then \
				echo "[PASS] $$base"; \
				pass_count=$$((pass_count + 1)); \
			else \
				echo "[FAIL] $$base (unexpected exit=$$ec)"; \
				fail_count=$$((fail_count + 1)); \
			fi; \
		fi; \
	done; \
	echo ""; \
	echo "Summary: PASS=$$pass_count FAIL=$$fail_count"; \
	test $$fail_count -eq 0

.PHONY: all run-tests clean

clean:
	rm -f $(TARGET) $(OBJS) $(BISON_C) $(BISON_H) $(LEX_C) out.txt out.abc
	rm -rf "$(OUT_DIR)"
