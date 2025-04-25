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

#include "VerifyPN.h"
#include "PetriEngine/PQL/Analyze.h"
#include "PetriEngine/PQL/ContainsVisitor.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"
#include "PetriEngine/PQL/ColoredUseVisitor.h"
#include "LTL/LTLValidator.h"
#include "LTL/Simplification/SpotToPQL.h"

#include <mutex>

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;


bool reduceColored(ColoredPetriNetBuilder &cpnBuilder, std::vector<std::shared_ptr<PQL::Condition> > &queries,
                   TemporalLogic logic, uint32_t timeout, std::ostream &out, int reduceMode,
                   std::vector<uint32_t>& userSequence) {
    if (!cpnBuilder.isColored()) return false;

    if (reduceMode == 0) {
        out << "\nSkipping colored structural reductions (-R 0)" << std::endl;
        out << "Net consists of " << cpnBuilder.getPlaceCount() << " places and " << cpnBuilder.getTransitionCount() << " transitions" << std::endl;
        return false;
    }

    ColoredUseVisitor useVisitor(cpnBuilder.colored_placenames(), cpnBuilder.getPlaceCount(),
                                 cpnBuilder.colored_transitionnames(), cpnBuilder.getTransitionCount());
    bool preserveLoops = false;
    bool preserveStutter = false;
    bool allReach = true;
    bool allLtl = true;
    bool allCtl = true;

    for (auto &q: queries) {
        PQL::Visitor::visit(useVisitor, q);
        preserveLoops = preserveLoops || PetriEngine::PQL::isLoopSensitive(q);
        preserveStutter = preserveStutter || PetriEngine::PQL::containsNext(q) || PetriEngine::PQL::hasNestedDeadlock(q);
        bool is_reach = PetriEngine::PQL::isReachability(q);
        if (!is_reach) {
            allReach = false;
            if (allCtl) {
                IsCTLVisitor v;
                Visitor::visit(v, q);
                allCtl = v.isCTL;
            }
            if (allLtl) {
                LTL::LTLValidator isLtl;
                allLtl = isLtl.isLTL(q);
            }
        }
    }

    if (!allCtl && !allLtl)
    {
        out << "Warning: Could not correctly detect query type in colored reducer" << std::endl;
        return false;
    }

    Colored::Reduction::QueryType queryType = allReach ? Colored::Reduction::QueryType::Reach :
            (allCtl ? Colored::Reduction::QueryType::CTL : Colored::Reduction::QueryType::LTL);

    Colored::Reduction::ColoredReducer reducer(cpnBuilder);
    bool anyReduction = reducer.reduce(timeout, useVisitor, queryType, preserveLoops, preserveStutter, reduceMode, userSequence);

    auto removedPlacesCount = (int32_t)reducer.origPlaceCount() - (int32_t)reducer.unskippedPlacesCount();
    auto removedTransitionsCount = (int32_t)reducer.origTransitionCount() - (int32_t)reducer.unskippedTransitionsCount();
    double placePercentage = 100 * (double)removedPlacesCount / reducer.origPlaceCount();
    double transitionPercentage = 100 * (double)removedTransitionsCount / reducer.origTransitionCount();

    out << "\nColored structural reductions computed in " << reducer.time() << " seconds" << std::endl;
    out << "Reduced from " << reducer.origPlaceCount() << " to " << reducer.unskippedPlacesCount() << " places " <<
        "(" << removedPlacesCount << ", " << placePercentage << "%)" << std::endl;
    out << "Reduced from " << reducer.origTransitionCount() << " to " << reducer.unskippedTransitionsCount() << " transitions " <<
        "(" << removedTransitionsCount << ", " << transitionPercentage << "%)" << std::endl;

    auto summary = reducer.createApplicationSummary();
    for (auto& rule : summary) {
        out << "Applications of rule " << rule.name << ": " << rule.applications << std::endl;
    }

    return anyReduction;
}

