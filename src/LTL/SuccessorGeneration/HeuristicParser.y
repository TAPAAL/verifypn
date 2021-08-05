
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
	int intval;
}

%name-prefix "heur"
//%expect 1

/* Terminal type definition */
%token <string> TEXT INT
%token <token> SUM DIST AUT FIRECOUNT
%token <token> LPAREN RPAREN COMMA


%type <heur> heurexp;
%type <intval> opt_int;

%start heuristic

%%

heuristic : heurexp { heuristic = std::unique_ptr<Heuristic>($1); }
          | error   { YYABORT; }
          ;

heurexp : AUT                       { $$ = new AutomatonHeuristic(net, aut); }
        | DIST                      { $$ = new DistanceHeuristic(net, cond); }
        | FIRECOUNT INT             { $$ = new LogFireCountHeuristic(net, atol($2->c_str())); delete $2; }
        | FIRECOUNT                 { $$ = new LogFireCountHeuristic(net, 5000); }
        | LPAREN heurexp RPAREN     { $$ = $2; }
        | SUM opt_int[lweight] heurexp[left] opt_int[rweight] heurexp[right]
            { $$ = new WeightedComposedHeuristic(std::unique_ptr<Heuristic>($left),
                                                 std::unique_ptr<Heuristic>($right),
                                                 $2, $4); }
        | SUM LPAREN opt_int[lweight] heurexp[left] COMMA opt_int[rweight] heurexp[right] RPAREN
            { $$ = new WeightedComposedHeuristic(std::unique_ptr<Heuristic>($left),
                                                 std::unique_ptr<Heuristic>($right),
                                                 $lweight, $rweight); }
       ;

opt_int : INT { $$ = atol($1->c_str()); delete $1; }
        | { $$ = 1; }
        ;