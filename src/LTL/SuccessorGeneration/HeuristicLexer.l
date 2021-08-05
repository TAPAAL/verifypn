%{
#include <string>
#include <memory>

#include "LTL/SuccessorGeneration/Heuristics.h"
#include "LTL/Structures/BuchiAutomaton.h"
#include "HeuristicParser.parser.hpp"

#define SAVE_TOKEN heurlval.string = new std::string(heurtext, heurleng)
#define SAVE_QUOTED_TOKEN heurlval.string = new std::string(heurtext+1, heurleng-2)
#define TOKEN(t) (heurlval.token = t)

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#endif

#define register      // Deprecated in C++11.
extern "C" int heurwrap(){return 1;}
extern std::unique_ptr<LTL::Heuristic> heuristic;
extern int heurparse();
%}
%option prefix="heur"
%option nounput

digit         [0-9]
letter        [a-zA-Z_]
%%

[ \t\n\r]         ;
"aut" {return TOKEN(AUT);}
"automaton" {return TOKEN(AUT);}
"dist" {return TOKEN(DIST);}
"distance" { return TOKEN(DIST); }
"fc" { return TOKEN(FIRECOUNT); }
"firecount" { return TOKEN(FIRECOUNT); }
"fire-count" { return TOKEN(FIRECOUNT); }
"fire" { return TOKEN(FIRECOUNT); }
"sum" { return TOKEN(SUM);}
"("							{return TOKEN(LPAREN);}
")"							{return TOKEN(RPAREN);}
{letter}({letter}|{digit})* {SAVE_TOKEN; return TEXT;}
{digit}+          {SAVE_TOKEN; return INT;}
"," { return TOKEN(COMMA);}
%%
namespace LTL {
std::unique_ptr<LTL::Heuristic> ParseHeuristic(const PetriEngine::PetriNet *net,
                                               const Structures::BuchiAutomaton &aut,
                                               const PetriEngine::PQL::Condition_ptr &cond,
                                               const char* heurString) {
	//Load up input buffer in Flex
	YY_BUFFER_STATE buf = heur_scan_string(heurString);

	if(heurparse(net, aut, cond) != 0)
		return NULL;

	//Delete the buffer
	heur_delete_buffer(buf);
	return std::move(heuristic);
}
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif
