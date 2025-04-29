

#include <iomanip>
#include <cstring>
#include <vector>
#include <ctime>

#include "PetriEngine/options.h"
#include "utils/errors.h"

std::vector<std::string> explode(std::string const & s) {
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, ',');) {
        result.push_back(std::move(token));
        if (result.back().empty()) result.pop_back();
    }

    return result;
}


void options_t::print(std::ostream& optionsOut) {
    if (printstatistics != StatisticsLevel::Full) {
        return;
    }

    if (strategy == Strategy::BFS) {
        optionsOut << "\nSearch=BFS";
    } else if (strategy == Strategy::DFS) {
        optionsOut << "\nSearch=DFS";
    } else if (strategy == Strategy::HEUR) {
        optionsOut << "\nSearch=HEUR";
    } else if (strategy == Strategy::RDFS) {
        optionsOut << "\nSearch=RDFS";
    } else if (strategy == Strategy::RPFS) {
        optionsOut << "\nSearch=RPFS";
    } else if (strategy == Strategy::RandomWalk) {
        optionsOut << "\nSearch=RandomWalk";
    } else {
        optionsOut << "\nSearch=OverApprox";
    }

    if (trace != TraceLevel::None) {
        optionsOut << ",Trace=ENABLED";
    } else {
        optionsOut << ",Trace=DISABLED";
    }

    if (print_bindings == true) {
        optionsOut << ",PrintBidndings=ENABLED";
    } else {
        optionsOut << ",PrintBindings=DISABLED";
    }

    if (kbound > 0) {
        optionsOut << ",Token_Bound=" << kbound;
    }

    if (statespaceexploration) {
        optionsOut << ",State_Space_Exploration=ENABLED";
    } else {
        optionsOut << ",State_Space_Exploration=DISABLED";
    }

    if (enablecolreduction == 0) {
        optionsOut << ",Colored_Structural_Reduction=DISABLED";
    } else if (enablecolreduction == 1) {
        optionsOut << ",Colored_Structural_Reduction=ALL";
    } else if (enablecolreduction == 2) {
        optionsOut << ",Colored_Structural_Reduction=CUSTOM SEQUENCE";
    }

    optionsOut << ",Colored_Struct_Red_Timout=" << colReductionTimeout;

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

    if (initPotencyTimeout > 0) {
        optionsOut << ",Init_Potency=ENABLED,InitPotencyTimeout=" << initPotencyTimeout;
    } else {
        optionsOut << ",Init_Potency=DISABLED";
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
    } else if (usedltl) {
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

    if (explicit_colored) {
        optionsOut << ",ExplicitColored=ENABLED";
        if (colored_sucessor_generator == ColoredSuccessorGeneratorOption::EVEN) {
            optionsOut << ",ColoredSuccessorGenerator=EVEN";
        } else if (colored_sucessor_generator == ColoredSuccessorGeneratorOption::FIXED) {
            optionsOut << ",ColoredSuccessorGenerator=FIXED";
        }
    }

    optionsOut << "\n";
}

void printHelp() {
    printf("Usage: verifypn [options] model-file query-file\n"
        "A tool for answering CTL, LTL and reachability queries\n"
        "for weighted P/T Petri nets extended with inhibitor arcs.\n"
        "\n"
        "Options:\n"
        "  -k, --k-bound <number of tokens>     Token bound, 0 to ignore (default)\n"
        "  -t, --trace                          Provide XML-trace to stderr\n"
        "  -b, --bindings                       Print bindings to stderr in XML format (only for CPNs, default is not to print)\n"
        "  -s, --search-strategy <strategy>     Search strategy:\n"
        "                                       - BestFS                        Heuristic search (default)\n"
        "                                       - BFS                           Breadth first search\n"
        "                                       - DFS                           Depth first search (CTL default)\n"
        "                                       - RDFS                          Random depth first search\n"
        "                                       - RPFS                          Random potency first search\n"
        "                                       - RandomWalk [<depth>] [<inc>]  Random walk using potency search\n"
        "                                           - depth  Maximum depth of a random walk (default 50000)\n"
        "                                           - inc    Increment of the maximum depth after every random walk (default 5000)\n"
        "                                       - OverApprox   Linear Over Approx\n"
        "  --init-potency-timeout <timeout>     Timeout for potencies initialization in seconds (default 10)\n"
        "                                       Only relevant for RPFS and RandomWalk strategies\n"
        "                                       write --init-potency-timeout 0 to disable the initialization\n"
        "  --seed-offset <number>               Extra noise to add to the seed of the random number generation\n"
        "  -e, --state-space-exploration        State-space exploration only (query-file is irrelevant)\n"
        "  -x, --xml-queries <query index>      Parse XML query file and verify queries of a given comma-seperated list\n"
        "  -r, --reduction <type>               Change structural net reduction:\n"
        "                                       - 0  disabled\n"
        "                                       - 1  aggressive reduction (default)\n"
        "                                       - 2  reduction preserving k-boundedness\n"
        "                                       - 3  user defined reduction sequence, eg -r 3 0,1,2,3 to use rules A,B,C,D only, and in that order\n"
        "  -R, --col-reduction <type>           Change structural net reduction:\n"
        "                                       - 0  disabled\n"
        "                                       - 1  aggressive reduction (default)\n"
        "                                       - 2  user defined reduction sequence, eg -b 2 1,0 to use colored rules B,A only, and in that order\n"
        "  -d, --reduction-timeout <timeout>    Timeout for structural reductions in seconds (default 60)\n"
        "  -D, --colreduction-timeout <timeout> Timeout for colored structural reductions in seconds (default 60)\n"
        "  -q, --query-reduction <timeout>      Query reduction timeout in seconds (default 30)\n"
        "                                       write -q 0 to disable query reduction\n"
        "  --interval-timeout <timeout>         Time in seconds before the max intervals is halved (default 10)\n"
        "                                       write --interval-timeout 0 to disable interval limits\n"
        "  --partition-timeout <timeout>        Timeout for color partitioning in seconds (default 5)\n"
        "  -l, --lpsolve-timeout <timeout>      LPSolve timeout in seconds, default 10\n"
        "  -p, --disable-partial-order          Disable partial order reduction (stubborn sets)\n"
        "  --ltl-por <type>                     Select partial order method to use with LTL engine (default automaton).\n"
        "                                       - automaton  apply Büchi-guided stubborn set method (Jensen et al., 2021).\n"
        "                                       - classic    classic stubborn set method (Valmari, 1990).\n"
        "                                                    Only applicable with formulae that do not \n"
        "                                                    contain the next-step operator.\n"
        "                                       - liebke     apply POR method by Liebke (2020).\n"
        "                                       - none       disable stubborn reductions for LTL model checking (equivalent to -p).\n"
        "  --ltl-heur <type>                    Select search heuristic for best-first search in LTL engine\n"
        "                                       Defaults to aut\n"
        "                                       - dist           Formula-driven heuristic, guiding toward states that satisfy\n"
        "                                                        the atomic propositions mentioned in the LTL formula.\n"
        "                                       - aut            Automaton-driven heuristic. Guides search toward states\n"
        "                                                        that satisfy progressing formulae in the automaton.\n"
        "                                       - fire-count     Prioritises transitions that were fired less often.\n"
        "  -a, --siphon-trap <timeout>          Siphon-Trap analysis timeout in seconds (default 0)\n"
        "      --siphon-depth <place count>     Search depth of siphon (default 0, which counts all places)\n"
        "  -n, --no-statistics                  Do not display any statistics (default is to display it)\n"
         "                                       Using -n 1 prints just statistics on number of states/edges/etc.\n"
        "  -h, --help                           Display this help message\n"
        "  -v, --version                        Display version information\n"
        "  -ctl, --ctl-algorithm [<type>]       Verify CTL properties\n"
        "                                       - local     Liu and Smolka's on-the-fly algorithm\n"
        "                                       - czero     local with certain zero extension (default)\n"
        "  -ltl, --ltl-algorithm [<type>]       Verify LTL properties (default tarjan). If omitted the queries are assumed to be CTL.\n"
        "                                       - ndfs      Nested depth first search algorithm\n"
        "                                       - tarjan    On-the-fly Tarjan's algorithm\n"
        "                                       - none      Run preprocessing steps only.\n"
        "  --noweak                             Disable optimizations for weak Büchi automata when doing \n"
        "                                       LTL model checking. Not recommended.\n"
        "  --noreach                            Force use of CTL/LTL engine, even when queries are reachability.\n"
        "                                       Not recommended since the reachability engine is faster.\n"
        "  --nounfold                           Stops after colored structural reductions and writing the reduced net\n"
        "                                       Useful for seeing the effect of colored reductions, without unfolding\n"
        "  -c, --cpn-overapproximation          Over approximate query on Colored Petri Nets (CPN only)\n"
        "  -C                                   Use explicit colored engine to answer query (CPN only).\n"
        "                                       Only supports -R, -t, --colored-successor-generator and -s options.\n"
        "  --colored-successor-generator        Sets the the successor generator used in the explicit colored engine\n"
        "                                       - fixed   transitions and bindings are traversed in a fixed order\n"
        "                                       - even    transitions and bindings are checked evenly (default)\n"
        "  --disable-cfp                        Disable the computation of possible colors in the Petri Net (CPN only)\n"
        "  --disable-partitioning               Disable the partitioning of colors in the Petri Net (CPN only)\n"
        "  --disable-symmetry-vars              Disable search for symmetric variables (CPN only)\n"
#ifdef VERIFYPN_MC_Simplification
        "  -z, --cores <number of cores>        Number of cores to use (currently only query simplification)\n"
#endif
        "  -tar, --trace-abstraction            Enables Trace Abstraction Refinement for reachability properties\n"
        "  --max-intervals <interval count>     The max amount of intervals kept when computing the color fixpoint\n"
        "                  <interval count>     Default is 250 and then after <interval-timeout> second(s) to 5\n"
        "  --write-simplified <filename>        Outputs the queries to the given file after simplification\n"
        "  --write-unfolded-queries <filename>  Outputs the queries to the given file before query reduction but after unfolding\n"
        "  --keep-solved                        Keeps queries reduced to TRUE and FALSE in the output (--write-simplified, --write-unfolded-queries)\n"
        "  --write-reduced <filename>           Outputs the model to the given file after structural reduction\n"
        "  --write-col-reduced <filename>       Outputs the model to the given file after colored structural reduction\n"
        "  --write-unfolded-net <filename>      Outputs the model to the given file before structural reduction but after unfolding\n"
        "  --binary-query-io <0,1,2,3>          Determines the input/output format of the query-file\n"
        "                                       - 0 MCC XML format for Input and Output\n"
        "                                       - 1 Input is binary, output is XML\n"
        "                                       - 2 Output is binary, input is XML\n"
        "                                       - 3 Input and Output is binary\n"
        "  --write-buchi <filename> [<format>]  Valid for LTL. Write the generated buchi automaton to file. Formats:\n"
        "                                       - dot   (default) Write the buchi in GraphViz Dot format\n"
        "                                       - hoa   Write the buchi in the Hanoi Omega-Automata Format\n"
        "                                       - spin  Write the buchi in the spin model checker format.\n"
        "  --compress-aps                       Enable compression of atomic propositions in LTL.\n"
        "                                       For some queries this helps reduce the overhead of query\n"
        "                                       simplification and Büchi construction, but gives worse\n"
        "                                       results since there is less opportunity for optimizations.\n"
        "  --noverify                           Disable verification e.g. for getting unfolded net\n"
        "  --trace-replay <file>                Replays a trace as output by the --trace option.\n"
        "                                       The trace is verified against the provided model and query.\n"
        "                                       Mainly useful for debugging.\n"
        "  --spot-optimization <1,2,3>          The optimization level passed to Spot for Büchi automaton creation.\n"
        "                                       1: Low (default), 2: Medium, 3: High\n"
        "                                       Using optimization levels above 1 may cause exponential blowups and is not recommended.\n"
        "  --strategy-output <file>             Outputs the synthesized strategy (if a such exist) to <filename>\n"
        "                                           Use '-' (dash) for outputting to standard output.\n"
        "\n"
        "Return Values:\n"
        "  0   Successful, query satisfiable\n"
        "  1   Unsuccesful, query not satisfiable\n"
        "  2   Unknown, algorithm was unable to answer the question\n"
        "  3   Error, see stderr for error message\n"
        "\n"
        "VerifyPN is an untimed CTL verification engine for TAPAAL.\n"
        "TAPAAL project page: <http://www.tapaal.net>\n");
}

bool options_t::parse(int argc, const char** argv) {
    if(argc <= 1)
    {
        printHelp();
        return true;
    }

    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-k") == 0 || std::strcmp(argv[i], "--k-bound") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &kbound) != 1 || kbound < 0) {
                throw base_error("Argument Error: Invalid number of tokens", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "-s") == 0 || std::strcmp(argv[i], "--search-strategy") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing search strategy after ", std::quoted(argv[i]));
            }
            auto* s = argv[++i];
            if (std::strcmp(s, "BestFS") == 0)
                strategy = Strategy::HEUR;
            else if (std::strcmp(s, "BFS") == 0)
                strategy = Strategy::BFS;
            else if (std::strcmp(s, "DFS") == 0)
                strategy = Strategy::DFS;
            else if (std::strcmp(s, "RDFS") == 0)
                strategy = Strategy::RDFS;
            else if (std::strcmp(s, "RPFS") == 0)
                strategy = Strategy::RPFS;
            else if (std::strcmp(s, "RandomWalk") == 0) {
                strategy = Strategy::RandomWalk;
                if (argc > i + 1) {
                    int64_t depthTemp = 0;
                    if (sscanf(argv[i + 1], "%ld", &depthTemp) == 1) { // next argument is an int64_t
                        if (depthTemp <= 0) {
                            throw base_error("Argument Error: Invalid depth value for RandomWalk ", std::quoted(argv[i + 1]));
                        } else {
                            depthRandomWalk = depthTemp;
                            ++i;
                        }
                    } else { // Next argument is not an integer, no depth specified
                        depthRandomWalk = 50000;
                        continue;
                    }
                }
                if (argc > i + 1) {
                    int64_t incTemp = 0;
                    if (sscanf(argv[i + 1], "%ld", &incTemp) == 1) { // next argument is an int64_t
                        if (incTemp < 0) {
                            throw base_error("Argument Error: Invalid increment value for RandomWalk ", std::quoted(argv[i + 1]));
                        } else {
                            incRandomWalk = incTemp;
                            ++i;
                        }
                    } else { // Next argument is not an integer, no increment specified
                        incRandomWalk = 5000;
                        continue;
                    }
                }
            } else if (std::strcmp(s, "OverApprox") == 0)
                strategy = Strategy::OverApprox;
            else {
                throw base_error("Argument Error: Unrecognized search strategy ", std::quoted(s));
            }
        } else if (std::strcmp(argv[i], "-q") == 0 || std::strcmp(argv[i], "--query-reduction") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &queryReductionTimeout) != 1 || queryReductionTimeout < 0) {
                throw base_error("Argument Error: Invalid query reduction timeout argument ", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "--init-potency-timeout") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &initPotencyTimeout) != 1 || initPotencyTimeout < 0) {
                throw base_error("Argument Error: Invalid init potency timeout argument ", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "--interval-timeout") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &intervalTimeout) != 1 || intervalTimeout < 0) {
                throw base_error("Argument Error: Invalid fixpoint timeout argument \"%s\"\n", argv[i]);
            }
        } else if (std::strcmp(argv[i], "--partition-timeout") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &partitionTimeout) != 1 || partitionTimeout < 0) {
                throw base_error("Argument Error: Invalid fixpoint timeout argument ", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "-l") == 0 || std::strcmp(argv[i], "--lpsolve-timeout") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &lpsolveTimeout) != 1 || lpsolveTimeout < 0) {
                throw base_error("Argument Error: Invalid LPSolve timeout argument ", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "-e") == 0 || std::strcmp(argv[i], "--state-space-exploration") == 0) {
            statespaceexploration = true;
            computePartition = false;
        } else if (std::strcmp(argv[i], "-n") == 0 || std::strcmp(argv[i], "--no-statistics") == 0) {
            if (argc > i + 1) {
                if (strcmp("1", argv[i+1]) == 0) {
                    printstatistics = StatisticsLevel::SearchOnly;
                }
                else if (strcmp("2", argv[i+1]) == 0) {
                    printstatistics = StatisticsLevel::Full;
                }
                else {
                    printstatistics = StatisticsLevel::None;
                    continue;
                }
                ++i;
            }
            else {
                printstatistics = StatisticsLevel::None;
            }
        } else if (std::strcmp(argv[i], "-t") == 0 || std::strcmp(argv[i], "--trace") == 0) {
            if (argc > i + 1) {
                if (std::strcmp("1", argv[i + 1]) == 0) {
                    trace = TraceLevel::Transitions;
                } else if (std::strcmp("2", argv[i + 1]) == 0) {
                    trace = TraceLevel::Full;
                } else {
                    trace = TraceLevel::Full;
                    continue;
                }
                ++i;
            } else {
                trace = TraceLevel::Full;
            }
         } else if (std::strcmp(argv[i], "-b") == 0 || std::strcmp(argv[i], "--bindings") == 0) {
            print_bindings = true;
        } else if (std::strcmp(argv[i], "-x") == 0 || std::strcmp(argv[i], "--xml-queries") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            std::vector<std::string> q = explode(argv[++i]);
            for (auto& qn : q) {
                int32_t n;
                if (sscanf(qn.c_str(), "%d", &n) != 1 || n <= 0) {
                    throw base_error("Error in query numbers ", std::quoted(qn));
                } else {
                    querynumbers.insert(--n);
                }
            }
        } else if (std::strcmp(argv[i], "-r") == 0 || std::strcmp(argv[i], "--reduction") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &enablereduction) != 1 || enablereduction < 0 || enablereduction > 4) {
                throw base_error("Argument Error: Invalid reduction argument ", std::quoted(argv[i]));
            }
            if (enablereduction == 3) {
                reductions.clear();
                std::vector<std::string> q = explode(argv[++i]);
                for (auto& qn : q) {
                    int32_t n;
                    if (sscanf(qn.c_str(), "%d", &n) != 1 || n < 0 || n > 18) {
                        throw base_error("Error in reduction rule choice ", std::quoted(qn));
                    } else {
                        reductions.push_back(n);
                    }
                }
            }
        } else if (std::strcmp(argv[i], "-R") == 0 || std::strcmp(argv[i], "--col-reduction") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &enablecolreduction) != 1 || enablecolreduction < 0 || enablecolreduction > 2) {
                throw base_error("Argument Error: Invalid colored reduction argument ", std::quoted(argv[i]));
            }
            if (enablecolreduction == 2) {
                std::vector<std::string> q = explode(argv[++i]);
                for (auto& qn : q) {
                    int32_t n;
                    if (sscanf(qn.c_str(), "%d", &n) != 1 || n < 0 || n > 6) {
                        throw base_error("Error in colored reduction rule choice ", std::quoted(qn));
                    } else {
                        colreductions.push_back(n);
                    }
                }
            }
        } else if (std::strcmp(argv[i], "-d") == 0 || std::strcmp(argv[i], "--reduction-timeout") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &reductionTimeout) != 1) {
                throw base_error("Argument Error: Invalid reduction timeout argument ", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "-D") == 0 || std::strcmp(argv[i], "--colreduction-timeout") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &colReductionTimeout) != 1) {
                throw base_error("Argument Error: Invalid reduction timeout argument ", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "--seed-offset") == 0) {
            if (sscanf(argv[++i], "%u", &seed_offset) != 1) {
                throw base_error("Argument Error: Invalid seed offset argument ", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "-p") == 0 || std::strcmp(argv[i], "--disable-partial-order") == 0) {
            stubbornreduction = false;
        } else if (std::strcmp(argv[i], "-a") == 0 || std::strcmp(argv[i], "--siphon-trap") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%u", &siphontrapTimeout) != 1) {
                throw base_error("Argument Error: Invalid siphon-trap timeout ", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "--siphon-depth") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%u", &siphonDepth) != 1) {
                throw base_error("Argument Error: Invalid siphon-depth count ", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "-tar") == 0 || std::strcmp(argv[i], "--trace-abstraction") == 0) {
            tar = true;

        } else if (std::strcmp(argv[i], "--max-intervals") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%d", &max_intervals) != 1 || max_intervals < 0) {
                throw base_error("Argument Error: Invalid number of max intervals in first argument ", std::quoted(argv[i]));
            }
            if (i != argc - 1) {
                if (sscanf(argv[++i], "%d", &max_intervals_reduced) != 1 || max_intervals_reduced < 0) {
                    throw base_error("Argument Error: Invalid number of max intervals in second argument ", std::quoted(argv[i]));
                }
            }
        } else if (std::strcmp(argv[i], "--write-simplified") == 0) {
            query_out_file = std::string(argv[++i]);
        } else if (std::strcmp(argv[i], "--binary-query-io") == 0) {
            if (sscanf(argv[++i], "%u", &binary_query_io) != 1 || binary_query_io > 3) {
                throw base_error("Argument Error: Invalid binary-query-io value ", std::quoted(argv[i]));
            }
        } else if (std::strcmp(argv[i], "--write-reduced") == 0) {
            model_out_file = std::string(argv[++i]);
        } else if (std::strcmp(argv[i], "--write-col-reduced") == 0) {
            model_col_out_file = std::string(argv[++i]);
        } else if (std::strcmp(argv[i], "--write-unfolded-net") == 0) {
            unfolded_out_file = std::string(argv[++i]);
        } else if (std::strcmp(argv[i], "--write-unfolded-queries") == 0) {
            unfold_query_out_file = std::string(argv[++i]);
        } else if (std::strcmp(argv[i], "--write-buchi") == 0) {
            buchi_out_file = std::string(argv[++i]);
            if (argc > i + 1) {
                if (std::strcmp(argv[i + 1], "dot") == 0) {
                    buchi_out_type = LTL::BuchiOutType::Dot;
                } else if (std::strcmp(argv[i + 1], "hoa") == 0) {
                    buchi_out_type = LTL::BuchiOutType::HOA;
                } else if (std::strcmp(argv[i + 1], "spin") == 0) {
                    buchi_out_type = LTL::BuchiOutType::Spin;
                } else continue;
                ++i;
            }
        } else if (std::strcmp(argv[i], "--compress-aps") == 0) {
            if (argc <= i + 1 || std::strcmp(argv[i + 1], "1") == 0) {
                ltl_compress_aps = LTL::APCompression::Full;
                ++i;
            } else if (std::strcmp(argv[i + 1], "0") == 0) {
                ltl_compress_aps = LTL::APCompression::None;
                ++i;
            }
        } else if (std::strcmp(argv[i], "--spot-optimization") == 0) {
            if (argc == i + 1) {
                throw base_error("Missing argument to --spot-optimization");
            } else if (std::strcmp(argv[i + 1], "1") == 0) {
                buchiOptimization = LTL::BuchiOptimization::Low;
            } else if (std::strcmp(argv[i + 1], "2") == 0) {
                buchiOptimization = LTL::BuchiOptimization::Medium;
            } else if (std::strcmp(argv[i + 1], "3") == 0) {
                buchiOptimization = LTL::BuchiOptimization::High;
            } else {
                throw base_error("Invalid argument ", std::quoted(argv[i]), " to --spot-optimization");
            }
            ++i;
        } else if (std::strcmp(argv[i], "--trace-replay") == 0) {
            replay_trace = true;
            replay_file = std::string(argv[++i]);
        } else if (std::strcmp(argv[i], "-C") == 0) {
            explicit_colored = true;
        } else if (std::strcmp(argv[i], "--colored-successor-generator") == 0) {
            if (argc == i + 1) {
                throw base_error("Missing argument to --colored-successor-generator");
            }
            if (std::strcmp(argv[i + 1], "fixed") == 0) {
                colored_sucessor_generator = ColoredSuccessorGeneratorOption::FIXED;
            } else if (std::strcmp(argv[i + 1], "even") == 0) {
                colored_sucessor_generator = ColoredSuccessorGeneratorOption::EVEN;
            } else {
                throw base_error("Invalid argument ", std::quoted(argv[i + 1]), " to --colored-successor-generator");
            }
            ++i;
        }
