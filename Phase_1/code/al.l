/**
 * HY-340 Project Phase 1 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

/* --------------------------------- code -------------------------------------------- */
%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define YY_DECL int alpha_yylex (void* ylval)

/* from FAQ 8 */
struct alpha_token_t
{
    unsigned int line_number;
    unsigned int token_number;
    char *content;
    char *token_type;
    struct alpha_token_t *next_t;
};

struct alpha_token_t* list_head = NULL;
int counter_tk_number = 0;
%}

/* --------------------------------- parameters -------------------------------------- */
%option yylineno
%option	noyywrap

/* --------------------------- regular expressions ----------------------------------- */

/* from frontistirio 1 */
id          [a-zA-z][a-zA-z_0-9]*
intconst    (0[xX][0-9A-Fa-f]+)|([0-9]+)
realconst   [0-9]*\.[0-9](e|E)?[0-9]*
whitespace  [\r\n \t\v]
// string      \"[^\n"]*\"
comment1    "//".*
// comment2    "#".*
operator    (\+)|(\-)|(\*)|(\/)|(\=)|(\%)|(\+\+)|(\-\-)|(\>)|(\<)|(\>\=)|(\<\=)|(\=\=)|(\!\=)
keyword     (function)|(while)|(for)|(if)|(else)|(continue)|(break)|(return)|(and)|(or)|(not)|(local)|(true)|(false)|(nil)

/* --------------------------------- what are these ? -------------------------------- */
%x COMMENT 
// %% 

/* from FAQ 1 */
void print_output(struct alpha_token_t* list)
{
    printf("debug: no code yet");

    struct alpha_token_t* temp_l = list;

    if (temp_l == NULL) {
        printf("empty list\n");
        
    } else {
        while(temp_l) {
            // TODO: logika tha prepei na kanoyme kapoio elegxo
            // gia to token_type kai analoga na kanoume printf ?
            switch (temp_l->token_type)
            {
            case OPERATOR: 
                printf("%d: #%d '%s' OPERATOR %s\n", temp_l->line_number, temp_l->token_number, temp_l->content, temp_l->token_type); 
                break;
            // TODO: these + add rest + proper spases + actually declare the types T_T
            // case INTCONST: 
            //     printf(""); 
            //     break;
            // case REALCONST: 
            //     printf(""); 
            //     break;
            // case KEYWORD: 
            //     printf(""); 
                break;
            default:
                break;
            }

            temp_l = temp_l->next_t;
        }
    }

}
