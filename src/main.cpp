/* TAPAAL untimed verification engine verifypn
 * Copyright (C) 2011-2023  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                          Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                          Lars Kærlund Østergaard <larsko@gmail.com>,
 *                          Jiri Srba <srba.jiri@gmail.com>,
 *                          Peter Gjøl Jensen <root@petergjoel.dk>
 *
 * CTL Extension
 *                          Peter Fogh <peter.f1992@gmail.com>
 *                          Isabella Kaufmann <bellakaufmann93@gmail.com>
 *                          Tobias Skovgaard Jepsen <tobiasj1991@gmail.com>
 *                          Lasse Steen Jensen <lassjen88@gmail.com>
 *                          Søren Moss Nielsen <soren_moss@mac.com>
 *                          Samuel Pastva <daemontus@gmail.com>
 *                          Jiri Srba <srba.jiri@gmail.com>
 *
 * Stubborn sets, query simplification, siphon-trap property
 *                          Frederik Meyer Boenneland <sadpantz@gmail.com>
 *                          Jakob Dyhr <jakobdyhr@gmail.com>
 *                          Peter Gjøl Jensen <root@petergjoel.dk>
 *                          Mads Johannsen <mads_johannsen@yahoo.com>
 *                          Jiri Srba <srba.jiri@gmail.com>
 *
 * LTL Extension
 *                          Nikolaj Jensen Ulrik <nikolaj@njulrik.dk>
 *                          Simon Mejlby Virenfeldt <simon@simwir.dk>
 *
 * Color Extension
 *                          Alexander Bilgram <alexander@bilgram.dk>
 *                          Peter Haar Taankvist <ptaankvist@gmail.com>
 *                          Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                          Andreas H. Klostergaard
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <PetriEngine/Colored/PnmlWriter.h>
#include "VerifyPN.h"
#include "PetriEngine/Synthesis/SimpleSynthesis.h"
#include "LTL/LTLSearch.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/ExplicitColored/ColoredPetriNetBuilder.h"
#include "PetriEngine/ExplicitColored/Algorithms/NaiveWorklist.h"
#include "utils/NullStream.h"
using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

int explicitColored(options_t& options, shared_string_set& string_set, std::vector<Condition_ptr>& queries, const std::vector<std::string>& queryNames);

int main(int argc, const char** argv) {
    shared_string_set string_set; //<-- used for de-duplicating names of places/transitions
    try {
        options_t options;
        if (options.parse(argc, argv)) // if options were --help or --version
            return to_underlying(ReturnValue::SuccessCode);

        if (options.printstatistics == StatisticsLevel::Full) {
            std::cout << std::endl << "Parameters: ";
            for (int i = 1; i < argc; i++) {
                std::cout << argv[i] << " ";
            }
            std::cout << std::endl;
        }
        options.print();

        //----------------------- Parse Query -----------------------//
        std::vector<std::string> querynames;
        auto ctlStarQueries = readQueries(string_set, options, querynames);
        auto queries = options.logic == TemporalLogic::CTL
                       ? getCTLQueries(ctlStarQueries)
                       : getLTLQueries(ctlStarQueries);

        if (options.explicit_colored) {
            return explicitColored(options, string_set, queries, querynames);
        }

        ColoredPetriNetBuilder cpnBuilder(string_set);
        try {
            cpnBuilder.parse_model(options.modelfile);
            options.isCPN = cpnBuilder.isColored(); // TODO: this is really nasty, should be moved in a refactor
        } catch (const base_error &err) {
            throw base_error("CANNOT_COMPUTE\nError parsing the model\n", err.what());
        }

        if (!options.model_col_out_file.empty() && cpnBuilder.hasPartition()) {
            std::cerr << "Cannot write colored PNML as the original net has partitions. Not supported (yet)" << std::endl;
            return to_underlying(ReturnValue::UnknownCode);
        }

        if (options.cpnOverApprox && !cpnBuilder.isColored()) {
            std::cerr << "CPN OverApproximation is only usable on colored models" << std::endl;
            return to_underlying(ReturnValue::UnknownCode);
        }

        if (options.printstatistics == StatisticsLevel::Full) {
            std::cout << "Finished parsing model" << std::endl;
        }

        if (options.printstatistics == StatisticsLevel::Full && options.queryReductionTimeout > 0) {
            negstat_t stats;
            std::cout << "RWSTATS LEGEND:";
            stats.printRules(std::cout);
            std::cout << std::endl;
        }

        if (cpnBuilder.isColored()) {
            negstat_t stats;
            EvaluationContext context(nullptr, nullptr);
            for (ssize_t qid = queries.size() - 1; qid >= 0; --qid) {
                queries[qid] = pushNegation(queries[qid], stats, context, false, false, false);
                if (options.printstatistics == StatisticsLevel::Full) {
                    std::cout << "\nQuery before expansion and reduction: ";
                    queries[qid]->toString(std::cout);
                    std::cout << std::endl;

                    std::cout << "RWSTATS COLORED PRE:";
                    stats.print(std::cout);
                    std::cout << std::endl;
                }
            }
        }

        if (options.cpnOverApprox) {
            for (ssize_t qid = queries.size() - 1; qid >= 0; --qid) {
                negstat_t stats;
                EvaluationContext context(nullptr, nullptr);
                auto q = pushNegation(queries[qid], stats, context, false, false, false);
                if (!PetriEngine::PQL::isReachability(q) || PetriEngine::PQL::isLoopSensitive(q) ||
                    stats.negated_fireability) {
                    std::cerr
                            << "Warning: CPN OverApproximation is only available for Reachability queries without deadlock, negated fireability and UpperBounds, skipping "
                            << querynames[qid] << std::endl;
                    queries.erase(queries.begin() + qid);
                    querynames.erase(querynames.begin() + qid);
                }
            }
        }

        std::stringstream ss;
        std::ostream& out = options.printstatistics == StatisticsLevel::Full ? std::cout : ss;
        reduceColored(cpnBuilder, queries, options.logic, options.colReductionTimeout, out, options.enablecolreduction, options.colreductions);

        if (options.model_col_out_file.size() > 0) {
            std::fstream file;
            file.open(options.model_col_out_file, std::ios::out);
            PetriEngine::Colored::PnmlWriter writer(cpnBuilder, file);
            writer.toColPNML();
        }

        if (!options.doUnfolding) {
            return 0;
        }

        auto [builder, transition_names, place_names] = unfold(cpnBuilder,
            options.computePartition, options.symmetricVariables,
            options.computeCFP, out,
            options.partitionTimeout, options.max_intervals, options.max_intervals_reduced,
            options.intervalTimeout, options.cpnOverApprox, options.print_bindings);

        builder.sort();
        std::vector<ResultPrinter::Result> results(queries.size(), ResultPrinter::Result::Unknown);
        ResultPrinter printer(&builder, &options, querynames);

        if (options.unfolded_out_file.size() > 0) {
            outputNet(builder, options.unfolded_out_file);
        }

        //----------------------- Query Simplification -----------------------//
        bool alldone = options.queryReductionTimeout > 0;
        PetriNetBuilder b2(builder);
        std::set<size_t> initial_marking_solved;
        size_t initial_size = 0;
        ResultPrinter p2(&b2, &options, querynames);
        {
            std::unique_ptr<PetriNet> qnet(b2.makePetriNet(false));
            std::unique_ptr<MarkVal[]> qm0(qnet->makeInitialMarking());
            for(size_t i = 0; i < qnet->numberOfPlaces(); ++i)
                initial_size += qm0[i];

            if(queries.empty() && options.cpnOverApprox)
            {
                std::cerr << "WARNING: Could not run CPN over-approximation on any queries, terminating." << std::endl;
                std::exit(0);
            }

            if (queries.empty() ||
                contextAnalysis(cpnBuilder.isColored() && !options.cpnOverApprox, transition_names, place_names, b2, qnet.get(), queries) != ReturnValue::ContinueCode) {
                throw base_error("Could not analyze the queries");
            }

            if (options.unfold_query_out_file.size() > 0) {
                outputCompactQueries(builder, queries, querynames, options.unfold_query_out_file, options.keep_solved);
            }


            {
                EvaluationContext context(qm0.get(), qnet.get());
                for (size_t i = 0; i < queries.size(); ++i) {
                    ContainsFireabilityVisitor has_fireability;
                    Visitor::visit(has_fireability, queries[i]);
                    if(has_fireability.getReturnValue() && options.cpnOverApprox) continue;
                    if(containsUpperBounds(queries[i])) continue;
                    auto r = PQL::evaluate(queries[i].get(), context);
                    if(r == Condition::RFALSE)
                    {
                        queries[i] = BooleanCondition::FALSE_CONSTANT;
                        initial_marking_solved.emplace(i);
                    }
                    else if(r == Condition::RTRUE)
                    {
                        queries[i] = BooleanCondition::TRUE_CONSTANT;
                        initial_marking_solved.emplace(i);
                    }
                }
            }

            // simplification. We always want to do negation-push and initial marking check.
            simplify_queries(qm0.get(), qnet.get(), queries, options, std::cout);

            if (options.query_out_file.size() > 0) {
                outputQueries(builder, queries, querynames, options.query_out_file, options.binary_query_io, options.keep_solved);
            }

            if (!options.statespaceexploration) {
                for (size_t i = 0; i < queries.size(); ++i) {
                    if (queries[i]->isTriviallyTrue()) {
                        if(initial_marking_solved.count(i) > 0 && options.trace != TraceLevel::None)
                        {
                            // we misuse the implementation to make sure we print the empty-trace
                            // when the initial marking is sufficient.
                            Structures::StateSet tmp(*qnet, 0);
                            results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Satisfied, nullptr,
                                                    0, 1, 1, initial_size, &tmp, 0, qm0.get()).first;
                        }
                        else
                            results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Satisfied).first;
                        if (results[i] == ResultPrinter::Ignore && options.printstatistics == StatisticsLevel::Full) {
                            std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                        } else if (options.printstatistics == StatisticsLevel::Full) {
                            std::cout << "Query solved by Query Simplification.\n" << std::endl;
                        }
                    } else if (queries[i]->isTriviallyFalse()) {
                        if(initial_marking_solved.count(i) > 0 && options.trace != TraceLevel::None)
                        {
                            // we misuse the implementation to make sure we print the empty-trace
                            // when the initial marking is sufficient.
                            Structures::StateSet tmp(*qnet, 0);
                            // we are tricking the printer into printing the trace here.
                            // TODO fix, remove setInvariant
                            // also we make a new FALSE object here to avoid sideeffects.
                            queries[i] = std::make_shared<BooleanCondition>(false);
                            queries[i]->setInvariant(true);
                            results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Satisfied, nullptr,
                                                    0, 1, 1, initial_size, &tmp, 0, qm0.get()).first;
                        }
                        else
                            results[i] = p2.handle(i, queries[i].get(), ResultPrinter::NotSatisfied).first;
                        if (results[i] == ResultPrinter::Ignore && options.printstatistics == StatisticsLevel::Full) {
                            std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                        } else if (options.printstatistics == StatisticsLevel::Full) {
                            std::cout << "Query solved by Query Simplification.\n" << std::endl;
                        }

                    } else if (options.strategy == Strategy::OverApprox) {
                        results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Unknown).first;
                        if (options.printstatistics == StatisticsLevel::Full) {
                            std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                        }
                    } else if (options.noreach || !PetriEngine::PQL::isReachability(queries[i])) {
                        if (std::dynamic_pointer_cast<PQL::ControlCondition>(queries[i]))
                            results[i] = ResultPrinter::Synthesis;
                        else
                            results[i] = options.logic == TemporalLogic::CTL ? ResultPrinter::CTL : ResultPrinter::LTL;
                        alldone = false;
                    } else {
                        alldone = false;
                    }
                }

                if (alldone && options.model_out_file.size() == 0)
                    return to_underlying(ReturnValue::SuccessCode);
            }
        }

        options.queryReductionTimeout = 0;

        //--------------------- Apply Net Reduction ---------------//

        if (options.trace != TraceLevel::None) {
            // auto netBeforeReduction = std::unique_ptr<PetriNet>(b2.makePetriNet(false));
            builder.saveInitialNet();
        }

        builder.freezeOriginalSize();
        if (options.enablereduction > 0) {
            // Compute structural reductions
            builder.startTimer();
            builder.reduce(queries, results, options.enablereduction, options.trace != TraceLevel::None, nullptr,
                           options.reductionTimeout, options.reductions);
            printer.setReducer(builder.getReducer());
        }

        printStats(builder, options);

        auto net = std::unique_ptr<PetriNet>(builder.makePetriNet());

        if (options.model_out_file.size() > 0) {
            std::fstream file;
            file.open(options.model_out_file, std::ios::out);
            net->toXML(file);
        }

        if (alldone)
            return to_underlying(ReturnValue::SuccessCode);

        if (options.replay_trace) {
            if (contextAnalysis(cpnBuilder.isColored() && !options.cpnOverApprox, transition_names, place_names, builder, net.get(), queries) != ReturnValue::ContinueCode) {
                throw base_error("Fatal error assigning indexes");
            }
            std::ifstream replay_file(options.replay_file, std::ifstream::in);
            PetriEngine::TraceReplay replay{replay_file, net.get(), options};
            for (size_t i = 0; i < queries.size(); ++i) {
                if (results[i] == ResultPrinter::Unknown || results[i] == ResultPrinter::CTL ||
                    results[i] == ResultPrinter::LTL)
                    replay.replay(net.get(), queries[i]);
            }
            return to_underlying(ReturnValue::SuccessCode);
        }


        if (options.strategy == Strategy::OverApprox) {
            return to_underlying(ReturnValue::SuccessCode);
        }

        if (options.doVerification) {

            auto verifStart = std::chrono::high_resolution_clock::now();
            // When this ptr goes out of scope it will print the time spent during verification
            std::shared_ptr<void> defer (nullptr, [&verifStart](...){
                auto verifEnd = std::chrono::high_resolution_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::microseconds>(verifEnd - verifStart).count() / 1000000.0;
                std::cout << std::setprecision(6) << "Spent " << diff << " on verification" << std::endl;
            });

            //----------------------- Verify CTL queries -----------------------//
            std::vector<size_t> ctl_ids;
            std::vector<size_t> ltl_ids;
            std::vector<size_t> synth_ids;
            for (size_t i = 0; i < queries.size(); ++i) {
                if (results[i] == ResultPrinter::CTL) {
                    ctl_ids.push_back(i);
                } else if (results[i] == ResultPrinter::LTL) {
                    ltl_ids.push_back(i);
                } else if (results[i] == ResultPrinter::Synthesis) {
                    synth_ids.push_back(i);
                }
            }

            if (options.replay_trace) {
                if (contextAnalysis(cpnBuilder.isColored() && !options.cpnOverApprox, transition_names, place_names, builder, net.get(), queries) != ReturnValue::ContinueCode) {
                    throw base_error("Fatal error assigning indexes");
                }
                std::ifstream replay_file(options.replay_file, std::ifstream::in);
                PetriEngine::TraceReplay replay{replay_file, net.get(), options};
                for (int i: ltl_ids) {
                    replay.replay(net.get(), queries[i]);
                }
                return to_underlying(ReturnValue::SuccessCode);
            }

            // Assign indexes
            if (queries.empty() ||
                contextAnalysis(cpnBuilder.isColored() && !options.cpnOverApprox, transition_names, place_names, builder, net.get(), queries) != ReturnValue::ContinueCode) {
                throw base_error("An error occurred while assigning indexes");
            }

            if (!ctl_ids.empty()) {
                options.usedctl = true;
                auto reachabilityStrategy = options.strategy;

                if (options.strategy == Strategy::DEFAULT) options.strategy = Strategy::DFS;
                auto v = CTLMain(net.get(),
                                 options.ctlalgorithm,
                                 options.strategy,
                                 options.printstatistics,
                                 options.stubbornreduction,
                                 querynames,
                                 queries,
                                 ctl_ids,
                                 options);

                if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                    return to_underlying(v);
                }
                // go back to previous strategy if the program continues
                options.strategy = reachabilityStrategy;
            }

            //----------------------- Verify LTL queries -----------------------//

            if (!ltl_ids.empty() && options.ltlalgorithm != LTL::Algorithm::None) {
                options.usedltl = true;

                for (auto qid : ltl_ids) {
                    LTL::LTLSearch search(*net, queries[qid], options.buchiOptimization, options.ltl_compress_aps);
                    auto res = search.solve(options.trace != TraceLevel::None, options.kbound,
                        options.ltlalgorithm, options.stubbornreduction ? options.ltl_por : LTL::LTLPartialOrder::None,
                        options.strategy, options.ltlHeuristic, options.ltluseweak, options.seed_offset);

                    if(options.printstatistics != StatisticsLevel::None)
                        search.print_stats(std::cout);

                    std::cout << "FORMULA " << querynames[qid]
                        << (res ? " TRUE" : " FALSE") << " TECHNIQUES EXPLICIT "
                        << LTL::to_string(options.ltlalgorithm)
                        << (search.is_weak() ? " WEAK_SKIP" : "")
                        << (search.used_partial_order() != LTL::LTLPartialOrder::None ? " STUBBORN" : "")
                        << (search.used_partial_order() == LTL::LTLPartialOrder::Visible ? " CLASSIC_STUB" : "")
                        << (search.used_partial_order() == LTL::LTLPartialOrder::Automaton ? " AUT_STUB" : "")
                        << (search.used_partial_order() == LTL::LTLPartialOrder::Liebke ? " LIEBKE_STUB" : "");
                    auto heur = search.heuristic_type();
                    if (!heur.empty())
                        std::cout << " HEURISTIC " << heur;
                    std::cout << " OPTIM-" << to_underlying(options.buchiOptimization) << std::endl;

                    std::cout << "\nQuery index " << qid << " was solved\n";
                    std::cout << "Query is " << (res ? "" : "NOT ") << "satisfied." << std::endl;

                    if(options.trace != TraceLevel::None)
                        search.print_trace(std::cerr, *builder.getReducer());
                }

                if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                    return to_underlying(ReturnValue::SuccessCode);
                }
            }


            for (auto i : synth_ids) {
                if(options.tar) {
                    throw base_error("TAR not supported for synthesis.");
                }
                Synthesis::SimpleSynthesis strategy(*net, *queries[i], options.kbound);

                std::ostream *strategy_out = nullptr;

                results[i] = strategy.synthesize(options.strategy, options.stubbornreduction, false);

                strategy.result().print(querynames[i], options.printstatistics, i, options, std::cout);

                if (options.strategy_output == "-")
                    strategy_out = &std::cout;
                else if (options.strategy_output.size() > 0)
                    strategy_out = new std::ofstream(options.strategy_output);

                if (strategy_out != nullptr)
                    strategy.print_strategy(*strategy_out);

                if (strategy_out != nullptr && strategy_out != &std::cout)
                    delete strategy_out;
                if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                    return to_underlying(ReturnValue::SuccessCode);
                }
            }


            //----------------------- Siphon Trap ------------------------//

            if (options.siphontrapTimeout > 0) {
                for (uint32_t i = 0; i < results.size(); i++) {
                    bool isDeadlockQuery = std::dynamic_pointer_cast<DeadlockCondition>(queries[i]) != nullptr;

                    if (results[i] == ResultPrinter::Unknown && isDeadlockQuery) {
                        STSolver stSolver(printer, *net, queries[i].get(), options.siphonDepth);
                        stSolver.solve(options.siphontrapTimeout);
                        results[i] = stSolver.printResult();
                        if (results[i] != Reachability::ResultPrinter::Unknown && options.printstatistics == StatisticsLevel::Full) {
                            std::cout << "Query solved by Siphon-Trap Analysis." << std::endl << std::endl;
                        }
                    }
                }

                if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                    return to_underlying(ReturnValue::SuccessCode);
                }
            }
            options.siphontrapTimeout = 0;

            //----------------------- Reachability -----------------------//

            // remove the prefix EF/AF (LEGACY, should not be handled here)
            for(uint32_t i = 0; i < results.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                    queries[i] = prepareForReachability(queries[i]);
            }
            if (options.tar && net->numberOfPlaces() > 0) {
                //Create reachability search strategy
                TarResultPrinter tar_printer(printer);
                TARReachabilitySearch strategy(tar_printer, *net, builder.getReducer(), options.kbound);

                // Change default place-holder to default strategy
                fprintf(stdout, "Search strategy option was ignored as the TAR engine is called.\n");
                options.strategy = Strategy::DFS;

                //Reachability search
                strategy.reachable(queries, results,
                                   options.printstatistics,
                                   options.trace != TraceLevel::None);
            } else {
                ReachabilitySearch strategy(*net, printer, options.kbound);

                // Change default place-holder to default strategy
                if (options.strategy == Strategy::DEFAULT) options.strategy = Strategy::HEUR;

                //Reachability search
                if (options.initPotencyTimeout > 0 && (options.strategy == Strategy::RandomWalk || options.strategy == Strategy::RPFS)) {
                    std::vector<MarkVal> initialPotencies(net->numberOfTransitions(), 0);

                    {
                        std::unique_ptr<MarkVal[]> qm0(net->makeInitialMarking());
                        initialize_potency(qm0.get(), net.get(), queries, options, std::cout, initialPotencies);
                    }

                    strategy.reachable(queries, results,
                                    options.strategy,
                                    options.stubbornreduction,
                                    options.statespaceexploration,
                                    options.printstatistics,
                                    options.trace != TraceLevel::None,
                                    options.seed(),
                                    options.depthRandomWalk,
                                    options.incRandomWalk,
                                    initialPotencies);
                } else {
                    strategy.reachable(queries, results,
                                    options.strategy,
                                    options.stubbornreduction,
                                    options.statespaceexploration,
                                    options.printstatistics,
                                    options.trace != TraceLevel::None,
                                    options.seed(),
                                    options.depthRandomWalk,
                                    options.incRandomWalk);
                }
            }
        }
    } catch (base_error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        std::exit(-1);
    }

    return to_underlying(ReturnValue::SuccessCode);
}

int explicitColored(options_t& options, shared_string_set& string_set, std::vector<Condition_ptr>& queries, const std::vector<std::string>& queryNames) {
    std::cout << "Using explicit colored" << std::endl;
    NullStream nullStream;
    std::ostream &fullStatisticOut = options.printstatistics == StatisticsLevel::Full ? std::cout : nullStream;
    ExplicitColored::ColoredPetriNetBuilder builder;
    if (options.enablecolreduction) {
        ColoredPetriNetBuilder cpnBuilder(string_set);
        cpnBuilder.parse_model(options.modelfile);
        std::stringstream cpnOut;
        reduceColored(cpnBuilder, queries, options.logic, options.colReductionTimeout, fullStatisticOut,
                      options.enablecolreduction, options.colreductions);
        Colored::PnmlWriter writer(cpnBuilder, cpnOut);
        writer.toColPNML();
        builder.parse_model(cpnOut);
        fullStatisticOut << std::endl;
    } else {
        builder.parse_model(options.modelfile);
    }

    auto buildStatus = builder.build();

    switch (buildStatus) {
        case ExplicitColored::ColoredPetriNetBuilderStatus::OK:
            break;
        case ExplicitColored::ColoredPetriNetBuilderStatus::TOO_MANY_BINDINGS:
            std::cout << "The colored petri net has too many bindings to be represented" << std::endl
                    << "TOO_MANY_BINDINGS" << std::endl;
            return to_underlying(ReturnValue::UnknownCode);
        default:
            std::cout << "The explicit colored petri net builder gave an unexpected error " << static_cast<int>(
                        buildStatus) << std::endl
                    << "Unknown builder error " << static_cast<int>(buildStatus) << std::endl;
            return to_underlying(ReturnValue::ErrorCode);
    }

    auto net = builder.takeNet();
    bool result = false;
    bool randomize = false;
    auto placeIndices = builder.takePlaceIndices();
    auto transitionIndices = builder.takeTransitionIndices();
    if (randomize){
        net.randomizeBindingOrder(options.seed());
    }


    for (size_t i = 0; i < queries.size(); i++) {
        const auto seed = options.seed();
        ExplicitColored::ColoredResultPrinter resultPrinter(i, fullStatisticOut, queryNames, seed);
        ExplicitColored::NaiveWorklist naiveWorkList(net, queries[i], placeIndices, transitionIndices, resultPrinter, seed);
        switch (options.strategy) {
            case Strategy::DEFAULT:
            case Strategy::DFS:
                result = naiveWorkList.check(ExplicitColored::SearchStrategy::DFS, options.colored_sucessor_generator);
                break;
            case Strategy::BFS:
                result = naiveWorkList.check(ExplicitColored::SearchStrategy::BFS, options.colored_sucessor_generator);
                break;
            case Strategy::RDFS:
                result = naiveWorkList.check(ExplicitColored::SearchStrategy::RDFS, options.colored_sucessor_generator);
                break;
            case Strategy::HEUR:
                result = naiveWorkList.check(ExplicitColored::SearchStrategy::HEUR, options.colored_sucessor_generator);
                break;
            default:
                std::cout << "Strategy is not supported for explicit colored engine" << std::endl
                        << "UNSUPPORTED STRATEGY" << std::endl;
                return to_underlying(ReturnValue::ErrorCode);
        }
    }
    if (result) {
        return to_underlying(ReturnValue::SuccessCode);
    }
    return to_underlying(ReturnValue::FailedCode);
}