std::tuple<PetriNetBuilder, shared_name_name_map, shared_place_color_map>
unfold(ColoredPetriNetBuilder& cpnBuilder, bool compute_partiton, bool compute_symmetry, bool computed_fixed_point,
    std::ostream& out, int32_t partitionTimeout, int32_t max_intervals, int32_t intervals_reduced, int32_t interval_timeout, bool over_approx, bool print_bindings) {
    Colored::PartitionBuilder partition(cpnBuilder.transitions(), cpnBuilder.places());

    if(!cpnBuilder.isColored())
        return {cpnBuilder.pt_builder(), {}, {}};
    if (compute_partiton && !over_approx) {
        partition.compute(partitionTimeout);
    }

    Colored::VariableSymmetry symmetry(cpnBuilder, partition);
    if (compute_symmetry && !over_approx) {
        symmetry.compute();
    }

    Colored::ForwardFixedPoint fixed_point(cpnBuilder, partition);
    if (computed_fixed_point && !over_approx) {
        fixed_point.compute(max_intervals, intervals_reduced, interval_timeout);
    } else fixed_point.set_default();

    Colored::Unfolder unfolder(cpnBuilder, partition, symmetry, fixed_point, print_bindings);
    if(over_approx)
    {
        auto r = unfolder.strip_colors();
        return std::make_tuple<PetriNetBuilder, shared_name_name_map, shared_place_color_map>
            (std::move(r),
            shared_name_name_map{unfolder.transition_names()},
            shared_place_color_map{unfolder.place_names()});
    }
    else
    {
        auto r = unfolder.unfold();
        if (computed_fixed_point) {
            out << "\nColor fixpoint computed in " << fixed_point.time() << " seconds" << std::endl;
            out << "Max intervals used: " << fixed_point.max_intervals() << std::endl;
        }

        out << "Size of colored net: " <<
            cpnBuilder.unskippedPlacesCount() << " places, " <<
            cpnBuilder.unskippedTransitionsCount() << " transitions, and " <<
            cpnBuilder.getArcCount() << " arcs" << std::endl;
        out << "Size of unfolded net: " <<
            r.numberOfPlaces() << " places, " <<
            r.numberOfTransitions() << " transitions, and " <<
            unfolder.number_of_arcs() << " arcs" << std::endl;
        if (compute_partiton) {
            out << "Partitioned in " << partition.time() << " seconds" << std::endl;
        }
        out << "Unfolded in " << unfolder.time() << " seconds\n" << std::endl;
        
        unfolder.printBinding();
        
        return std::make_tuple<PetriNetBuilder, shared_name_name_map, shared_place_color_map>
            (std::move(r),
            shared_name_name_map{unfolder.transition_names()},
            shared_place_color_map{unfolder.place_names()});
    }
}

ReturnValue contextAnalysis(bool colored, const shared_name_name_map& transition_names, const shared_place_color_map& place_names,
    PetriNetBuilder& builder, const PetriNet* net, std::vector<std::shared_ptr<Condition> >& queries) {
    //Context analysis

    for (auto& q : queries) {
        ColoredAnalysisContext context(builder.getPlaceNames(), builder.getTransitionNames(), net,
            place_names, transition_names, colored);
        PetriEngine::PQL::analyze(q, context);
    }
    return ReturnValue::ContinueCode;
}



std::vector<Condition_ptr>
parseXMLQueries(shared_string_set& string_set, std::vector<std::string>& qstrings, std::istream& qfile, const std::set<size_t>& qnums, bool binary) {
    std::vector<QueryItem> queries;
    std::vector<Condition_ptr> conditions;
    if (binary) {
        QueryBinaryParser parser(string_set);
        if (!parser.parse(qfile, qnums)) {
            fprintf(stderr, "Error: Failed parsing binary query file\n");
            fprintf(stdout, "DO_NOT_COMPETE\n");
            conditions.clear();
            return conditions;
        }
        queries = std::move(parser.queries);
    } else {
        QueryXMLParser parser(string_set);
        if (!parser.parse(qfile, qnums)) {
            fprintf(stderr, "Error: Failed parsing XML query file\n");
            fprintf(stdout, "DO_NOT_COMPETE\n");
            conditions.clear();
            return conditions;
        }
        queries = std::move(parser.queries);
    }

    size_t i = 0;
    for (auto& q : queries) {
        if (!qnums.empty()
            && qnums.count(i) == 0) {
            ++i;
            continue;
        }
        ++i;

        if (q.parsingResult == QueryItem::UNSUPPORTED_QUERY) {
            fprintf(stdout, "The selected query in the XML query file is not supported\n");
            fprintf(stdout, "FORMULA %s CANNOT_COMPUTE\n", q.id.c_str());
            continue;
        }
        // fprintf(stdout, "Index of the selected query: %d\n\n", xmlquery);

        conditions.push_back(q.query);
        if (conditions.back() == nullptr) {
            fprintf(stderr, "Error: Failed to parse query \"%s\"\n", q.id.c_str()); //querystr.substr(2).c_str());
            fprintf(stdout, "FORMULA %s CANNOT_COMPUTE\n", q.id.c_str());
            conditions.pop_back();
        }

        qstrings.push_back(q.id);
    }
    return conditions;
}

