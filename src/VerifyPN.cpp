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
#include "PetriEngine/PQL/Analyze.h"

#include <mutex>

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

ReturnValue contextAnalysis(ColoredPetriNetBuilder& cpnBuilder, PetriNetBuilder& builder, const PetriNet* net, std::vector<std::shared_ptr<Condition> >& queries) {
    //Context analysis
    ColoredAnalysisContext context(builder.getPlaceNames(), builder.getTransitionNames(), net, cpnBuilder.getUnfoldedPlaceNames(), cpnBuilder.getUnfoldedTransitionNames(), cpnBuilder.isColored());
    for (auto& q : queries) {
        PetriEngine::PQL::analyze(q, context);

        //Print errors if any
        if (context.errors().size() > 0) {
            std::stringstream ss;
            for (size_t i = 0; i < context.errors().size(); i++) {
                ss << "Query Context Analysis Error: " << context.errors()[i].toString() << "\n";
            }
            throw base_error(ss.str());
        }
    }
    return ReturnValue::ContinueCode;
}

std::vector<Condition_ptr>
parseXMLQueries(std::vector<std::string>& qstrings, std::istream& qfile, const std::set<size_t>& qnums, bool binary) {
    std::vector<QueryItem> queries;
    std::vector<Condition_ptr> conditions;
    if (binary) {
        QueryBinaryParser parser;
        if (!parser.parse(qfile, qnums)) {
            fprintf(stderr, "Error: Failed parsing binary query file\n");
            fprintf(stdout, "DO_NOT_COMPETE\n");
            conditions.clear();
            return conditions;
        }
        queries = std::move(parser.queries);
    } else {
        QueryXMLParser parser;
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
readQueries(options_t& options, std::vector<std::string>& qstrings) {

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
            conditions = parseXMLQueries(qstrings, qfile, options.querynumbers, options.binary_query_io & 1);
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
    if (options.printstatistics) {
        if (options.enablereduction != 0) {

            std::cout << "Size of net before structural reductions: " <<
                builder.numberOfPlaces() << " places, " <<
                builder.numberOfTransitions() << " transitions" << std::endl;
            std::cout << "Size of net after structural reductions: " <<
                builder.numberOfPlaces() - builder.RemovedPlaces() << " places, " <<
                builder.numberOfTransitions() - builder.RemovedTransitions() << " transitions" << std::endl;
            std::cout << "Structural reduction finished after " << builder.getReductionTime() <<
                " seconds" << std::endl;

            std::cout << "\nNet reduction is enabled.\n";
            builder.printStats(std::cout);
        }
    }
}

void printUnfoldingStats(ColoredPetriNetBuilder& builder, options_t& options) {
    //if (options.printstatistics) {
    if (!builder.isColored() && !builder.isUnfolded())
        return;
    if (options.computeCFP) {
        std::cout << "\nColor fixpoint computed in " << builder.getFixpointTime() << " seconds" << std::endl;
        std::cout << "Max intervals used: " << builder.getMaxIntervals() << std::endl;
    }

    std::cout << "Size of colored net: " <<
        builder.getPlaceCount() << " places, " <<
        builder.getTransitionCount() << " transitions, and " <<
        builder.getArcCount() << " arcs" << std::endl;
    std::cout << "Size of unfolded net: " <<
        builder.getUnfoldedPlaceCount() << " places, " <<
        builder.getUnfoldedTransitionCount() << " transitions, and " <<
        builder.getUnfoldedArcCount() << " arcs" << std::endl;
    std::cout << "Unfolded in " << builder.getUnfoldTime() << " seconds" << std::endl;
    if (options.computePartition) {
        std::cout << "Partitioned in " << builder.getPartitionTime() << " seconds" << std::endl;
    }

    //}
}

void writeQueries(const std::vector<std::shared_ptr<Condition>>&queries, std::vector<std::string>& querynames, std::vector<uint32_t>& order,
    std::string& filename, bool binary, const std::unordered_map<std::string, uint32_t>& place_names, bool compact) {
    std::fstream out;

    if (binary) {
        out.open(filename, std::ios::binary | std::ios::out);
        uint32_t cnt = 0;
        for (uint32_t j = 0; j < queries.size(); j++) {
            if (queries[j]->isTriviallyTrue() || queries[j]->isTriviallyFalse()) continue;
            ++cnt;
        }
        out.write(reinterpret_cast<const char *> (&cnt), sizeof (uint32_t));
        cnt = place_names.size();
        out.write(reinterpret_cast<const char *> (&cnt), sizeof (uint32_t));
        for (auto& kv : place_names) {
            out.write(reinterpret_cast<const char *> (&kv.second), sizeof (uint32_t));
            out.write(kv.first.data(), kv.first.size());
            out.write("\0", sizeof (char));
        }
    } else {
        out.open(filename, std::ios::out);
        out << "<?xml version=\"1.0\"?>\n<property-set xmlns=\"http://mcc.lip6.fr/\">\n";
    }

    for (uint32_t j = 0; j < queries.size(); j++) {
        auto i = order[j];
        if (queries[i]->isTriviallyTrue() || queries[i]->isTriviallyFalse()) continue;
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
    if (std::dynamic_pointer_cast<ACondition>(query) != nullptr) {
        wasACond = true;
        cond = (*std::dynamic_pointer_cast<SimpleQuantifierCondition>(query))[0];
    } else if (std::dynamic_pointer_cast<ECondition>(query) != nullptr) {
        wasACond = false;
        cond = (*std::dynamic_pointer_cast<SimpleQuantifierCondition>(query))[0];
    } else {
        wasACond = true;
        cond = query;
    }

    {
#ifdef VERIFYPN_MC_Simplification
        std::scoped_lock scopedLock{spot_mutex};
#endif
        cond = LTL::simplify(cond, options);
    }
    negstat_t stats;

    cond = pushNegation(initialMarkingRW([&]() {
        return cond; }, stats, evalContext, false, false, true),
        stats, evalContext, false, false, true);

    if (options.printstatistics) {
        out << "RWSTATS PRE:";
        stats.print(out);
        out << std::endl;
    }

    try {
        auto simp_cond = PetriEngine::PQL::simplify(cond, simplificationContext);
        cond = pushNegation(simp_cond.formula, stats, evalContext, false, false, true);
    }    catch (std::bad_alloc &ba) {
        throw base_error("Query reduction failed.\nException information: ", ba.what());
    }

    cond = initialMarkingRW([&]() {
        auto r = pushNegation(cond, stats, evalContext, false, false, true);
        {
#ifdef VERIFYPN_MC_Simplification
            std::scoped_lock scopedLock{spot_mutex};
#endif
            return LTL::simplify(r, options);
        }
    }, stats, evalContext, false, false, true);

    if (cond->isTriviallyTrue() || cond->isTriviallyFalse()) {
        // nothing
    } else if (wasACond) {
        cond = std::make_shared<ACondition>(cond);
    } else {
        cond = std::make_shared<ECondition>(cond);
    }
    if (options.printstatistics) {
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
    std::vector<std::string> &querynames, std::string filename, uint32_t binary_query_io) {
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
    writeQueries(queries, querynames, reorder, filename, binary_query_io & 2, builder.getPlaceNames());
}

void outputCompactQueries(const PetriNetBuilder &builder, const std::vector<PetriEngine::PQL::Condition_ptr> &queries,
    std::vector<std::string> &querynames, std::string filename) {
    //Don't know if this is needed
    std::vector<uint32_t> reorder(queries.size());
    for (uint32_t i = 0; i < queries.size(); ++i) reorder[i] = i;

    writeQueries(queries, querynames, reorder, filename, false, builder.getPlaceNames(), true);
}

void simplify_queries(  const MarkVal* marking,
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
                auto& out = std::cout;
#else
            auto simplify = [&, c]() {
                std::stringstream out;
#endif

                auto& cache = caches[c];
                while (true) {
                    auto i = cnt++;
                    if (i >= queries.size()) return;
                    if (!hadTo[i]) continue;
                    hadTo[i] = false;
                    negstat_t stats;
                    EvaluationContext context(marking, net);

                    if (options.printstatistics && options.queryReductionTimeout > 0) {
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
                        queries[i] = simplify_ltl_query(queries[i], options,
                            context, simplificationContext, out);
#ifdef VERIFYPN_MC_Simplification
                        out_lock.lock();
                        std::cout << out.str();
                        out.clear();
                        out_lock.unlock();
#endif
                        continue;
                    }
                    queries[i] = pushNegation(initialMarkingRW([&]() {
                        return queries[i]; }, stats, context, false, false, true),
                        stats, context, false, false, true);
                    wasAGCPNApprox |= dynamic_cast<NotCondition*> (queries[i].get()) != nullptr;

                    if (options.queryReductionTimeout > 0 && options.printstatistics) {
                        out << "RWSTATS PRE:";
                        stats.print(out);
                        out << std::endl;
                    }


                    if (options.queryReductionTimeout > 0 && qt > 0) {
                        SimplificationContext simplificationContext(marking, net, qt,
                            options.lpsolveTimeout, &cache);
                        try {
                            negstat_t stats;
                            auto simp_cond = PetriEngine::PQL::simplify(queries[i], simplificationContext);
                            queries[i] = pushNegation(simp_cond.formula, stats, context, false, false, true);
                            wasAGCPNApprox |= dynamic_cast<NotCondition*> (queries[i].get()) != nullptr;
                            if (options.printstatistics) {
                                out << "RWSTATS POST:";
                                stats.print(out);
                                out << std::endl;
                            }
                        } catch (std::bad_alloc& ba) {
                            throw base_error("Query reduction failed.\nException information: ", ba.what());
                        }

                        if (options.printstatistics) {
                            out << "\nQuery after reduction: ";
                            queries[i]->toString(out);
                            out << std::endl;
                        }
                        if (simplificationContext.timeout()) {
                            if (options.printstatistics)
                                out << "Query reduction reached timeout.\n";
                            hadTo[i] = true;
                        } else {
                            if (options.printstatistics)
                                out << "Query reduction finished after " << simplificationContext.getReductionTime() << " seconds.\n";
                            --to_handle;
                        }
                    } else if (options.printstatistics) {
                        out << "Skipping linear-programming (-q 0)" << std::endl;
                    }
                    if (options.cpnOverApprox && wasAGCPNApprox) {
                        if (queries[i]->isTriviallyTrue())
                            queries[i] = std::make_shared<BooleanCondition>(false);
                        else if (queries[i]->isTriviallyFalse())
                            queries[i] = std::make_shared<BooleanCondition>(true);
                        queries[i]->setInvariant(wasAGCPNApprox);
                    }


                    if (options.printstatistics) {
                        auto postSize = formulaSize(queries[i]);
                        double redPerc = preSize - postSize == 0 ? 0 : ((double) (preSize - postSize) / (double) preSize)*100;
                        out << "Query size reduced from " << preSize << " to " << postSize << " nodes ( " << redPerc << " percent reduction).\n";
                    }
#ifdef VERIFYPN_MC_Simplification
                    out_lock.lock();
                    std::cout << out.str();
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
            outstream << tstream[i].str();
            outstream << std::endl;
        }
#endif
        end = std::chrono::high_resolution_clock::now();

    } while (std::any_of(hadTo.begin(), hadTo.end(), [](auto a) {
            return a;
    }) && std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() < options.queryReductionTimeout && to_handle > 0);
}