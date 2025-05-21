/**
 * HY-340 Project Phase 3 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

#include <stdlib.h>
#include <stdio.h>
#include "quads.h"

struct lc_stack_t {
   struct lc_stack_t* next;
   unsigned counter;
};

static struct lc_stack_t *lcs_top = 0;
static struct lc_stack_t *lcs_bottom = 0;

static unsigned loop_id_counter = 1;

unsigned loopcounter(void) {
    if (lcs_top != NULL) {
        return lcs_top->counter;
    } else {
        return 0;
    }
}

void push_loopcounter(void) {
    struct lc_stack_t* new_node = malloc(sizeof(struct lc_stack_t));
    new_node->counter = loop_id_counter++;
    new_node->next = lcs_top;
    lcs_top = new_node;
}

void pop_loopcounter(void) {
    if (!lcs_top) return;
    struct lc_stack_t* temp = lcs_top;
    lcs_top = lcs_top->next;
    free(temp);
}

// #define loopcounter \ (lcs_top->counter)
// extern void push_loopcounter (void);
// extern void pop_loopcounter (void);