std::vector<Condition_ptr >
readQueries(shared_string_set& string_set, options_t& options, std::vector<std::string>& qstrings) {

    std::vector<Condition_ptr > conditions;
    if (!options.statespaceexploration) {
        //Open query file
        std::ifstream qfile(options.queryfile, std::ifstream::in);
        if (!qfile) {
            fprintf(stderr, "Error: Query file \"%s\" couldn't be opened\n", options.queryfile);
            fprintf(stdout, "CANNOT_COMPUTE\n");
            conditions.clear();
            return conditions;
        }

        if (options.querynumbers.size() == 0) {

            //Read everything
            std::stringstream buffer;
            buffer << qfile.rdbuf();
            auto str = buffer.str();
            qstrings.push_back(options.queryfile);
            auto q = ParseQuery(str);
            if(q == nullptr)
                throw base_error("Error parsing: ", qstrings.back());
            conditions.emplace_back(q);
        } else {
            conditions = parseXMLQueries(string_set, qstrings, qfile, options.querynumbers, options.binary_query_io & 1);
        }
        qfile.close();
        return conditions;
    } else { // state-space exploration
        qstrings.push_back("statespace-search");
        conditions.push_back(std::make_shared<EFCondition>(BooleanCondition::FALSE_CONSTANT));
        return conditions;
    }
}

void printStats(PetriNetBuilder& builder, options_t& options) {
    if (options.printstatistics == StatisticsLevel::Full) {
        if (options.enablereduction != 0) {

            std::cout << "Size of net before structural reductions: " <<
                builder.originalNumberOfPlaces() << " places, " <<
                builder.originalNumberOfTransitions() << " transitions" << std::endl;
            std::cout << "Size of net after structural reductions: " <<
                builder.numberOfUnskippedPlaces() << " places, " <<
                builder.numberOfUnskippedTransitions() << " transitions" << std::endl;
            std::cout << "Structural reduction finished after " << builder.getReductionTime() <<
                " seconds" << std::endl;

            std::cout << "\nNet reduction is enabled.\n";
            if (options.trace != TraceLevel::None && options.enablereduction >= 1 && options.enablereduction <= 2)
                std::cout << "Rule H, J, R, S, Q disabled when a trace is requested.\n";
            builder.printStats(std::cout);
        }
    }
}


void writeQueries(const std::vector<std::shared_ptr<Condition>>&queries, std::vector<std::string>& querynames,
    std::vector<uint32_t>& order,
    std::string& filename, bool binary, const shared_name_index_map& place_names, bool keep_solved, bool compact) {
    std::fstream out;

    if (binary) {
        out.open(filename, std::ios::binary | std::ios::out);
        uint32_t cnt = 0;
        for (uint32_t j = 0; j < queries.size(); j++) {
            if ((queries[j]->isTriviallyTrue() || queries[j]->isTriviallyFalse()) && !keep_solved) continue;
            ++cnt;
        }
        out.write(reinterpret_cast<const char *> (&cnt), sizeof (uint32_t));
        cnt = place_names.size();
        out.write(reinterpret_cast<const char *> (&cnt), sizeof (uint32_t));
        for (auto& kv : place_names) {
            out.write(reinterpret_cast<const char *> (&kv.second), sizeof (uint32_t));
            out.write(kv.first->data(), kv.first->size());
            out.write("\0", sizeof (char));
        }
    } else {
        out.open(filename, std::ios::out);
        out << "<?xml version=\"1.0\"?>\n<property-set xmlns=\"http://mcc.lip6.fr/\">\n";
    }

    for (uint32_t j = 0; j < queries.size(); j++) {
        auto i = order[j];
        if ((queries[i]->isTriviallyTrue() || queries[i]->isTriviallyFalse()) && !keep_solved) continue;
        if (binary) {
            out.write(querynames[i].data(), querynames[i].size());
            out.write("\0", sizeof (char));
            BinaryPrinter binary_printer(out);
            Visitor::visit(binary_printer, queries[i]);
        } else {
            XMLPrinter xml_printer(out, compact ? 0 : 3, compact ? 0 : 2, !compact);
            xml_printer.print(*queries[i], querynames[i]);
        }
    }

    if (binary == 0) {
        out << "</property-set>\n";
    }
    out.close();
}

