/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_H_INCLUDED
# define YY_YY_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    IF = 258,                      /* IF  */
    ELSE = 259,                    /* ELSE  */
    WHILE = 260,                   /* WHILE  */
    FOR = 261,                     /* FOR  */
    RETURN = 262,                  /* RETURN  */
    BREAK = 263,                   /* BREAK  */
    CONTINUE = 264,                /* CONTINUE  */
    LOCAL = 265,                   /* LOCAL  */
    TRUE = 266,                    /* TRUE  */
    FALSE = 267,                   /* FALSE  */
    NIL = 268,                     /* NIL  */
    PLUS = 269,                    /* PLUS  */
    MINUS = 270,                   /* MINUS  */
    MULTIPLY = 271,                /* MULTIPLY  */
    DIVIDE = 272,                  /* DIVIDE  */
    ASSIGNMENT = 273,              /* ASSIGNMENT  */
    EQUAL = 274,                   /* EQUAL  */
    NOT_EQUAL = 275,               /* NOT_EQUAL  */
    GREATER_THAN = 276,            /* GREATER_THAN  */
    GREATER_EQUAL = 277,           /* GREATER_EQUAL  */
    LESS_THAN = 278,               /* LESS_THAN  */
    LESS_EQUAL = 279,              /* LESS_EQUAL  */
    LEFT_PARENTHESIS = 280,        /* LEFT_PARENTHESIS  */
    RIGHT_PARENTHESIS = 281,       /* RIGHT_PARENTHESIS  */
    LEFT_BRACE = 282,              /* LEFT_BRACE  */
    RIGHT_BRACE = 283,             /* RIGHT_BRACE  */
    LEFT_BRACKET = 284,            /* LEFT_BRACKET  */
    RIGHT_BRACKET = 285,           /* RIGHT_BRACKET  */
    SEMICOLON = 286,               /* SEMICOLON  */
    COMMA = 287,                   /* COMMA  */
    COLON = 288,                   /* COLON  */
    IDENTIFIER = 289,              /* IDENTIFIER  */
    INTCONST = 290,                /* INTCONST  */
    REALCONST = 291,               /* REALCONST  */
    STRING = 292,                  /* STRING  */
    FUNCTION = 293,                /* FUNCTION  */
    AND = 294,                     /* AND  */
    OR = 295,                      /* OR  */
    NOT = 296,                     /* NOT  */
    MODULO = 297,                  /* MODULO  */
    PLUS_PLUS = 298,               /* PLUS_PLUS  */
    MINUS_MINUS = 299,             /* MINUS_MINUS  */
    EQUAL_EQUAL = 300,             /* EQUAL_EQUAL  */
    LESS = 301,                    /* LESS  */
    GREATER = 302,                 /* GREATER  */
    DOT = 303,                     /* DOT  */
    DOT_DOT = 304,                 /* DOT_DOT  */
    COLON_COLON = 305,             /* COLON_COLON  */
    PUNCTUATION = 306,             /* PUNCTUATION  */
    OPERATOR = 307,                /* OPERATOR  */
    LOWER_THAN_ELSE = 308          /* LOWER_THAN_ELSE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 27 "parser.y"

    int intValue;
    double realValue;
    char* stringValue;

#line 123 "parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSER_H_INCLUDED  */
