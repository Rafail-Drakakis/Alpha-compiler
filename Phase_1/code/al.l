/**
 * HY-340 Project Phase 1 2024-2025
 *
 * Members:
 *      csd5171 Fytaki Maria
 *      csd5310 Rafail Drakakis
 *      csd5082 Theologos Kokkinellis
 */

%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define YY_DECL int alpha_yylex(void* yylval)

#define token_string_compare(val1, val2) strcmp((val1), (val2))

/* Token Structure */
typedef struct alpha_token_t {
    unsigned int line_number;
    unsigned int token_number;
    char *content;
    char *token_type;
    char *data_type;
    struct alpha_token_t *next_t;
} alpha_token_t;

alpha_token_t* list_head = NULL;
int counter_tk_number = 0;
int comment_line_start = 0;
int comment_line_end = 0;


/* Function to insert tokens into the list */
void insert_token(int line, const char* text, const char* type, const char* type_of_data) {
    alpha_token_t* new_token = (alpha_token_t*)malloc(sizeof(alpha_token_t));
    new_token->line_number = line;
    new_token->token_number = ++counter_tk_number;
    new_token->content = strdup(text);
    new_token->token_type = strdup(type);
    new_token->data_type = strdup(type_of_data);
    new_token->next_t = NULL;

    if (!list_head) {
        list_head = new_token;
    } else {
        alpha_token_t* temp = list_head;
        while (temp->next_t)
            temp = temp->next_t;
        temp->next_t = new_token;
    }
}

%}

%option yylineno
%option noyywrap

/* for block comments, before %% */
%x COMMENT

/* Regular Expressions */
id          [a-zA-Z][a-zA-Z_0-9]*
intconst    (0[xX][0-9A-Fa-f]+)|([0-9]+)
realconst   [0-9]+\.[0-9]+(e[+-]?[0-9]+)?
string      \"([^\\n"\\]|\\t|\\n|\\\"|\\\\|\\.)*\"
whitespace  [ \t\r\n]+
comment1    \/\/.*
operator    (\+|\-|\*|\/|=|%|\+\+|\-\-|==|!=|>|<|>=|<=)
keyword     (function|while|for|if|else|continue|break|return|and|or|not|local|true|false|nil)
punctuation (::|\.\.|:|\.|\{|\}|\[|\]|\(|\)|;|,)

%%
{keyword} {
    if (!token_string_compare(yytext, "if"))            { insert_token(yylineno, yytext, "KEYWORD IF", "enumerated");}
    else if (!token_string_compare(yytext, "else"))     { insert_token(yylineno, yytext, "KEYWORD ELSE", "enumerated"); }
    else if (!token_string_compare(yytext, "while"))    { insert_token(yylineno, yytext, "KEYWORD WHILE", "enumerated"); }
    else if (!token_string_compare(yytext, "for"))      { insert_token(yylineno, yytext, "KEYWORD FOR", "enumerated"); }
    else if (!token_string_compare(yytext, "function")) { insert_token(yylineno, yytext, "KEYWORD FUNCTION", "enumerated"); }
    else if (!token_string_compare(yytext, "return"))   { insert_token(yylineno, yytext, "KEYWORD RETURN", "enumerated"); }
    else if (!token_string_compare(yytext, "break"))    { insert_token(yylineno, yytext, "KEYWORD BREAK", "enumerated"); }
    else if (!token_string_compare(yytext, "continue")) { insert_token(yylineno, yytext, "KEYWORD CONTINUE", "enumerated"); }
    else if (!token_string_compare(yytext, "and"))      { insert_token(yylineno, yytext, "KEYWORD AND", "enumerated"); }
    else if (!token_string_compare(yytext, "not"))      { insert_token(yylineno, yytext, "KEYWORD NOT", "enumerated"); }
    else if (!token_string_compare(yytext, "or"))       { insert_token(yylineno, yytext, "KEYWORD OR", "enumerated"); }
    else if (!token_string_compare(yytext, "local"))    { insert_token(yylineno, yytext, "KEYWORD LOCAL", "enumerated"); }
    else if (!token_string_compare(yytext, "true"))     { insert_token(yylineno, yytext, "KEYWORD TRUE", "enumerated"); }
    else if (!token_string_compare(yytext, "false"))    { insert_token(yylineno, yytext, "KEYWORD FALSE", "enumerated"); }
    else if (!token_string_compare(yytext, "nil"))      { insert_token(yylineno, yytext, "KEYWORD NIL", "enumerated"); }
    else {
        insert_token(yylineno, yytext, "KEYWORD", "enumerated");
    }
    return 1;
}