std::vector<Condition_ptr> getCTLQueries(const std::vector<Condition_ptr>& ctlStarQueries) {
    std::vector<Condition_ptr> ctlQueries;
    for (const auto &ctlStarQuery : ctlStarQueries) {
        IsCTLVisitor isCtlVisitor;
        Visitor::visit(isCtlVisitor, ctlStarQuery);
        if (isCtlVisitor.isCTL) {
            AsCTL asCtl;
            Visitor::visit(asCtl, ctlStarQuery);
            ctlQueries.push_back(asCtl._ctl_query);
        } else {
            throw base_error("A query could not be translated from CTL* to CTL.");
        }

    }
    return ctlQueries;
}

std::vector<Condition_ptr> getLTLQueries(const std::vector<Condition_ptr>& ctlStarQueries) {
    std::vector<Condition_ptr> ltlQueries;
    for (const auto &ctlStarQuery : ctlStarQueries) {
        LTL::LTLValidator isLtl;
        if (isLtl.isLTL(ctlStarQuery)) {
            ltlQueries.push_back(ctlStarQuery);
        } else {
            throw base_error("A query could not be translated from CTL* to LTL.");
        }
    }
    return ltlQueries;
}

#ifdef VERIFYPN_MC_Simplification
std::mutex spot_mutex;
#endif

Condition_ptr simplify_ltl_query(Condition_ptr query,
    options_t options,
    const EvaluationContext &evalContext,
    SimplificationContext &simplificationContext,
    std::ostream &out) {
    Condition_ptr cond;
    bool wasACond;
    std::vector<std::pair<std::string,size_t>> names;
    if (std::dynamic_pointer_cast<ACondition>(query) != nullptr) {
        wasACond = true;
        cond = (*std::dynamic_pointer_cast<SimpleQuantifierCondition>(query))[0];
    } else if (std::dynamic_pointer_cast<ECondition>(query) != nullptr) {
        wasACond = false;
        cond = (*std::dynamic_pointer_cast<SimpleQuantifierCondition>(query))[0];
    } else if(auto path = dynamic_cast<PathQuant*>(query.get())) {
        wasACond = path->is<AllPaths>();
        for(;path; path = dynamic_cast<PathQuant*>(path->child().get()))
        {
            if(wasACond != path->is<AllPaths>())
            {
                std::stringstream ss;
                query->toString(ss);
                throw base_error("Missing Hyper-LTL quantifiers: ", ss.str());
            }
            names.emplace_back(std::make_pair(path->name(), path->offset()));
            cond = path->child();
        }
    } else {
        wasACond = true;
        cond = query;
    }

    {
#ifdef VERIFYPN_MC_Simplification
        std::scoped_lock scopedLock{spot_mutex};
#endif
        cond = LTL::simplify(cond, options.buchiOptimization, options.ltl_compress_aps);
    }
    negstat_t stats;

    cond = pushNegation(initialMarkingRW([&]() {
        return cond;
    }, stats, evalContext, names.size() > 1, false, true),
        stats, evalContext, names.size() > 1, false, true);

    if (options.printstatistics == StatisticsLevel::Full) {
        out << "RWSTATS PRE:";
        stats.print(out);
        out << std::endl;
    }

    try {
        auto simp_cond = PetriEngine::PQL::simplify(cond, simplificationContext);
        cond = pushNegation(simp_cond.formula, stats, evalContext, names.size() > 1, false, true);
    }    catch (std::bad_alloc &ba) {
        throw base_error("Query reduction failed.\nException information: ", ba.what());
    }

    cond = initialMarkingRW([&]() {
        auto r = pushNegation(cond, stats, evalContext, names.size() > 1, false, true);
        {
#ifdef VERIFYPN_MC_Simplification
            std::scoped_lock scopedLock{spot_mutex};
#endif
            return LTL::simplify(r, options.buchiOptimization, options.ltl_compress_aps);
        }
    }, stats, evalContext, names.size() > 1, false, true);

    if (cond->isTriviallyTrue() || cond->isTriviallyFalse()) {
        // nothing
    } else if (wasACond) {
        if(names.empty())
            cond = std::make_shared<ACondition>(cond);
        else
            for(;!names.empty(); names.pop_back())
                cond = std::make_shared<AllPaths>(names.back().first, cond, names.back().second);
    } else {
        if(names.empty())
            cond = std::make_shared<ECondition>(cond);
        else
            for(;!names.empty(); names.pop_back())
                cond = std::make_shared<ExistPath>(names.back().first, cond, names.back().second);
    }
    if (options.printstatistics == StatisticsLevel::Full) {
        out << "RWSTATS POST:";
        stats.print(out);
        out << std::endl;
        out << "Query after reduction: ";
        cond->toString(out);
        out << std::endl;
    }
    return cond;
}