#ifdef VERIFYPN_MC_Simplification
        else if (std::strcmp(argv[i], "-z") == 0) {
            if (i == argc - 1) {
                throw base_error("Missing number after ", std::quoted(argv[i]));
            }
            if (sscanf(argv[++i], "%u", &cores) != 1) {
                throw base_error("Argument Error: Invalid cores count ", std::quoted(argv[i]));
            }
        }
#endif
        else if (std::strcmp(argv[i], "--keep-solved") == 0)
        {
            keep_solved = true;
        }
        else if (std::strcmp(argv[i], "-noreach") == 0 || std::strcmp(argv[i], "--noreach") == 0) {
            noreach = true;
        } else if (std::strcmp(argv[i], "-ctl") == 0 || std::strcmp(argv[i], "--ctl-algorithm") == 0) {
            logic = TemporalLogic::CTL;
            if (argc > i + 1) {
                if (std::strcmp(argv[i + 1], "local") == 0) {
                    ctlalgorithm = CTL::Local;
                } else if (std::strcmp(argv[i + 1], "czero") == 0) {
                    ctlalgorithm = CTL::CZero;
                } else {
                    throw base_error("Argument Error: Invalid ctl-algorithm type ", std::quoted(argv[i + 1]));
                }
                i++;
            }
        } else if (std::strcmp(argv[i], "-ltl") == 0 || std::strcmp(argv[i], "--ltl-algorithm") == 0) {
            logic = TemporalLogic::LTL;
            if (argc > i + 1) {
                if (std::strcmp(argv[i + 1], "ndfs") == 0) {
                    ltlalgorithm = LTL::Algorithm::NDFS;
                } else if (std::strcmp(argv[i + 1], "tarjan") == 0) {
                    ltlalgorithm = LTL::Algorithm::Tarjan;
                } else if (std::strcmp(argv[i + 1], "none") == 0) {
                    ltlalgorithm = LTL::Algorithm::None;
                } else {
                    continue;
                }
                i++;
            }
        } else if (std::strcmp(argv[i], "--ltl-por") == 0) {
            if (argc == i + 1) {
                throw base_error("Missing argument to --ltl-por");
            } else if (std::strcmp(argv[i + 1], "classic") == 0) {
                ltl_por = LTL::LTLPartialOrder::Visible;
            } else if (std::strcmp(argv[i + 1], "automaton") == 0) {
                ltl_por = LTL::LTLPartialOrder::Automaton;
            } else if (std::strcmp(argv[i + 1], "liebke") == 0) {
                ltl_por = LTL::LTLPartialOrder::Liebke;
            } else if (std::strcmp(argv[i + 1], "none") == 0) {
                ltl_por = LTL::LTLPartialOrder::None;
            } else {
                throw base_error("Unrecognized argument ", std::quoted(argv[i + 1]), " to --ltl-por\n");
            }
            ++i;
        } else if (std::strcmp(argv[i], "--ltl-heur") == 0) {
            if (argc == i + 1) {
                throw base_error("Missing argument to --ltl-heur");
            }
            if (std::strcmp(argv[i + 1], "aut") == 0) {
                ltlHeuristic = LTL::LTLHeuristic::Automaton;
            } else if (std::strcmp(argv[i + 1], "dist") == 0) {
                ltlHeuristic = LTL::LTLHeuristic::Distance;
            } else if (std::strcmp(argv[i + 1], "fire-count") == 0) {
                ltlHeuristic = LTL::LTLHeuristic::FireCount;
            } else {
                throw base_error("Unknown --ltl-heur value ", std::quoted(argv[i+1]));
            }

            ++i;
        } else if (std::strcmp(argv[i], "-noweak") == 0 || std::strcmp(argv[i], "--noweak") == 0) {
            ltluseweak = false;
        } else if (std::strcmp(argv[i], "-c") == 0 || std::strcmp(argv[i], "--cpn-overapproximation") == 0) {
            cpnOverApprox = true;
        } else if (std::strcmp(argv[i], "--disable-cfp") == 0) {
            computeCFP = false;
        } else if (std::strcmp(argv[i], "--disable-partitioning") == 0) {
            computePartition = false;
        } else if (std::strcmp(argv[i], "--noverify") == 0) {
            doVerification = false;
        } else if (std::strcmp(argv[i], "--nounfold") == 0) {
            doUnfolding = false;
        } else if (std::strcmp(argv[i], "--disable-symmetry-vars") == 0) {
            symmetricVariables = false;
        } else if (std::strcmp(argv[i], "--strategy-output") == 0) {
            if (argc == i + 1) {
                throw base_error("Missing argument to --strategy-output");
            }
            ++i;
            strategy_output = argv[i];
        }
        else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printHelp();
            return true;
        } else if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            printf("VerifyPN (untimed verification engine for TAPAAL) %s\n", VERIFYPN_VERSION);
            printf("Copyright (C) 2011-2023\n");
            printf("                        Alexander Bilgram <alexander@bilgram.dk>\n");
            printf("                        Frederik Meyer Boenneland <sadpantz@gmail.com>\n");
            printf("                        Emil Normann Brandt <emilnormannbrandt@gmail.com>\n");
            printf("                        Jesper Adriaan van Diepen <jespoke@hotmail.com>\n");
            printf("                        Jakob Dyhr <jakobdyhr@gmail.com>\n");
            printf("                        Peter Fogh <peter.f1992@gmail.com>\n");
            printf("                        Jonas Finnemann Jensen <jopsen@gmail.com>\n");
            printf("                        Emil Gybel Henriksen <emil-g-h@hotmail.com>\n");
            printf("                        Jens Emil Fink Højriis <jens.emil@live.dk>\n");
            printf("                        Lasse Steen Jensen <lassjen88@gmail.com>\n");
            printf("                        Peter Gjøl Jensen <root@petergjoel.dk>\n");
            printf("                        Nicolaj Østerby Jensen <nicoesterby@gmail.com>\n");
            printf("                        Tobias Skovgaard Jepsen <tobiasj1991@gmail.com>\n");
            printf("                        Mads Johannsen <mads_johannsen@yahoo.com>\n");
            printf("                        Kenneth Yrke Jørgensen <kenneth@yrke.dk>\n");
            printf("                        Isabella Kaufmann <bellakaufmann93@gmail.com>\n");
            printf("                        Alan Mozafar Khorsid <alan18@hotmail.dk>\n");
            printf("                        Andreas Hairing Klostergaard <kloster92@me.com>\n");
            printf("                        Esben Nielsen <esbenn179@gmail.com>\n");
            printf("                        Søren Moss Nielsen <soren_moss@mac.com>\n");
            printf("                        Thomas Søndersø Nielsen <primogens@gmail.com>\n");
            printf("                        Samuel Pastva <daemontus@gmail.com>\n");
            printf("                        Kira Stæhr Pedersen <kira2809@live.dk>\n");
            printf("                        Thomas Pedersen <thomas.pedersen@stofanet.dk>\n");
            printf("                        Theodor Risager <theodor349@gmail.com>\n");
            printf("                        Jiri Srba <srba.jiri@gmail.com>\n");
            printf("                        Adam Moloney Stück <adam@adast.xyz>\n");
            printf("                        Andreas Sebastian Sørensen <todes92@protonmail.com>\n");
            printf("                        Mathias Mehl Sørensen <mathiasmehlsoerensen@gmail.com>\n");
            printf("                        Peter Haar Taankvist <ptaankvist@gmail.com>\n");
            printf("                        Rasmus Grønkjær Tollund <rasmusgtollund@gmail.com>\n");
            printf("                        Nikolaj Jensen Ulrik <nikolaj@njulrik.dk>\n");
            printf("                        Simon Mejlby Virenfeldt <simon@simwir.dk>\n");
            printf("                        Lars Kærlund Østergaard <larsko@gmail.com>\n");
            printf("GNU GPLv3 or later <http://gnu.org/licenses/gpl.html>\n");
            return true;
        } else if (modelfile == nullptr) {
            modelfile = argv[i];
        } else if (queryfile == nullptr) {
            queryfile = argv[i];
        } else {
            throw base_error("Argument Error: Unrecognized option ", std::quoted(modelfile));
        }
    }

    if (statespaceexploration) {
        // for state-space exploration some options are mandatory
        enablereduction = 0;
        kbound = 0;
        queryReductionTimeout = 0;
        initPotencyTimeout = 0;
        lpsolveTimeout = 0;
        siphontrapTimeout = 0;
        stubbornreduction = false;
        //        outputtrace = false;
    }


    //----------------------- Validate Arguments -----------------------//

    //Check for model file
    if (!modelfile) {
        throw base_error("Argument Error: No model-file provided");
    }

    //Check for query file
    if (!modelfile && !statespaceexploration) {
        throw base_error("Argument Error: No query-file provided");
    }

    //Check for compatibility with LTL model checking
    if (logic == TemporalLogic::LTL) {
        if (tar) {
            throw base_error("Argument Error: -tar is not compatible with LTL model checking.");
        }
        if (siphontrapTimeout != 0) {
            throw base_error("Argument Error: -a/--siphon-trap is not compatible with LTL model checking.");
        }
        if (siphonDepth != 0) {
            throw base_error("Argument Error: --siphon-depth is not compatible with LTL model checking.");
        }
        if(strategy != Strategy::DFS &&
           strategy != Strategy::RDFS &&
           strategy != Strategy::HEUR &&
           strategy != Strategy::RPFS &&
           strategy != Strategy::RandomWalk &&
           strategy != Strategy::DEFAULT &&
           strategy != Strategy::OverApprox)
        {
            throw base_error("Argument Error: Unsupported search strategy for LTL. Supported values are DEFAULT, OverApprox, DFS, RDFS, RPFS, RandomWalk and BestFS.");
        }
    }

    if (false && replay_trace && logic != TemporalLogic::LTL) {
        throw base_error("Argument Error: Trace replay_trace is only supported for LTL model checking.");
    }
    seed_offset = (time(nullptr) xor seed_offset);
    return false;
}