{operator} {
    if (!token_string_compare(yytext, "+"))        { insert_token(yylineno, yytext, "OPERATOR PLUS", "enumerated"); }
    else if (!token_string_compare(yytext, "-"))   { insert_token(yylineno, yytext, "OPERATOR MINUS", "enumerated"); }
    else if (!token_string_compare(yytext, "*"))   { insert_token(yylineno, yytext, "OPERATOR MULT", "enumerated"); }
    else if (!token_string_compare(yytext, "/"))   { insert_token(yylineno, yytext, "OPERATOR DIVIDE", "enumerated"); }
    else if (!token_string_compare(yytext, "%"))   { insert_token(yylineno, yytext, "OPERATOR MODULO", "enumerated"); }
    else if (!token_string_compare(yytext, "++"))  { insert_token(yylineno, yytext, "OPERATOR PLUS_PLUS", "enumerated"); }
    else if (!token_string_compare(yytext, "--"))  { insert_token(yylineno, yytext, "OPERATOR MINUS_MINUS", "enumerated"); }
    else if (!token_string_compare(yytext, "="))   { insert_token(yylineno, yytext, "OPERATOR ASSIGNMENT", "enumerated"); }
    else if (!token_string_compare(yytext, "=="))  { insert_token(yylineno, yytext, "OPERATOR EQUAL_EQUAL", "enumerated"); }
    else if (!token_string_compare(yytext, "!="))  { insert_token(yylineno, yytext, "OPERATOR NOT_EQUAL", "enumerated"); }
    else if (!token_string_compare(yytext, ">"))   { insert_token(yylineno, yytext, "OPERATOR GREATER", "enumerated"); }
    else if (!token_string_compare(yytext, "<"))   { insert_token(yylineno, yytext, "OPERATOR LESS", "enumerated"); }
    else if (!token_string_compare(yytext, ">="))  { insert_token(yylineno, yytext, "OPERATOR GREATER_EQUAL", "enumerated"); }
    else if (!token_string_compare(yytext, "<="))  { insert_token(yylineno, yytext, "OPERATOR LESS_EQUAL", "enumerated"); }
    else {
        /* If none of the above operators doesnt match then just store it as a general OPERATOR. */
        insert_token(yylineno, yytext, "OPERATOR", "enumerated");
    }
    return 1;
}

{punctuation} {
    if (!token_string_compare(yytext, "("))       { insert_token(yylineno, yytext, "PUNCTUATION LEFT_PARENTHESIS", "enumerated"); }
    else if (!token_string_compare(yytext, ")"))  { insert_token(yylineno, yytext, "PUNCTUATION RIGHT_PARENTHESIS", "enumerated"); }
    else if (!token_string_compare(yytext, "["))  { insert_token(yylineno, yytext, "PUNCTUATION LEFT_BRACKET", "enumerated"); }
    else if (!token_string_compare(yytext, "]"))  { insert_token(yylineno, yytext, "PUNCTUATION RIGHT_BRACKET", "enumerated"); }
    else if (!token_string_compare(yytext, "{"))  { insert_token(yylineno, yytext, "PUNCTUATION LEFT_BRACE", "enumerated"); }
    else if (!token_string_compare(yytext, "}"))  { insert_token(yylineno, yytext, "PUNCTUATION RIGHT_BRACE", "enumerated"); }
    else if (!token_string_compare(yytext, ";"))  { insert_token(yylineno, yytext, "PUNCTUATION SEMICOLON", "enumerated"); }
    else if (!token_string_compare(yytext, "."))  { insert_token(yylineno, yytext, "PUNCTUATION DOT", "enumerated"); }
    else if (!token_string_compare(yytext, "..")) { insert_token(yylineno, yytext, "PUNCTUATION DOT_DOT", "enumerated"); }
    else if (!token_string_compare(yytext, "::")) { insert_token(yylineno, yytext, "PUNCTUATION COLON_COLON", "enumerated"); }
    else if (!token_string_compare(yytext, ":"))  { insert_token(yylineno, yytext, "PUNCTUATION COLON", "enumerated"); }
    else if (!token_string_compare(yytext, ","))  { insert_token(yylineno, yytext, "PUNCTUATION COMMA", "enumerated"); }
    else {
        insert_token(yylineno, yytext, "PUNCTUATION", "enumerated");
    }
    return 1;
}