void outputNet(const PetriNetBuilder &builder, std::string out_file) {
    PetriNetBuilder b2(builder);
    auto unfoldedNet = std::unique_ptr<PetriNet>(b2.makePetriNet(false));
    std::fstream file;
    file.open(out_file, std::ios::out);
    unfoldedNet->toXML(file);
}

void outputQueries(const PetriNetBuilder &builder, const std::vector<PetriEngine::PQL::Condition_ptr> &queries,
    std::vector<std::string> &querynames, std::string filename, uint32_t binary_query_io, bool keep_solved) {
    std::vector<uint32_t> reorder(queries.size());
    for (uint32_t i = 0; i < queries.size(); ++i) reorder[i] = i;
    std::sort(reorder.begin(), reorder.end(), [&](auto a, auto b) {

        if (isReachability(queries[a]) != isReachability(queries[b]))
            return isReachability(queries[a]) > isReachability(queries[b]);
        if (isLoopSensitive(queries[a]) != isLoopSensitive(queries[b]))
            return isLoopSensitive(queries[a]) < isLoopSensitive(queries[b]);
        if (containsNext(queries[a]) != containsNext(queries[b]))
            return containsNext(queries[a]) < containsNext(queries[b]);
        return formulaSize(queries[a]) < formulaSize(queries[b]);
    });
    writeQueries(queries, querynames, reorder, filename, binary_query_io & 2, builder.getPlaceNames(), keep_solved);
}

void outputCompactQueries(const PetriNetBuilder &builder, const std::vector<PetriEngine::PQL::Condition_ptr> &queries,
    std::vector<std::string> &querynames, std::string filename, bool keep_solved) {
    //Don't know if this is needed
    std::vector<uint32_t> reorder(queries.size());
    for (uint32_t i = 0; i < queries.size(); ++i) reorder[i] = i;

    writeQueries(queries, querynames, reorder, filename, false, builder.getPlaceNames(), keep_solved, true);
}

