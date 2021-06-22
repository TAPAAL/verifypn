
%parse-param { const PetriEngine::PetriNet *net }
             { const LTL::Structures::BuchiAutomaton &aut }
             { const PetriEngine::PQL::Condition_ptr &cond }

%{
#include <stdio.h>
#include <memory>

#include "LTL/SuccessorGeneration/Heuristics.h"
#include "LTL/Structures/BuchiAutomaton.h"

using namespace LTL;

std::unique_ptr<Heuristic> heuristic;
extern int heurlex();
void heurerror(const PetriEngine::PetriNet *,
               const Structures::BuchiAutomaton&,
               const PetriEngine::PQL::Condition_ptr &cond,
               const char *s) {printf("ERROR: %s\n", s); }

%}

%define parse.error detailed
%define parse.lac full

/* Possible data representation */
%union {
    LTL::Heuristic *heur;
	std::string *string;
	int token;
}

%name-prefix "heur"
//%expect 1

/* Terminal type definition */
%token <string> TEXT INT
%token <token> SUM DIST AUT FIRECOUNT
%token <token> LPAREN RPAREN COMMA


%type <heur> heurexp;

%start heuristic

%%

heuristic : heurexp { heuristic = std::unique_ptr<Heuristic>($1); }
          | error   { YYABORT; }
          ;

heurexp : AUT                       { $$ = new WeightedAutomatonHeuristic(net, aut); }
        | DIST                      { $$ = new DistanceHeuristic(net, cond); }
        | FIRECOUNT INT             { $$ = new LogFireCountHeuristic(net, atol($2->c_str())); delete $2; }
        | FIRECOUNT                 { $$ = new LogFireCountHeuristic(net, 5000); }
        | LPAREN heurexp RPAREN     { $$ = $2; }
        | SUM heurexp heurexp                      { $$ = new SumComposedHeuristic(std::unique_ptr<Heuristic>($2),
                                                                                   std::unique_ptr<Heuristic>($3)); }
        | SUM LPAREN heurexp COMMA heurexp RPAREN  { $$ = new SumComposedHeuristic(std::unique_ptr<Heuristic>($3),
                                                                                   std::unique_ptr<Heuristic>($5)); }
        //| error   { yyerrok; }
       ;
