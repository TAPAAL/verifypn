/* A Bison parser, made by GNU Bison 2.7.12-4996.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

#ifndef YY_PQLQ_PETRIENGINE_PQL_PQLQUERYPARSER_PARSER_HPP_INCLUDED
# define YY_PQLQ_PETRIENGINE_PQL_PQLQUERYPARSER_PARSER_HPP_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int pqlqdebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ID = 258,
     INT = 259,
     DEADLOCK = 260,
     TRUE = 261,
     FALSE = 262,
     LPAREN = 263,
     RPAREN = 264,
     AND = 265,
     OR = 266,
     NOT = 267,
     EQUAL = 268,
     NEQUAL = 269,
     LESS = 270,
     LESSEQUAL = 271,
     GREATER = 272,
     GREATEREQUAL = 273,
     PLUS = 274,
     MINUS = 275,
     MULTIPLY = 276
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2053 of yacc.c  */
#line 16 "PetriEngine/PQL/PQLQueryParser.y"

	PetriEngine::PQL::Expr* expr;
	PetriEngine::PQL::Condition* cond;
	std::string *string;
	int token;


/* Line 2053 of yacc.c  */
#line 86 "PetriEngine/PQL/PQLQueryParser.parser.hpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE pqlqlval;
extern YYLTYPE pqlqlloc;
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int pqlqparse (void *YYPARSE_PARAM);
#else
int pqlqparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int pqlqparse (void);
#else
int pqlqparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_PQLQ_PETRIENGINE_PQL_PQLQUERYPARSER_PARSER_HPP_INCLUDED  */
