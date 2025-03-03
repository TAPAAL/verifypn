#ifndef OPTIONS_H
#define OPTIONS_H

#include "CTL/Algorithm/AlgorithmTypes.h"
#include "LTL/LTLOptions.h"

#include <ctype.h>
#include <stddef.h>
#include <limits>
#include <set>
#include <sstream>
#include <vector>
#include <iostream>
#include <cstdint>


enum class Strategy {
    BFS,
    DFS,
    HEUR,
    RDFS,
    OverApprox,
    RPFS,
    RandomWalk,
    DEFAULT
};

enum class ColoredSuccessorGeneratorOption {
    FIXED,
    EVEN
};

enum class TraceLevel {
    None,
    Transitions,
    Full
};

enum class TemporalLogic {
    CTL, LTL
};

enum class StatisticsLevel {
    None,
    SearchOnly,
    Full
};

struct options_t {
//    bool outputtrace = false;
    int kbound = 0;
    const char* modelfile = nullptr;
    const char* queryfile = nullptr;
    int enablereduction = 1; // 0 ... disabled,  1 ... aggresive (default), 2 ... k-boundedness preserving, 3 ... selection
    int enablecolreduction = 1;
    std::vector<uint32_t> reductions{8,2,3,4,5,7,9,6,0,1};
    std::vector<uint32_t> colreductions{};
    int reductionTimeout = 60;
    int colReductionTimeout = 30;
    bool stubbornreduction = true;
    bool statespaceexploration = false;
    StatisticsLevel printstatistics = StatisticsLevel::Full;
    std::set<size_t> querynumbers;
    Strategy strategy = Strategy::DEFAULT;
    int queryReductionTimeout = 30, intervalTimeout = 10, partitionTimeout = 5, lpsolveTimeout = 10, initPotencyTimeout = 10;
    TraceLevel trace = TraceLevel::None;
    bool use_query_reductions = true;
    uint32_t siphontrapTimeout = 0;
    uint32_t siphonDepth = 0;
    uint32_t cores = 1;
    bool doVerification = true;
    bool doUnfolding = true;
    int64_t depthRandomWalk = 50000;
    int64_t incRandomWalk = 5000;

    TemporalLogic logic = TemporalLogic::CTL;
    bool noreach = false;
    //CTL Specific options
    bool usedctl = false;
    CTL::CTLAlgorithmType ctlalgorithm = CTL::CZero;
    bool tar = false;
    uint32_t binary_query_io = 0;

    // LTL Specific options
    bool usedltl = false;
    LTL::Algorithm ltlalgorithm = LTL::Algorithm::Tarjan;
    bool ltluseweak = true;
    std::string buchi_out_file;
    LTL::BuchiOutType buchi_out_type = LTL::BuchiOutType::Dot;
    LTL::APCompression ltl_compress_aps = LTL::APCompression::None;
    LTL::LTLPartialOrder ltl_por = LTL::LTLPartialOrder::Automaton;
    LTL::BuchiOptimization buchiOptimization = LTL::BuchiOptimization::Low;
    LTL::LTLHeuristic ltlHeuristic = LTL::LTLHeuristic::Automaton;

    bool replay_trace = false;
    std::string replay_file;


    std::string query_out_file;
    std::string model_out_file;
    std::string model_col_out_file;
    std::string unfolded_out_file;
    std::string unfold_query_out_file;
    bool keep_solved = false;


    //CPN Specific options
    bool cpnOverApprox = false;
    bool computeCFP = true;
    bool computePartition = true;
    bool symmetricVariables = true;
    bool isCPN = false;
    uint32_t seed_offset = 0;
    int max_intervals = 500; //0 disabled
    int max_intervals_reduced = 5;
    bool print_bindings = false;

    bool explicit_colored = false;
    ColoredSuccessorGeneratorOption colored_sucessor_generator = ColoredSuccessorGeneratorOption::EVEN;

    std::string strategy_output;

    size_t seed() { return ++seed_offset; }
    void print(std::ostream& out = std::cout);
    bool parse(int argc, const char** argv);
};
#endif
