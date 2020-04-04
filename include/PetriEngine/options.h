#ifndef OPTIONS_H
#define OPTIONS_H

#include <ctype.h>
#include <stddef.h>
#include <limits>
#include <set>
#include <string.h>

#include "Reachability/ReachabilitySearch.h"
#include "../CTL/Algorithm/AlgorithmTypes.h"

struct options_t {
//    bool outputtrace = false;
    int kbound = 0;
    char* modelfile = NULL;
    char* queryfile = NULL;
    int enablereduction = 1; // 0 ... disabled,  1 ... aggresive (default), 2 ... k-boundedness preserving, 3 ... selection
    std::vector<uint32_t> reductions{8,2,3,4,5,7,9,6,0,1};
    int reductionTimeout = 60;
    bool stubbornreduction = true; 
    bool statespaceexploration = false;
    bool printstatistics = true;
    std::set<size_t> querynumbers;
    PetriEngine::Reachability::Strategy strategy = PetriEngine::Reachability::DEFAULT;
    bool trace = false;
    int queryReductionTimeout = 30, lpsolveTimeout = 10;
    uint32_t siphontrapTimeout = 0;
    uint32_t siphonDepth = 0;
    uint32_t cores = 1;

    //CTL Specific options
    bool gamemode = false;
    bool isctl = false;
    CTL::CTLAlgorithmType ctlalgorithm = CTL::CZero;
    bool tar = false;
    uint32_t binary_query_io = 0;
    
    std::string query_out_file;
    std::string model_out_file;

    
    //CPN Specific options
    bool cpnOverApprox = false;
    bool isCPN = false;
    uint32_t seed_offset = 0;

    void print() {
        if (!printstatistics) {
            return;
        }
        
        string optionsOut;
        
        if (strategy == PetriEngine::Reachability::Strategy::BFS) {
            optionsOut = "\nSearch=BFS";
        } else if (strategy == PetriEngine::Reachability::Strategy::DFS) {
            optionsOut = "\nSearch=DFS";
        } else if (strategy == PetriEngine::Reachability::Strategy::HEUR) {
            optionsOut = "\nSearch=HEUR";
        } else if (strategy == PetriEngine::Reachability::Strategy::RDFS){
            optionsOut = "\nSearch=RDFS";
        } else {
            optionsOut = "\nSearch=OverApprox";
        }
        
        if (trace) {
            optionsOut += ",Trace=ENABLED";
        } else {
            optionsOut += ",Trace=DISABLED";
        }
        
        if (kbound > 0) {
            optionsOut += ",Token_Bound=" + std::to_string(kbound);
        }
        
        if (statespaceexploration) {
            optionsOut += ",State_Space_Exploration=ENABLED";
        } else {
            optionsOut += ",State_Space_Exploration=DISABLED";
        }
        
        if (enablereduction == 0) {
            optionsOut += ",Structural_Reduction=DISABLED";
        } else if (enablereduction == 1) {
            optionsOut += ",Structural_Reduction=AGGRESSIVE";
        } else {
            optionsOut += ",Structural_Reduction=KBOUND_PRESERVING";
        }
        
        optionsOut += ",Struct_Red_Timout=" + std::to_string(reductionTimeout);
        
        if (stubbornreduction) {
            optionsOut += ",Stubborn_Reduction=ENABLED";
        } else {
            optionsOut += ",Stubborn_Reduction=DISABLED";
        }
        
        if (queryReductionTimeout > 0) {
            optionsOut += ",Query_Simplication=ENABLED,QSTimeout=" + std::to_string(queryReductionTimeout);
        } else {
            optionsOut += ",Query_Simplication=DISABLED";
        }
        
        if (siphontrapTimeout > 0) {
            optionsOut += ",Siphon_Trap=ENABLED,SPTimeout=" + std::to_string(siphontrapTimeout);
        } else {
            optionsOut += ",Siphon_Trap=DISABLED";
        }
        
        optionsOut += ",LPSolve_Timeout=" + std::to_string(lpsolveTimeout);
        

        if (ctlalgorithm == CTL::CZero) {
            optionsOut += ",CTLAlgorithm=CZERO";
        } else {
            optionsOut += ",CTLAlgorithm=LOCAL";
        }
        
        optionsOut += "\n";
        
        std::cout << optionsOut;
    }
};

#endif
