#ifndef OPTIONS_H

#include <ctype.h>
#include <stddef.h>
#include <limits>
#include <set>
#include <string.h>

#include "Reachability/ReachabilitySearch.h"


struct options_t {
//    bool outputtrace = false;
    int kbound = 0;
    char* modelfile = NULL;
    char* queryfile = NULL;
    int enablereduction = 1; // 0 ... disabled,  1 ... aggresive (default), 2 ... k-boundedness preserving
    bool stubbornreduction = true;
    bool querysimplification = false;
    bool siphontrapenabled = false;
    bool statespaceexploration = false;
    bool printstatistics = true;
    size_t memorylimit = 2048;
    std::set<size_t> querynumbers = std::set<size_t>();
    PetriEngine::Reachability::Strategy strategy = PetriEngine::Reachability::HEUR;
    bool mccoutput = false;
    bool trace = false;
    int queryReductionTimeout = 30, lpsolveTimeout = 10;
    int siphontrapTimeout = 0;
    
    void print() {
        if (!printstatistics || mccoutput) {
            return;
        }
        
        string optionsOut;
        
        if (strategy == PetriEngine::Reachability::Strategy::BFS) {
            optionsOut = "\nSearch=BFS";
        } else if (strategy == PetriEngine::Reachability::Strategy::DFS) {
            optionsOut = "\nSearch=DFS";
        } else if (strategy == PetriEngine::Reachability::Strategy::HEUR) {
            optionsOut = "\nSearch=HEUR";
        } else {
            optionsOut = "\nSearch=RDFS";
        }
        
        if (trace) {
            optionsOut += ", Trace=ENABLED";
        } else {
            optionsOut += ", Trace=DISABLED";
        }
        
        if (kbound > 0) {
            optionsOut += ", Token_Bound=" + std::to_string(kbound);
        }
        
        if (statespaceexploration) {
            optionsOut += ", State_Space_Exploration=ENABLED";
        } else {
            optionsOut += ", State_Space_Exploration=DISABLED";
        }
        
        if (enablereduction == 0) {
            optionsOut += ", Structural_Reduction=DISABLED";
        } else if (enablereduction == 1) {
            optionsOut += ", Structural_Reduction=AGGRESSIVE";
        } else {
            optionsOut += ", Structural_Reduction=KBOUND_PRESERVING";
        }
        
        if (stubbornreduction) {
            optionsOut += ", Stubborn_Reduction=ENABLED";
        } else {
            optionsOut += ", Stubborn_Reduction=DISABLED";
        }
        
        if (querysimplification) {
            optionsOut += ", Query_Simplication=ENABLED, SQTimeout=" + std::to_string(queryReductionTimeout);
        } else {
            optionsOut += ", Query_Simplication=DISABLED";
        }
        
        if (siphontrapenabled) {
            optionsOut += ", Siphon_Trap=ENABLED, SPTimeout=" + std::to_string(siphontrapTimeout);
        } else {
            optionsOut += ", Siphon_Trap=DISABLED";
        }
        
        optionsOut += ", LPSolve_Timeout=" + std::to_string(lpsolveTimeout);
        optionsOut += "\n";
        
        std::cout << optionsOut;
        
        return;
    }
};

#endif
