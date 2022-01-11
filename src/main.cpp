/* TAPAAL untimed verification engine verifypn
 * Copyright (C) 2011-2021  Jonas Finnemann Jensen <jopsen@gmail.com>,
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


#include "VerifyPN.h"
#include "PetriEngine/Synthesis/SimpleSynthesis.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

int main(int argc, const char** argv) {

    options_t options;
    if(options.parse(argc, argv)) // if options were --help or --version
        return SuccessCode;

    if(options.printstatistics)
    {
        std::cout << std::endl << "Parameters: ";
        for (int i = 1; i < argc; i++) {
            std::cout << argv[i] << " ";
        }
        std::cout << std::endl;
    }
    options.print();

    ColoredPetriNetBuilder cpnBuilder;
    try {
        cpnBuilder.parse_model(options.modelfile);
        options.isCPN = cpnBuilder.isColored(); // TODO: this is really nasty, should be moved in a refactor
    } catch (const base_error& err) {
        std::cout << "CANNOT_COMPUTE" << std::endl;
        std::cerr << "Error parsing the model" << std::endl;
        return ErrorCode;
    }

    if(options.cpnOverApprox && !cpnBuilder.isColored())
    {
        std::cerr << "CPN OverApproximation is only usable on colored models" << std::endl;
        return UnknownCode;
    }

    if (options.printstatistics) {
        std::cout << "Finished parsing model" << std::endl;
    }

    //----------------------- Parse Query -----------------------//
    std::vector<std::string> querynames;
    auto ctlStarQueries = readQueries(options, querynames);
    auto queries = options.logic == TemporalLogic::CTL
            ? getCTLQueries(ctlStarQueries)
            : getLTLQueries(ctlStarQueries);

    if(options.printstatistics && options.queryReductionTimeout > 0)
    {
        negstat_t stats;
        std::cout << "RWSTATS LEGEND:";
        stats.printRules(std::cout);
        std::cout << std::endl;
    }

    if(cpnBuilder.isColored())
    {
        negstat_t stats;
        EvaluationContext context(nullptr, nullptr);
        for (ssize_t qid = queries.size() - 1; qid >= 0; --qid) {
            queries[qid] = pushNegation(queries[qid], stats, context, false, false, false);
            if(options.printstatistics)
            {
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
            if (!PetriEngine::PQL::isReachability(q) || PetriEngine::PQL::isLoopSensitive(q) || stats.negated_fireability) {
                std::cerr << "Warning: CPN OverApproximation is only available for Reachability queries without deadlock, negated fireability and UpperBounds, skipping " << querynames[qid] << std::endl;
                queries.erase(queries.begin() + qid);
                querynames.erase(querynames.begin() + qid);
            }
        }
    }


    if(options.computePartition){
        cpnBuilder.computePartition(options.partitionTimeout);
    }
    if(options.symmetricVariables){
        cpnBuilder.computeSymmetricVariables();
    }
    if(options.computeCFP){
        cpnBuilder.computePlaceColorFixpoint(options.max_intervals, options.max_intervals_reduced, options.intervalTimeout);
    }


    auto builder = options.cpnOverApprox ? cpnBuilder.stripColors() : cpnBuilder.unfold();
    printUnfoldingStats(cpnBuilder, options);
    builder.sort();
    std::vector<ResultPrinter::Result> results(queries.size(), ResultPrinter::Result::Unknown);
    ResultPrinter printer(&builder, &options, querynames);

    if(options.unfolded_out_file.size() > 0) {
        outputNet(builder, options.unfolded_out_file);
    }

    //----------------------- Query Simplification -----------------------//
    bool alldone = options.queryReductionTimeout > 0;
    PetriNetBuilder b2(builder);
    ResultPrinter p2(&b2, &options, querynames);
    {
        std::unique_ptr<PetriNet> qnet(b2.makePetriNet(false));
        std::unique_ptr<MarkVal[]> qm0(qnet->makeInitialMarking());


        if(queries.size() == 0 || contextAnalysis(cpnBuilder, b2, qnet.get(), queries) != ContinueCode)
        {
            std::cerr << "Could not analyze the queries" << std::endl;
            return ErrorCode;
        }

        if(options.unfold_query_out_file.size() > 0) {
            outputCompactQueries(builder, queries, querynames, options.unfold_query_out_file);
        }

        // simplification. We always want to do negation-push and initial marking check.
        simplify_queries(qm0.get(), qnet.get(), queries, options, std::cout);


        if(options.query_out_file.size() > 0) {
            outputQueries(builder, queries, querynames, options.query_out_file, options.binary_query_io);
        }
    }

    if (!options.statespaceexploration){
        for(size_t i = 0; i < queries.size(); ++i)
        {
            if(queries[i]->isTriviallyTrue()){
                results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Satisfied).first;
                if(results[i] == ResultPrinter::Ignore && options.printstatistics)
                {
                    std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                }
                else if (options.printstatistics) {
                    std::cout << "Query solved by Query Simplification." << std::endl << std::endl;
                }
            } else if (queries[i]->isTriviallyFalse()) {
                results[i] = p2.handle(i, queries[i].get(), ResultPrinter::NotSatisfied).first;
                if(results[i] == ResultPrinter::Ignore &&  options.printstatistics)
                {
                    std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                }
                else if (options.printstatistics) {
                    std::cout << "Query solved by Query Simplification." << std::endl << std::endl;
                }
            } else if (options.strategy == Strategy::OverApprox){
                results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Unknown).first;
                if (options.printstatistics) {
                    std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                }
            } else if (options.noreach || !PetriEngine::PQL::isReachability(queries[i])) {
                if(std::dynamic_pointer_cast<PQL::ControlCondition>(queries[i]))
                    results[i] = ResultPrinter::Synthesis;
                else
                    results[i] = options.logic == TemporalLogic::CTL ? ResultPrinter::CTL : ResultPrinter::LTL;
                alldone = false;
            } else {
                queries[i] = prepareForReachability(queries[i]);
                alldone = false;
            }
        }

        if(alldone && options.model_out_file.size() == 0) return SuccessCode;
    }

    options.queryReductionTimeout = 0;

    //--------------------- Apply Net Reduction ---------------//

    if (options.enablereduction > 0) {
        // Compute how many times each place appears in the query
        builder.startTimer();
        builder.reduce(queries, results, options.enablereduction, options.trace != TraceLevel::None, nullptr, options.reductionTimeout, options.reductions);
        printer.setReducer(builder.getReducer());
    }

    printStats(builder, options);

    auto net = std::unique_ptr<PetriNet>(builder.makePetriNet());

    if(options.model_out_file.size() > 0)
    {
        std::fstream file;
        file.open(options.model_out_file, std::ios::out);
        net->toXML(file);
    }

    if(alldone) return SuccessCode;

    if (options.replay_trace) {
        if (contextAnalysis(cpnBuilder, builder, net.get(), queries) != ContinueCode) {
            std::cerr << "Fatal error assigning indexes" << std::endl;
            exit(1);
        }
        std::ifstream replay_file(options.replay_file, std::ifstream::in);
        PetriEngine::TraceReplay replay{replay_file, net.get(), options};
        for (size_t i=0; i < queries.size(); ++i) {
            if (results[i] == ResultPrinter::Unknown || results[i] == ResultPrinter::CTL || results[i] == ResultPrinter::LTL)
                replay.replay(net.get(), queries[i]);
        }
        return SuccessCode;
    }

    if(options.strategy == Strategy::OverApprox)
    {
        return SuccessCode;
    }

    if(options.doVerification){

        //----------------------- Verify CTL queries -----------------------//
        std::vector<size_t> ctl_ids;
        std::vector<size_t> ltl_ids;
        std::vector<size_t> synth_ids;
        for(size_t i = 0; i < queries.size(); ++i)
        {
            if(results[i] == ResultPrinter::CTL)
            {
                ctl_ids.push_back(i);
            }
            else if (results[i] == ResultPrinter::LTL) {
                ltl_ids.push_back(i);
            }
            else if (results[i] == ResultPrinter::Synthesis)
            {
                synth_ids.push_back(i);
            }
        }

        if (options.replay_trace) {
            if (contextAnalysis(cpnBuilder, builder, net.get(), queries) != ContinueCode) {
                std::cerr << "Fatal error assigning indexes" << std::endl;
                exit(1);
            }
            std::ifstream replay_file(options.replay_file, std::ifstream::in);
            PetriEngine::TraceReplay replay{replay_file, net.get(), options};
            for (int i : ltl_ids) {
                replay.replay(net.get(), queries[i]);
            }
            return SuccessCode;
        }

        if (!ctl_ids.empty()) {
            options.usedctl = true;
            auto reachabilityStrategy = options.strategy;

            // Assign indexes
            if(queries.empty() || contextAnalysis(cpnBuilder, builder, net.get(), queries) != ContinueCode)
            {
                std::cerr << "An error occurred while assigning indexes" << std::endl;
                return ErrorCode;
            }
            if(options.strategy == Strategy::DEFAULT) options.strategy = Strategy::DFS;
            auto v = CTLMain(net.get(),
                        options.ctlalgorithm,
                        options.strategy,
                        options.gamemode,
                        options.printstatistics,
                        true,
                        options.stubbornreduction,
                        querynames,
                        queries,
                        ctl_ids,
                        options);

            if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                return v;
            }
            // go back to previous strategy if the program continues
            options.strategy = reachabilityStrategy;
        }

        //----------------------- Verify LTL queries -----------------------//

        if (!ltl_ids.empty() && options.ltlalgorithm != LTL::Algorithm::None) {
            options.usedltl = true;
            auto v = contextAnalysis(cpnBuilder, builder, net.get(), queries);
            if (v != ContinueCode) {
                std::cerr << "Error performing context analysis" << std::endl;
                return v;
            }

            for (auto qid : ltl_ids) {
                auto res = LTL::LTLMain(net.get(), queries[qid], querynames[qid], options, builder.getReducer());
                std::cout << "\nQuery index " << qid << " was solved\n";
                std::cout << "Query is " << (res ? "" : "NOT ") << "satisfied." << std::endl;

            }
            if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                return SuccessCode;
            }
        }

        for(auto i : synth_ids)
        {
            Synthesis::SimpleSynthesis strategy(*net, *queries[i], options.kbound);

            std::ostream* strategy_out = nullptr;

            results[i] = strategy.synthesize(options.strategy, options.stubbornreduction, false);

            if(options.strategy_output == "_")
                strategy_out = &std::cout;
            else if(options.strategy_output.size() > 0)
                strategy_out = new std::ofstream(options.strategy_output);

            if(strategy_out != nullptr)
                strategy.print_strategy(*strategy_out);

            if(strategy_out != nullptr && strategy_out != &std::cout)
                delete strategy_out;
        }

        //----------------------- Siphon Trap ------------------------//

        if(options.siphontrapTimeout > 0){
            for (uint32_t i = 0; i < results.size(); i ++) {
                bool isDeadlockQuery = std::dynamic_pointer_cast<DeadlockCondition>(queries[i]) != nullptr;

                if (results[i] == ResultPrinter::Unknown && isDeadlockQuery) {
                    STSolver stSolver(printer, *net, queries[i].get(), options.siphonDepth);
                    stSolver.solve(options.siphontrapTimeout);
                    results[i] = stSolver.printResult();
                    if (results[i] == Reachability::ResultPrinter::NotSatisfied && options.printstatistics) {
                        std::cout << "Query solved by Siphon-Trap Analysis." << std::endl << std::endl;
                    }
                }
            }

            if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                return SuccessCode;
            }
        }
        options.siphontrapTimeout = 0;

        //----------------------- Reachability -----------------------//

        //Analyse context again to reindex query
        contextAnalysis(cpnBuilder, builder, net.get(), queries);

        // Change default place-holder to default strategy
        if(options.strategy == Strategy::DEFAULT) options.strategy = Strategy::HEUR;

        if(options.tar && net->numberOfPlaces() > 0)
        {
            //Create reachability search strategy
            TARReachabilitySearch strategy(printer, *net, builder.getReducer(), options.kbound);

            // Change default place-holder to default strategy
            fprintf(stdout, "Search strategy option was ignored as the TAR engine is called.\n");
            options.strategy = Strategy::DFS;

            //Reachability search
            strategy.reachable(queries, results,
                    options.printstatistics,
                    options.trace != TraceLevel::None);
        }
        else
        {
            ReachabilitySearch strategy(*net, printer, options.kbound);

            // Change default place-holder to default strategy
            if(options.strategy == Strategy::DEFAULT) options.strategy = Strategy::HEUR;

            //Reachability search
            strategy.reachable(queries, results,
                            options.strategy,
                            options.stubbornreduction,
                            options.statespaceexploration,
                            options.printstatistics,
                            options.trace != TraceLevel::None,
                            options.seed());
        }
    }


    return SuccessCode;
}
