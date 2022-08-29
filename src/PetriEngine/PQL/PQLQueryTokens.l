%{
#include <string>
#include <memory>
#include "PetriEngine/PQL/PQL.h"
#include "PQLQueryParser.parser.hpp"

#define SAVE_TOKEN pqlqlval.string = new std::string(pqlqtext, pqlqleng)
#define SAVE_QUOTED_TOKEN pqlqlval.string = new std::string(pqlqtext+1, pqlqleng-2)
#define TOKEN(t) (pqlqlval.token = t)

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#endif

#define register      // Deprecated in C++11.
extern "C" int pqlqwrap(){return 1;}
extern PetriEngine::PQL::Condition* query;
extern int pqlqparse();
%}
%option prefix="pqlq"
%option nounput

digit         [0-9]
letter        [a-zA-Z_]
%%

[ \t\n\r]         ;
"true"            {return TOKEN(TRUE);}
"false"           {return TOKEN(FALSE);}
"deadlock"          {return TOKEN(DEADLOCK);}
"and"           {return TOKEN(AND);}
"AND"           {return TOKEN(AND);}
"or"            {return TOKEN(OR);}
"OR"            {return TOKEN(OR);}
"not"           {return TOKEN(NOT);}
"NOT"           {return TOKEN(NOT);}
"EF"						{return TOKEN(EF);}
"EG"						{return TOKEN(EG);}
"AF"						{return TOKEN(AF);}
"AG"						{return TOKEN(AG);}
"EX"						{return TOKEN(EX);}
"AX"						{return TOKEN(AX);}
"A"						{return TOKEN(A);}
"E"						{return TOKEN(E);}
"X"						{return TOKEN(X);}
"F"						{return TOKEN(F);}
"G"						{return TOKEN(G);}
"U"						{return TOKEN(U);}
"control"				{return TOKEN(CONTROL);}
"exists"				{return TOKEN(EXISTS);}
"forall"				{return TOKEN(FORALL);}
"."                     {return TOKEN(DOT);}
"NOT" {return TOKEN(NOT);}
{letter}({letter}|{digit})* {SAVE_TOKEN; return ID;}
{digit}+          {SAVE_TOKEN; return INT;}
(\"(\\.|[^"])*\") {SAVE_QUOTED_TOKEN; return ID;}
"&"|"&"?					{return TOKEN(AND);}
"|""|"?						{return TOKEN(OR);}
"!"							{return TOKEN(NOT);}
"=="						{return TOKEN(EQUAL);}
"!="						{return TOKEN(NEQUAL);}
"<"							{return TOKEN(LESS);}
"<="						{return TOKEN(LESSEQUAL);}
">"							{return TOKEN(GREATER);}
">="						{return TOKEN(GREATEREQUAL);}
"("							{return TOKEN(LPAREN);}
")"							{return TOKEN(RPAREN);}
"+"							{return TOKEN(PLUS);}
"-"							{return TOKEN(MINUS);}
"*"							{return TOKEN(MULTIPLY);}
"="							{return TOKEN(EQUAL);}
"?"                         {return TOKEN(QUESTIONMARK);}
"is-fireable" {return TOKEN(FIREABLE);}
"tokens-count"|"#tokens" {return TOKEN(TOKENCOUNT); }
"," {return TOKEN(COMMA); }
":" {return TOKEN(COLON); }
.							{printf("Unknown token %s!\n", pqlqtext); yyterminate();}


%%
namespace PetriEngine{ namespace PQL {
std::shared_ptr<Condition> ParseQuery(const std::string& queryString) {
	//Load up input buffer in Flex
	YY_BUFFER_STATE buf = pqlq_scan_string(queryString.c_str());

	if(pqlqparse() != 0)
		return nullptr;

	//Delete the buffer
	pqlq_delete_buffer(buf);
	return Condition_ptr(query);
}
}}
#ifdef __clang__
#pragma clang diagnostic pop
#endif