void simplify_queries(const MarkVal* marking,
                      const PetriNet* net,
                      std::vector<PetriEngine::PQL::Condition_ptr>& queries,
                      options_t& options, std::ostream& outstream) {


    // simplification. We always want to do negation-push and initial marking check.
    std::vector<LPCache> caches(options.cores);
    std::atomic<uint32_t> to_handle(queries.size());
    auto begin = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    std::vector<bool> hadTo(queries.size(), true);

    do {
        auto qt = (options.queryReductionTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / (1 + (to_handle / options.cores));
        if ((to_handle <= options.cores || options.cores == 1) && to_handle > 0)
            qt = (options.queryReductionTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / to_handle;
        std::atomic<uint32_t> cnt(0);
#ifdef VERIFYPN_MC_Simplification
        std::vector<std::thread> threads;
        std::mutex out_lock;
#endif
        uint32_t old = to_handle;
        for (size_t c = 0; c < std::min<uint32_t>(options.cores, old); ++c) {
#ifdef VERIFYPN_MC_Simplification
            threads.push_back(std::thread([&, c]() {
            std::stringstream out;
#else
            auto simplify = [&, c]() {

                auto& out = outstream;
#endif

                auto& cache = caches[c];
                while (true) {
                    auto i = cnt++;
                    if (i >= queries.size()) return;
                    if (!hadTo[i]) continue;
                    hadTo[i] = false;
                    negstat_t stats;
                    EvaluationContext context(marking, net);

                    if (options.printstatistics == StatisticsLevel::Full && options.queryReductionTimeout > 0) {
                        out << "\nQuery before reduction: ";
                        queries[i]->toString(out);
                        out << std::endl;
                    }

#ifndef VERIFYPN_MC_Simplification
                    qt = (options.queryReductionTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / (queries.size() - i);
#endif
                    // this is used later, we already know that this is a plain reachability (or AG)
                    auto preSize = formulaSize(queries[i]);

                    bool wasAGCPNApprox = dynamic_cast<NotCondition*> (queries[i].get()) != nullptr;
                    if (options.logic == TemporalLogic::LTL) {
                        if (options.queryReductionTimeout == 0 || qt == 0) continue;
                        SimplificationContext simplificationContext(marking, net, qt,
                            options.lpsolveTimeout, &cache);
                        if (simplificationContext.markingOutOfBounds()) {
                            std::cout << "WARNING: Initial marking contains a place or places with too many tokens. Query simplifaction for LTL is skipped.\n";
                            break;
                        } 
                        queries[i] = simplify_ltl_query(queries[i], options,
                            context, simplificationContext, out);
#ifdef VERIFYPN_MC_Simplification
                        out_lock.lock();
                        outstream << out.str();
                        out.clear();
                        out_lock.unlock();
#endif
                        continue;
                    }
                    queries[i] = pushNegation(initialMarkingRW([&]() {
                        return queries[i];
                    }, stats, context, false, false, true),
                        stats, context, false, false, true);
                    wasAGCPNApprox |= dynamic_cast<NotCondition*> (queries[i].get()) != nullptr;

                    if (options.queryReductionTimeout > 0 && options.printstatistics == StatisticsLevel::Full) {
                        out << "RWSTATS PRE:";
                        stats.print(out);
                        out << std::endl;
                    }

                    

                    if (options.queryReductionTimeout > 0 && qt > 0) {  
                        SimplificationContext simplificationContext(marking, net, qt,
                            options.lpsolveTimeout, &cache);
                        if (simplificationContext.markingOutOfBounds()) {
                            std::cout << "WARNING: Initial marking contains a place or places with too many tokens. Query simplifaction is skipped.\n";
                            break;
                        } 
                        try {
                            negstat_t stats;
                            auto simp_cond = PetriEngine::PQL::simplify(queries[i], simplificationContext);
                            queries[i] = pushNegation(simp_cond.formula, stats, context, false, false, true);
                            wasAGCPNApprox |= dynamic_cast<NotCondition*> (queries[i].get()) != nullptr;
                            if (options.printstatistics == StatisticsLevel::Full) {
                                out << "RWSTATS POST:";
                                stats.print(out);
                                out << std::endl;
                            }
                        } catch (std::bad_alloc& ba) {
                            throw base_error("Query reduction failed.\nException information: ", ba.what());
                        }

                        if (options.printstatistics == StatisticsLevel::Full) {
                            out << "\nQuery after reduction: ";
                            queries[i]->toString(out);
                            out << std::endl;
                        }
                        if (simplificationContext.timeout()) {
                            if (options.printstatistics == StatisticsLevel::Full)
                                out << "Query reduction reached timeout.\n";
                            hadTo[i] = true;
                        } else {
                            if (options.printstatistics == StatisticsLevel::Full)
                                out << "Query reduction finished after " << simplificationContext.getReductionTime() << " seconds.\n";
                            --to_handle;
                        }
                        
                    } else if (options.printstatistics == StatisticsLevel::Full) {
                        out << "Skipping linear-programming (-q 0)" << std::endl;
                    }
                    if (options.cpnOverApprox && wasAGCPNApprox) {
                        if (queries[i]->isTriviallyTrue())
                            queries[i] = std::make_shared<BooleanCondition>(false);
                        else if (queries[i]->isTriviallyFalse())
                            queries[i] = std::make_shared<BooleanCondition>(true);
                        queries[i]->setInvariant(wasAGCPNApprox);
                    }


                    if (options.printstatistics == StatisticsLevel::Full) {
                        auto postSize = formulaSize(queries[i]);
                        double redPerc = preSize - postSize == 0 ? 0 : ((double) (preSize - postSize) / (double) preSize)*100;
                        out << "Query size reduced from " << preSize << " to " << postSize << " nodes ( " << redPerc << " percent reduction).\n";
                    }
#ifdef VERIFYPN_MC_Simplification
                    out_lock.lock();
                    outstream << out.str();
                    out.clear();
                    out_lock.unlock();
#endif
                }
            }
#ifdef VERIFYPN_MC_Simplification
            ));
#else
            ;
            simplify();
#endif
        }
#ifndef VERIFYPN_MC_Simplification
        break;
#else
        for (size_t i = 0; i < std::min<uint32_t>(options.cores, old); ++i) {
            threads[i].join();
        }
#endif
        end = std::chrono::high_resolution_clock::now();

    } while (std::any_of(hadTo.begin(), hadTo.end(), [](auto a) {
            return a;
    }) && std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() < options.queryReductionTimeout && to_handle > 0);
}

void initialize_potency(const MarkVal* marking,
                              const PetriNet* net,
                              std::vector<PetriEngine::PQL::Condition_ptr>& queries,
                              options_t& options, std::ostream& outstream,
                              std::vector<PetriEngine::MarkVal> &potencies) {
    std::vector<LPCache> caches(options.cores);
    std::atomic<uint32_t> to_handle(queries.size());
    auto begin = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    std::vector<bool> hadTo(queries.size(), true);

    do {
        auto pt = (options.initPotencyTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / (1 + (to_handle / options.cores));
        if ((to_handle <= options.cores || options.cores == 1) && to_handle > 0)
            pt = (options.initPotencyTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / to_handle;
        std::atomic<uint32_t> cnt(0);
#ifdef VERIFYPN_MC_Simplification
        std::vector<std::thread> threads;
        std::mutex out_lock;
#endif
        uint32_t old = to_handle;
        for (size_t c = 0; c < std::min<uint32_t>(options.cores, old); ++c) {
#ifdef VERIFYPN_MC_Simplification
            threads.push_back(std::thread([&, c]() {
            std::stringstream out;
#else
            auto initPot = [&, c]() {

                auto& out = outstream;
#endif

                auto& cache = caches[c];
                while (true) {
                    auto i = cnt++;
                    if (i >= queries.size()) return;
                    if (!hadTo[i]) continue;
                    hadTo[i] = false;

#ifndef VERIFYPN_MC_Simplification
                    pt = (options.initPotencyTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / (queries.size() - i);
#endif

                    if (options.initPotencyTimeout > 0 && pt > 0) {
                        SimplificationContext potencyInitializationContext(marking, net, pt,
                                                                           options.lpsolveTimeout,
                                                                           &cache, options.initPotencyTimeout);
                        try {
                            uint32_t maxConfigurationsSolved = 10;
                            PetriEngine::PQL::initPotencyVisit(queries[i], potencyInitializationContext, potencies, maxConfigurationsSolved);
                            // if (options.printstatistics != StatisticsLevel::None) {
                            //     out << "\nPotencies: ";
                            //     for (size_t p = 0; p < potencies.size(); ++p) {
                            //         if (potencies[p] != 0)
                            //            out << p << " : " << potencies[p] << ", ";
                            //     }
                            //     out << '\n' << std::endl;
                            // }
                        } catch (std::bad_alloc& ba) {
                            throw base_error("Potency initialization failed.\nException information: ", ba.what());
                        }

                        if (potencyInitializationContext.potencyTimeout()) {
                            if (options.printstatistics == StatisticsLevel::Full)
                                out << "Potency initialization reached timeout.\n";
                            hadTo[i] = true;
                        } else {
                            if (options.printstatistics == StatisticsLevel::Full)
                                out << "\nPotency initialization finished after " << potencyInitializationContext.getReductionTime() << " seconds.\n\n";
                            --to_handle;
                        }
                    } else if (options.printstatistics == StatisticsLevel::Full) {
                        out << "Skipping potency initialization" << std::endl;
                    }

#ifdef VERIFYPN_MC_Simplification
                    out_lock.lock();
                    outstream << out.str();
                    out.clear();
                    out_lock.unlock();
#endif
                }
            }
#ifdef VERIFYPN_MC_Simplification
            ));
#else
            ;
            initPot();
#endif
        }
#ifndef VERIFYPN_MC_Simplification
        break;
#else
        for (size_t i = 0; i < std::min<uint32_t>(options.cores, old); ++i) {
            threads[i].join();
        }
#endif
        end = std::chrono::high_resolution_clock::now();

    } while (std::any_of(hadTo.begin(), hadTo.end(), [](auto a) {
            return a;
    }) && std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() < options.initPotencyTimeout && to_handle > 0);
}
