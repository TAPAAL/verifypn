#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <memory>
#include <utility>
#include <functional>
#include <PetriEngine/Colored/ColoredPetriNetBuilder.h>

#ifdef VERIFYPN_MC_Simplification
#include <thread>
#include <iso646.h>
#endif

#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "PetriEngine/TAR/TARReachability.h"
#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"
#include "PetriParse/PNMLParser.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/options.h"
#include "PetriEngine/errorcodes.h"
#include "PetriEngine/STSolver.h"
#include "PetriEngine/Simplification/Member.h"
#include "PetriEngine/Simplification/LinearPrograms.h"
#include "PetriEngine/Simplification/Retval.h"

#include "CTL/CTLEngine.h"
#include "PetriEngine/PQL/Expressions.h"
#include "LTL/LTLValidator.h"
#include "LTL/LTL_algorithm/NestedDepthFirstSearch.h"
#include "LTL/LTL_algorithm/TarjanModelChecker.h"
#include "LTL/LTL.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

std::vector<std::string> explode(std::string const & s)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, ','); )
    {
        result.push_back(std::move(token));
        if(result.back().empty()) result.pop_back();
    }

    return result;
}

ReturnValue parseOptions(int argc, char* argv[], options_t& options)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--k-bound") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.kbound) != 1 || options.kbound < 0) {
                fprintf(stderr, "Argument Error: Invalid number of tokens \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--search-strategy") == 0){
            if (i==argc-1) {
                fprintf(stderr, "Missing search strategy after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            char* s = argv[++i];
            if(strcmp(s, "BestFS") == 0)
                options.strategy = HEUR;
            else if(strcmp(s, "BFS") == 0)
                options.strategy = BFS;
            else if(strcmp(s, "DFS") == 0)
                options.strategy = DFS;
            else if(strcmp(s, "RDFS") == 0)
                options.strategy = RDFS;
            else if(strcmp(s, "OverApprox") == 0)
                options.strategy = OverApprox;
            else{
                fprintf(stderr, "Argument Error: Unrecognized search strategy \"%s\"\n", s);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--query-reduction") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.queryReductionTimeout) != 1 || options.queryReductionTimeout < 0) {
                fprintf(stderr, "Argument Error: Invalid query reduction timeout argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--lpsolve-timeout") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.lpsolveTimeout) != 1 || options.lpsolveTimeout < 0) {
                fprintf(stderr, "Argument Error: Invalid LPSolve timeout argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--state-space-exploration") == 0) {
            options.statespaceexploration = true;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-statistics") == 0) {
            options.printstatistics = false;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0) {
            options.trace = true;
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--xml-queries") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            std::vector<std::string> q = explode(argv[++i]);
            for(auto& qn : q)
            {
                int32_t n;
                if(sscanf(qn.c_str(), "%d", &n) != 1 || n <= 0)
                {
                    std::cerr << "Error in query numbers : " << qn << std::endl;
                }
                else
                {
                    options.querynumbers.insert(--n);
                }
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reduction") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.enablereduction) != 1 || options.enablereduction < 0 || options.enablereduction > 3) {
                fprintf(stderr, "Argument Error: Invalid reduction argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
            if(options.enablereduction == 3)
            {
                options.reductions.clear();
                std::vector<std::string> q = explode(argv[++i]);
                for(auto& qn : q)
                {
                    int32_t n;
                    if(sscanf(qn.c_str(), "%d", &n) != 1 || n < 0 || n > 9)
                    {
                        std::cerr << "Error in reduction rule choice : " << qn << std::endl;
                        return ErrorCode;
                    }
                    else
                    {
                        options.reductions.push_back(n);
                    }
                }
            }
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--reduction-timeout") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.reductionTimeout) != 1) {
                fprintf(stderr, "Argument Error: Invalid reduction timeout argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if(strcmp(argv[i], "--seed-offset") == 0) {
            if (sscanf(argv[++i], "%u", &options.seed_offset) != 1) {
                fprintf(stderr, "Argument Error: Invalid seed offset argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        }  else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--partial-order-reduction") == 0) {
            options.stubbornreduction = false;
        } else if(strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--siphon-trap") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%u", &options.siphontrapTimeout) != 1) {
                fprintf(stderr, "Argument Error: Invalid siphon-trap timeout \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if(strcmp(argv[i], "--siphon-depth") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%u", &options.siphonDepth) != 1) {
                fprintf(stderr, "Argument Error: Invalid siphon-depth count \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        }
        else if (strcmp(argv[i], "-tar") == 0)
        {
            options.tar = true;

        }
        else if (strcmp(argv[i], "--write-simplified") == 0)
        {
            options.query_out_file = std::string(argv[++i]);
        }
        else if(strcmp(argv[i], "--binary-query-io") == 0)
        {
            if (sscanf(argv[++i], "%u", &options.binary_query_io) != 1 || options.binary_query_io > 3) {
                fprintf(stderr, "Argument Error: Invalid binary-query-io value \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        }
        else if (strcmp(argv[i], "--write-reduced") == 0)
        {
            options.model_out_file = std::string(argv[++i]);
        }
#ifdef VERIFYPN_MC_Simplification
            else if (strcmp(argv[i], "-z") == 0)
        {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%u", &options.cores) != 1) {
                fprintf(stderr, "Argument Error: Invalid cores count \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        }
#endif
        else if (strcmp(argv[i], "-ctl") == 0){
            if(argc > i + 1){
                if(strcmp(argv[i + 1], "local") == 0){
                    options.ctlalgorithm = CTL::Local;
                }
                else if(strcmp(argv[i + 1], "czero") == 0){
                    options.ctlalgorithm = CTL::CZero;
                }
                else
                {
                    fprintf(stderr, "Argument Error: Invalid ctl-algorithm type \"%s\"\n", argv[i + 1]);
                    return ErrorCode;
                }
                options.isctl = true;
                options.isltl = false;
                i++;

            }
        } else if (strcmp(argv[i], "-ltl") == 0) {
            if(argc > i + 1){
                if(strcmp(argv[i + 1], "ndfs") == 0){
                    options.ltlalgorithm = LTL::Algorithm::NDFS;
                }
                else if(strcmp(argv[i + 1], "tarjan") == 0){
                    options.ltlalgorithm = LTL::Algorithm::Tarjan;
                }
                else {
                    fprintf(stderr, "Argument Error: Invalid ltl-algorithm type \"%s\"\n", argv[i + 1]);
                    return ErrorCode;
                }
                options.isctl = false;
                options.isltl = true;
                i++;
            }
        }

        else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--game-mode") == 0){
            options.gamemode = true;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cpn-overapproximation") == 0) {
            options.cpnOverApprox = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: verifypn [options] model-file query-file\n"
                   "A tool for answering CTL and reachability queries of place cardinality\n"
                   "deadlock and transition fireability for weighted P/T Petri nets\n"
                   "extended with inhibitor arcs.\n"
                   "\n"
                   "Options:\n"
                   "  -k, --k-bound <number of tokens>   Token bound, 0 to ignore (default)\n"
                   "  -t, --trace                        Provide XML-trace to stderr\n"
                   "  -s, --search-strategy <strategy>   Search strategy:\n"
                   "                                     - BestFS       Heuristic search (default)\n"
                   "                                     - BFS          Breadth first search\n"
                   "                                     - DFS          Depth first search (CTL default)\n"
                   "                                     - RDFS         Random depth first search\n"
                   "                                     - OverApprox   Linear Over Approx\n"
                   "  --seed-offset <number>             Extra noise to add to the seed of the random number generation\n"
                   "  -e, --state-space-exploration      State-space exploration only (query-file is irrelevant)\n"
                   "  -x, --xml-query <query index>      Parse XML query file and verify query of a given index\n"
                   "  -r, --reduction <type>             Change structural net reduction:\n"
                   "                                     - 0  disabled\n"
                   "                                     - 1  aggressive reduction (default)\n"
                   "                                     - 2  reduction preserving k-boundedness\n"
                   "                                     - 3  user defined reduction sequence, eg -r 3 0,1,2,3 to use rules A,B,C,D only, and in that order\n"
                   "  -d, --reduction-timeout <timeout>  Timeout for structural reductions in seconds (default 60)\n"
                   "  -q, --query-reduction <timeout>    Query reduction timeout in seconds (default 30)\n"
                   "                                     write -q 0 to disable query reduction\n"
                   "  -l, --lpsolve-timeout <timeout>    LPSolve timeout in seconds, default 10\n"
                   "  -p, --partial-order-reduction      Disable partial order reduction (stubborn sets)\n"
                   "  -a, --siphon-trap <timeout>        Siphon-Trap analysis timeout in seconds (default 0)\n"
                   "      --siphon-depth <place count>   Search depth of siphon (default 0, which counts all places)\n"
                   "  -n, --no-statistics                Do not display any statistics (default is to display it)\n"
                   "  -h, --help                         Display this help message\n"
                   "  -v, --version                      Display version information\n"
                   "  -ctl <type>                        Verify CTL properties\n"
                   "                                     - local     Liu and Smolka's on-the-fly algorithm\n"
                   "                                     - czero     local with certain zero extension (default)\n"
                   "  -ltl <type>                        Verify LTL properties\n"
                   "                                     - ndfs      Nested Depth First Search\n"
                   "                                     - tarjan    On-the-fly variant of Tarjan's algorithm\n"
                   "                                     "
                   "  -c, --cpn-overapproximation        Over approximate query on Colored Petri Nets (CPN only)\n"
                   //"  -g                                 Enable game mode (CTL Only)" // Feature not yet implemented
                   #ifdef VERIFYPN_MC_Simplification
                   "  -z <number of cores>               Number of cores to use (currently only query simplification)\n"
                   #endif
                   "  -tar                               Enables Trace Abstraction Refinement for reachability properties\n"
                   "  --write-simplified <filename>      Outputs the queries to the given file after simplification\n"
                   "  --write-reduced <filename>         Outputs the model to the given file after structural reduction\n"
                   "  --binary-query-io <0,1,2,3>        Determines the input/output format of the query-file\n"
                   "                                     - 0 MCC XML format for Input and Output\n"
                   "                                     - 1 Input is binary, output is XML\n"
                   "                                     - 2 Output is binary, input is XML\n"
                   "                                     - 3 Input and Output is binary\n"
                   "\n"
                   "Return Values:\n"
                   "  0   Successful, query satisfiable\n"
                   "  1   Unsuccesful, query not satisfiable\n"
                   "  2   Unknown, algorithm was unable to answer the question\n"
                   "  3   Error, see stderr for error message\n"
                   "\n"
                   "VerifyPN is an untimed CTL verification engine for TAPAAL.\n"
                   "TAPAAL project page: <http://www.tapaal.net>\n");
            return SuccessCode;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("VerifyPN (untimed verification engine for TAPAAL) %s\n", VERIFYPN_VERSION);
            printf("Copyright (C) 2011-2020\n");
            printf("                        Frederik Meyer Boenneland <sadpantz@gmail.com>\n");
            printf("                        Jakob Dyhr <jakobdyhr@gmail.com>\n");
            printf("                        Peter Fogh <peter.f1992@gmail.com>\n");
            printf("                        Jonas Finnemann Jensen <jopsen@gmail.com>\n");
            printf("                        Lasse Steen Jensen <lassjen88@gmail.com>\n");
            printf("                        Peter Gjøl Jensen <root@petergjoel.dk>\n");
            printf("                        Tobias Skovgaard Jepsen <tobiasj1991@gmail.com>\n");
            printf("                        Mads Johannsen <mads_johannsen@yahoo.com>\n");
            printf("                        Isabella Kaufmann <bellakaufmann93@gmail.com>\n");
            printf("                        Andreas Hairing Klostergaard <kloster92@me.com>\n");
            printf("                        Søren Moss Nielsen <soren_moss@mac.com>\n");
            printf("                        Thomas Søndersø Nielsen <primogens@gmail.com>\n");
            printf("                        Samuel Pastva <daemontus@gmail.com>\n");
            printf("                        Jiri Srba <srba.jiri@gmail.com>\n");
            printf("                        Lars Kærlund Østergaard <larsko@gmail.com>\n");
            printf("GNU GPLv3 or later <http://gnu.org/licenses/gpl.html>\n");
            return SuccessCode;
        } else if (options.modelfile == NULL) {
            options.modelfile = argv[i];
        } else if (options.queryfile == NULL) {
            options.queryfile = argv[i];
        } else {
            fprintf(stderr, "Argument Error: Unrecognized option \"%s\"\n", options.modelfile);
            return ErrorCode;
        }
    }
    //Print parameters
    if (options.printstatistics) {
        std::cout << std::endl << "Parameters: ";
        for (int i = 1; i < argc; i++) {
            std::cout << argv[i] << " ";
        }
        std::cout << std::endl;
    }

    if (options.statespaceexploration) {
        // for state-space exploration some options are mandatory
        options.enablereduction = 0;
        options.kbound = 0;
        options.queryReductionTimeout = 0;
        options.lpsolveTimeout = 0;
        options.siphontrapTimeout = 0;
        options.stubbornreduction = false;
//        outputtrace = false;
    }


    //----------------------- Validate Arguments -----------------------//

    //Check for model file
    if (!options.modelfile) {
        fprintf(stderr, "Argument Error: No model-file provided\n");
        return ErrorCode;
    }

    //Check for query file
    if (!options.modelfile && !options.statespaceexploration) {
        fprintf(stderr, "Argument Error: No query-file provided\n");
        return ErrorCode;
    }

    return ContinueCode;
}


ReturnValue contextAnalysis(ColoredPetriNetBuilder& cpnBuilder, PetriNetBuilder& builder, const PetriNet* net, vector<QueryItem> queries)
{
    //Context analysis
    ColoredAnalysisContext context(builder.getPlaceNames(), builder.getTransitionNames(), net, cpnBuilder.getUnfoldedPlaceNames(), cpnBuilder.getUnfoldedTransitionNames(), cpnBuilder.isColored());
    for(auto& q : queries)
    {
        if (q.id.empty())
            continue;
        q.query->analyze(context);

        //Print errors if any
        if (context.errors().size() > 0) {
            for (size_t i = 0; i < context.errors().size(); i++) {
                fprintf(stderr, "Query Context Analysis Error: %s\n", context.errors()[i].toString().c_str());
            }
            return ErrorCode;
        }
    }
    return ContinueCode;
}

ReturnValue parseModel(AbstractPetriNetBuilder &builder, const std::string &filename) {
    ifstream model_file{filename};
    if (!model_file) {
        fprintf(stderr, "Error: Model file \"%s\" couldn't be opened\n", filename.c_str());
        fprintf(stdout, "CANNOT_COMPUTE\n");
        return ErrorCode;
    }

    PNMLParser parser;
    parser.parse(model_file, &builder);
    return ContinueCode;
}

/**
 * Converts a formula on the form A f, E f or f into just f, assuming f is an LTL formula.
 * In the case E f, not f is returned, and in this case the model checking result should be negated
 * (indicated by bool in return value)
 * @param formula - a formula on the form A f, E f or f
 * @return @code(ltl_formula, should_negate) - ltl_formula is the formula f if it is a valid LTL formula, nullptr otherwise.
 * should_negate indicates whether the returned formula is negated (in the case the parameter was E f)
 */
std::pair<Condition_ptr, bool> to_ltl(const Condition_ptr &formula) {
    LTL::LTLValidator validator;
    bool should_negate = false;
    Condition_ptr converted;
    if (auto _formula = dynamic_cast<ECondition *>(formula.get())) {
        converted = std::make_shared<NotCondition>((*_formula)[0]);
        should_negate = true;
    } else if (auto _formula = dynamic_cast<ACondition *>(formula.get())) {
        converted = (*_formula)[0];
    } else {
        converted = formula;
    }
    converted->visit(validator);
    if (validator.bad()) {
        converted = nullptr;
    }
    return std::make_pair(converted, should_negate);
}

ReturnValue LTLMain(options_t options) {
    ReturnValue v;
    QueryXMLParser parser;

    std::string model_file{options.modelfile};
    std::string qfilename{options.queryfile};

    //std::string qfilename = //"/home/waefwerf/dev/P9/INPUTS/AirplaneLD-PT-0200/LTLCardinality.xml";
    std::ifstream queryfile{qfilename};
    if (!queryfile.is_open()){
        std::cerr << "Error opening the query file: " << qfilename << std::endl;
        return ErrorCode;
    }
    if (!parser.parse(queryfile, options.querynumbers)){
        std::cerr << "Error parsing the query file" << std::endl;
        return ErrorCode;
    }
    ColoredPetriNetBuilder cpnBuilder;

    if ((v = parseModel(cpnBuilder, model_file)) != ContinueCode) {
        std::cerr << "Error parsing the model" << std::endl;
        return v;
    }
    auto strippedBuilder = cpnBuilder.stripColors(); //TODO can we trivially handle colors or do we need to strip?
    PetriNetBuilder builder(strippedBuilder);
    std::unique_ptr<PetriNet> net{builder.makePetriNet()};
    if ((v = contextAnalysis(cpnBuilder, builder, net.get(), parser.queries)) != ContinueCode){
        std::cerr << "Error performing context analysis" << std::endl;
        return v;
    }
    for (const auto &query : parser.queries) {
        if (query.query) {
            auto[negated_formula, negate_answer] = to_ltl(query.query);
            if (!negated_formula) {
                std::cerr << "Query file " << qfilename << " contained non-LTL formula";
                return ErrorCode;
            }
            std::unique_ptr<LTL::ModelChecker> modelChecker;
            switch (options.ltlalgorithm) {
                case LTL::Algorithm::NDFS:
                    modelChecker = std::make_unique<LTL::NestedDepthFirstSearch>(*net, negated_formula);
                    break;
                case LTL::Algorithm::Tarjan:
                    if (options.trace)
                        modelChecker = std::make_unique<LTL::TarjanModelChecker<true>>(*net, negated_formula);
                    else
                        modelChecker = std::make_unique<LTL::TarjanModelChecker<false>>(*net, negated_formula);
                    break;
            }

            bool satisfied = negate_answer ^ modelChecker->isSatisfied();
            std::cout  << "FORMULA " << query.id << (satisfied ? " TRUE" : " FALSE")  << " TECHNIQUES EXPLICIT" << (options.ltlalgorithm == LTL::Algorithm::NDFS ? " NDFS" : " TARJAN") << std::endl;
        }
    }
    return SuccessCode;
}


int main(int argc, char *argv[]) {
    options_t options;
    ReturnValue v = parseOptions(argc, argv, options);
    if (v != ContinueCode) return v;
/*    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " model_file query_file" << std::endl;
        exit(1);
    }*/
    return LTLMain(options);
/*    QueryXMLParser parser;

    std::ifstream testfile{"test_models/query-test002/query.xml"};
    assert(testfile.is_open());
    std::set<size_t> queries{1};
    parser.parse(testfile, queries);
    //parser.printQueries(2);
    IsCTLVisitor isCtlVisitor;
    parser.queries[1].query->visit(isCtlVisitor);
    cout << "Is CTL query: " << isCtlVisitor.isCTL << endl;
    AsCTL asCtlVisitor;
    parser.queries[1].query->visit(asCtlVisitor);
    cout << "As CTL success." << endl;*/
    return 0;
}