{id}          { insert_token(yylineno, yytext, "IDENTIFIER", "char*"); return 1; }
{intconst}    { insert_token(yylineno, yytext, "INTCONST", "integer"); return 1; }
{realconst}   { insert_token(yylineno, yytext, "REALCONST", "float"); return 1; }
{string} {
    char *processed_str = strdup(yytext);
    
    // Remove outer quotes
    processed_str[strlen(processed_str) - 1] = '\0';
    memmove(processed_str, processed_str + 1, strlen(processed_str));

    // Replace escape sequences
    for (int i = 0; processed_str[i]; i++) {
        if (processed_str[i] == '\\' && processed_str[i + 1]) {
            if (processed_str[i + 1] == 't') {
                processed_str[i] = '\t';
                memmove(&processed_str[i + 1], &processed_str[i + 2], strlen(&processed_str[i + 2]) + 1);
            } else if (processed_str[i + 1] == 'n') {
                processed_str[i] = '\n';
                memmove(&processed_str[i + 1], &processed_str[i + 2], strlen(&processed_str[i + 2]) + 1);
            } else if (processed_str[i + 1] == '"') {
                processed_str[i] = '"';
                memmove(&processed_str[i + 1], &processed_str[i + 2], strlen(&processed_str[i + 2]) + 1);
            } else if (processed_str[i + 1] == '\\') {
                processed_str[i] = '\\';
                memmove(&processed_str[i + 1], &processed_str[i + 2], strlen(&processed_str[i + 2]) + 1);
            }
        }
    }

    insert_token(yylineno, processed_str, "STRING", "char*");
    free(processed_str);
    return 1;
}
{comment1}    { insert_token(yylineno, yytext, "COMMENT LINE_COMMENT", "enumerated"); }

"/*" {	
	comment_line_start = yylineno;
	BEGIN(COMMENT); 			/* front 1 slide 18*/ 
}

<COMMENT>[^*\n]* { 				/* eat anything that is not '*' or nl */ }
<COMMENT>"*"+[^*/] {				/* eat up '*'s not followed by '/'s */ }
<COMMENT>\n { comment_line_end = yylineno;  }
<COMMENT>"*"+"/" {	
	comment_line_end = yylineno; 		/* line eo cmnt */
	char instead_of_content[30];
	snprintf(instead_of_content, sizeof(instead_of_content), "%d - %d", comment_line_start, comment_line_end);
	insert_token(comment_line_start, instead_of_content, "COMMENT BLOCK_COMMENT", "enumerated");
	BEGIN(INITIAL); }

{whitespace}  {  }
.   { /* Ignore unrecognized characters */ }

%%

/* Function to print tokens */
void print_output() {
    alpha_token_t* temp = list_head;
    printf("---------------- Lexical Analysis ----------------\n");
    while (temp) {
        printf("%d: #%d \"%s\" %s <-%s\n", 
		temp->line_number, 
		temp->token_number, 
		temp->content, 
		temp->token_type,
		temp->data_type);
        temp = temp->next_t;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <input file> [output file]\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *input_file = fopen(argv[1], "r");
    if (!input_file) {
        perror("Error opening input file");
        return EXIT_FAILURE;
    }
    yyin = input_file;

    while (alpha_yylex(NULL));
    fclose(input_file);

    if (argc == 3) {
        FILE *output_file = fopen(argv[2], "w");
        if (!output_file) {
            perror("Error opening output file");
            return EXIT_FAILURE;
        }
        freopen(argv[2], "w", stdout);
    }

    print_output();
    return EXIT_SUCCESS;
}