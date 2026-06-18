CC = gcc
FLEX = flex
LEX_FILE = al.l
OUTPUT = al
TEST_DIR = ../tests
OUT_DIR = $(TEST_DIR)/out

all: $(OUTPUT)

$(OUTPUT): lex.yy.c
	$(CC) -o $(OUTPUT) lex.yy.c -lfl

lex.yy.c: $(LEX_FILE)
	$(FLEX) $(LEX_FILE)

run-tests: $(OUTPUT)
	mkdir -p "$(OUT_DIR)"
	pass_count=0; fail_count=0; \
	for file in $(TEST_DIR)/*.alpha; do \
		base=$$(basename "$$file" .alpha); \
		out_file="$(OUT_DIR)/$$base.out"; \
		if ./$(OUTPUT) "$$file" "$$out_file" >/dev/null 2>&1 && \
		   [ -s "$$out_file" ] && \
		   grep -q "^---------------- Lexical Analysis ----------------$$" "$$out_file"; then \
			echo "[PASS] $$base"; \
			pass_count=$$((pass_count + 1)); \
		else \
			echo "[FAIL] $$base"; \
			fail_count=$$((fail_count + 1)); \
		fi; \
	done; \
	echo "Summary: $$pass_count passed, $$fail_count failed"; \
	test $$fail_count -eq 0

.PHONY: all run-tests clean

clean:
	rm -f $(OUTPUT) lex.yy.c
	rm -rf "$(OUT_DIR)"

