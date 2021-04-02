#ifndef OPTIONS_H
#define OPTIONS_H

#include <ctype.h>
#include <stddef.h>
#include <limits>
#include <set>
#include <sstream>

#include "Reachability/ReachabilitySearch.h"
#include "../CTL/Algorithm/AlgorithmTypes.h"
#include "../LTL/AlgorithmTypes.h"


enum class TemporalLogic {
    CTL, LTL
};

enum class TraceLevel {
    None,
    Transitions,
    Full
};

struct options_t {
//    bool outputtrace = false;
    int kbound = 0;
    char* modelfile = nullptr;
    char* queryfile = nullptr;
    int enablereduction = 1; // 0 ... disabled,  1 ... aggresive (default), 2 ... k-boundedness preserving, 3 ... selection
    std::vector<uint32_t> reductions{8,2,3,4,5,7,9,6,0,1};
    int reductionTimeout = 60;
    bool stubbornreduction = true; 
    bool statespaceexploration = false;
    bool printstatistics = true;
    std::set<size_t> querynumbers;
    PetriEngine::Reachability::Strategy strategy = PetriEngine::Reachability::DEFAULT;
    int queryReductionTimeout = 30, intervalTimeout = 10, lpsolveTimeout = 10;
    TraceLevel trace = TraceLevel::None;
    bool use_query_reductions = true;
    uint32_t siphontrapTimeout = 0;
    uint32_t siphonDepth = 0;
    uint32_t cores = 1;
    std::string output_stats;

    TemporalLogic logic = TemporalLogic::CTL;
    bool noreach = false;
    //CTL Specific options
    bool gamemode = false;
    bool usedctl = false;
    CTL::CTLAlgorithmType ctlalgorithm = CTL::CZero;
    bool tar = false;
    uint32_t binary_query_io = 0;

    // LTL Specific options
    bool usedltl = false;
    LTL::Algorithm ltlalgorithm = LTL::Algorithm::Tarjan;
    bool ltluseweak = true;

    std::string query_out_file;
    std::string model_out_file;

    
    //CPN Specific options
    bool cpnOverApprox = false;
    bool isCPN = false;
    uint32_t seed_offset = 0;
    int max_intervals = 250;
    int max_intervals_reduced = 5;

    void print() {
        if (!printstatistics) {
            return;
        }
        
        std::stringstream optionsOut;
        
        if (strategy == PetriEngine::Reachability::Strategy::BFS) {
            optionsOut << "\nSearch=BFS";
        } else if (strategy == PetriEngine::Reachability::Strategy::DFS) {
            optionsOut << "\nSearch=DFS";
        } else if (strategy == PetriEngine::Reachability::Strategy::HEUR) {
            optionsOut << "\nSearch=HEUR";
        } else if (strategy == PetriEngine::Reachability::Strategy::RDFS){
            optionsOut << "\nSearch=RDFS";
        } else {
            optionsOut << "\nSearch=OverApprox";
        }
        
        if (trace != TraceLevel::None) {
            optionsOut << ",Trace=ENABLED";
        } else {
            optionsOut << ",Trace=DISABLED";
        }
        
        if (kbound > 0) {
            optionsOut << ",Token_Bound=" << kbound;
        }
        
        if (statespaceexploration) {
            optionsOut << ",State_Space_Exploration=ENABLED";
        } else {
            optionsOut << ",State_Space_Exploration=DISABLED";
        }
        
        if (enablereduction == 0) {
            optionsOut << ",Structural_Reduction=DISABLED";
        } else if (enablereduction == 1) {
            optionsOut << ",Structural_Reduction=AGGRESSIVE";
        } else {
            optionsOut << ",Structural_Reduction=KBOUND_PRESERVING";
        }
        
        optionsOut << ",Struct_Red_Timout=" << reductionTimeout;
        
        if (stubbornreduction) {
            optionsOut << ",Stubborn_Reduction=ENABLED";
        } else {
            optionsOut << ",Stubborn_Reduction=DISABLED";
        }
        
        if (queryReductionTimeout > 0) {
            optionsOut << ",Query_Simplication=ENABLED,QSTimeout=" << queryReductionTimeout;
        } else {
            optionsOut << ",Query_Simplication=DISABLED";
        }
        
        if (siphontrapTimeout > 0) {
            optionsOut << ",Siphon_Trap=ENABLED,SPTimeout=" << siphontrapTimeout;
        } else {
            optionsOut << ",Siphon_Trap=DISABLED";
        }
        
        optionsOut << ",LPSolve_Timeout=" << lpsolveTimeout;
        

        if (usedctl) {
            if (ctlalgorithm == CTL::CZero) {
                optionsOut << ",CTLAlgorithm=CZERO";
            } else {
                optionsOut << ",CTLAlgorithm=LOCAL";
            }
        }
        else if (usedltl) {
            switch (ltlalgorithm) {
                case LTL::Algorithm::NDFS:
                    optionsOut << ",LTLAlgorithm=NDFS";
                    break;
                case LTL::Algorithm::Tarjan:
                    optionsOut << ",LTLAlgorithm=Tarjan";
                    break;
                case LTL::Algorithm::None:
                    optionsOut << ",LTLAlgorithm=None";
                    break;
            }
        }

        optionsOut << "\n";
        
        std::cout << optionsOut.str();
    }
};

#endif
