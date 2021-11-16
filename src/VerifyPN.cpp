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
#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <memory>
#include <utility>
#include <functional>
// #include <filesystem>
// #include <bits/stdc++.h>
// #include <iostream>
// #include <sys/stat.h>
// #include <sys/types.h>
#ifdef VERIFYPN_MC_Simplification
#include <thread>
#include <iso646.h>
#include <mutex>
#endif

#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "PetriEngine/TAR/TARReachability.h"
#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"
#include "PetriParse/QueryBinaryParser.h"
#include "PetriParse/PNMLParser.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/CTLVisitor.h"
#include <PetriEngine/PQL/XMLPrinter.h>
#include "PetriEngine/options.h"
#include "PetriEngine/errorcodes.h"
#include "PetriEngine/STSolver.h"
#include "PetriEngine/Simplification/Member.h"
#include "PetriEngine/Simplification/LinearPrograms.h"
#include "PetriEngine/Simplification/Retval.h"

#include "CTL/CTLEngine.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "LTL/LTL.h"
#include "PetriEngine/TraceReplay.h"
#include "LTL/LTLMain.h"

#include <atomic>
#include <PetriEngine/PQL/BinaryPrinter.h>
#include <PetriEngine/PQL/Simplifier.h>
#include <PetriEngine/PQL/PushNegation.h>

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

ReturnValue contextAnalysis(ColoredPetriNetBuilder& cpnBuilder, PetriNetBuilder& builder, const PetriNet* net, std::vector<std::shared_ptr<Condition> >& queries)
{
    //Context analysis
    ColoredAnalysisContext context(builder.getPlaceNames(), builder.getTransitionNames(), net, cpnBuilder.getUnfoldedPlaceNames(), cpnBuilder.getUnfoldedTransitionNames(), cpnBuilder.isColored());
    for(auto& q : queries)
    {
        q->analyze(context);

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


auto
readQueries(options_t& options, std::vector<std::string>& qstrings)
{

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

        if(options.querynumbers.size() == 0)
        {
            std::string querystring; // excluding EF and AG

            //Read everything
            std::stringstream buffer;
            buffer << qfile.rdbuf();
            std::string querystr = buffer.str(); // including EF and AG
            //Parse XML the queries and querystr let be the index of xmlquery

            qstrings.push_back(querystring);
            //Validate query type
            if (querystr.substr(0, 2) != "EF" && querystr.substr(0, 2) != "AG") {
                    fprintf(stderr, "Error: Query type \"%s\" not supported, only (EF and AG is supported)\n", querystr.substr(0, 2).c_str());
                    return conditions;
            }
            //Check if is invariant
            bool isInvariant = querystr.substr(0, 2) == "AG";

            //Wrap in not if isInvariant
            querystring = querystr.substr(2);
            std::vector<std::string> tmp;
            conditions.emplace_back(ParseQuery(querystring, tmp));
            if(isInvariant) conditions.back() = std::make_shared<AGCondition>(conditions.back());
            else            conditions.back() = std::make_shared<EFCondition>(conditions.back());
        }
        else
        {
            std::vector<QueryItem> queries;
            if(options.binary_query_io & 1)
            {
                QueryBinaryParser parser;
                if (!parser.parse(qfile, options.querynumbers)) {
                    fprintf(stderr, "Error: Failed parsing binary query file\n");
                    fprintf(stdout, "DO_NOT_COMPETE\n");
                    conditions.clear();
                    return conditions;
                }
                queries = std::move(parser.queries);
            }
            else
            {
                QueryXMLParser parser;
                if (!parser.parse(qfile, options.querynumbers)) {
                    fprintf(stderr, "Error: Failed parsing XML query file\n");
                    fprintf(stdout, "DO_NOT_COMPETE\n");
                    conditions.clear();
                    return conditions;
                }
                queries = std::move(parser.queries);
            }

            size_t i = 0;
            for(auto& q : queries)
            {
                if(!options.querynumbers.empty()
                        && options.querynumbers.count(i) == 0)
                {
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
        }
        qfile.close();
        return conditions;
    } else { // state-space exploration
        std::string querystring = "false";
        std::vector<std::string> empty;
        conditions.push_back(std::make_shared<EFCondition>(ParseQuery(querystring, empty)));
        return conditions;
    }
 }

ReturnValue parseModel(AbstractPetriNetBuilder& builder, options_t& options)
{
    //Load the model
    std::ifstream mfile(options.modelfile, std::ifstream::in);
    if (!mfile) {
        fprintf(stderr, "Error: Model file \"%s\" couldn't be opened\n", options.modelfile);
        fprintf(stdout, "CANNOT_COMPUTE\n");
        return ErrorCode;
    }


    //Parse and build the petri net
    PNMLParser parser;
    parser.parse(mfile, &builder);
    options.isCPN = builder.isColored();

    // Close the file
    mfile.close();
    return ContinueCode;
}

void printStats(PetriNetBuilder& builder, options_t& options)
{
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

            std::cout   << "\nNet reduction is enabled.\n";
            builder.printStats(std::cout);
        }
    }
}

void printUnfoldingStats(ColoredPetriNetBuilder& builder, options_t& options) {
    //if (options.printstatistics) {
        if (!builder.isColored() && !builder.isUnfolded())
            return;
        if(options.computeCFP){
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
        if(options.computePartition){
            std::cout << "Partitioned in " << builder.getPartitionTime() << " seconds" << std::endl;
        }

    //}
}

std::string getXMLQueries(std::vector<std::shared_ptr<Condition>> queries, std::vector<std::string> querynames, std::vector<ResultPrinter::Result> results) {
    bool cont = false;
    for(uint32_t i = 0; i < results.size(); i++) {
        if (results[i] == ResultPrinter::CTL) {
            cont = true;
            break;
        }
    }

    if (!cont) {
        return "";
    }

    std::stringstream ss;
    ss << "<?xml version=\"1.0\"?>\n<property-set xmlns=\"http://mcc.lip6.fr/\">\n";

    for(uint32_t i = 0; i < queries.size(); i++) {
        if (!(results[i] == ResultPrinter::CTL)) {
            continue;
        }
        ss << "  <property>\n    <id>" << querynames[i] << "</id>\n    <description>Simplified</description>\n    <formula>\n";
        XMLPrinter xml_printer(ss, 3);
        queries[i]->visit(xml_printer);
        ss << "    </formula>\n  </property>\n";
    }

    ss << "</property-set>\n";

    return ss.str();
}

void writeQueries(const std::vector<std::shared_ptr<Condition>>& queries, std::vector<std::string>& querynames, std::vector<uint32_t>& order,
    std::string& filename, bool binary, const std::unordered_map<std::string, uint32_t>& place_names)
{
    std::fstream out;

    if(binary)
    {
        out.open(filename, std::ios::binary | std::ios::out);
        uint32_t cnt = 0;
        for(uint32_t j = 0; j < queries.size(); j++) {
            if(queries[j]->isTriviallyTrue() || queries[j]->isTriviallyFalse()) continue;
            ++cnt;
        }
        out.write(reinterpret_cast<const char *>(&cnt), sizeof(uint32_t));
        cnt = place_names.size();
        out.write(reinterpret_cast<const char *>(&cnt), sizeof(uint32_t));
        for(auto& kv : place_names)
        {
            out.write(reinterpret_cast<const char *>(&kv.second), sizeof(uint32_t));
            out.write(kv.first.data(), kv.first.size());
            out.write("\0", sizeof(char));
        }
    }
    else
    {
        out.open(filename, std::ios::out);
        out << "<?xml version=\"1.0\"?>\n<property-set xmlns=\"http://mcc.lip6.fr/\">\n";
    }

    for(uint32_t j = 0; j < queries.size(); j++) {
        auto i = order[j];
        if(queries[i]->isTriviallyTrue() || queries[i]->isTriviallyFalse()) continue;
        if(binary)
        {
            out.write(querynames[i].data(), querynames[i].size());
            out.write("\0", sizeof(char));
            BinaryPrinter binary_printer(out);
            queries[i]->visit(binary_printer);
        }
        else
        {
            out << "  <property>\n    <id>" << querynames[i] << "</id>\n    <description>Simplified</description>\n    <formula>\n";
            XMLPrinter xml_printer(out, 3);
            queries[i]->visit(xml_printer);
            out << "    </formula>\n  </property>\n";
        }
    }

    if(binary == 0)
    {
        out << "</property-set>\n";
    }
    out.close();
}

void writeCompactQueries(const std::vector<std::shared_ptr<Condition>>& queries, std::vector<std::string>& querynames, std::vector<uint32_t>& order,
    std::string& filename, const ColoredPetriNetBuilder& cpnBuilder, const PetriNetBuilder& builder, const PetriNet* net)
{
    std::fstream out;
    ColoredAnalysisContext context(builder.getPlaceNames(), builder.getTransitionNames(), net, cpnBuilder.getUnfoldedPlaceNames(), cpnBuilder.getUnfoldedTransitionNames(), cpnBuilder.isColored());

    out.open(filename, std::ios::out);
    out << "<?xml version=\"1.0\"?>\n<property-set xmlns=\"http://mcc.lip6.fr/\">\n";

    for(uint32_t j = 0; j < queries.size(); j++) {
        auto i = order[j];
        if(queries[i]->isTriviallyTrue() || queries[i]->isTriviallyFalse()) continue;

            out << "  <property>\n    <id>" << querynames[i] << "</id>\n    <description>Simplified</description>\n    <formula>\n";
            XMLPrinter xml_printer(out, 0);
            queries[i]->visit(xml_printer);
    }

    out << "</property-set>\n";

    out.close();
}

std::vector<Condition_ptr> getCTLQueries(const std::vector<Condition_ptr>& ctlStarQueries) {
    std::vector<Condition_ptr> ctlQueries;
    for (const auto &ctlStarQuery : ctlStarQueries) {
        IsCTLVisitor isCtlVisitor;
        ctlStarQuery->visit(isCtlVisitor);
        if (isCtlVisitor.isCTL) {
            AsCTL asCtl;
            ctlStarQuery->visit(asCtl);
            ctlQueries.push_back(asCtl._ctl_query);
        } else {
            std::cerr << "Error: A query could not be translated from CTL* to CTL." << std::endl;
            exit(1);
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
            std::cerr << "Error: a query could not be translated from CTL* to LTL." << std::endl;
            exit(1);
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
                                 std::ostream &out = std::cout) {
    Condition_ptr cond;
    bool wasACond;
    if (std::dynamic_pointer_cast<ACondition>(query) != nullptr) {
        wasACond = true;
        cond = (*std::dynamic_pointer_cast<SimpleQuantifierCondition>(query))[0];
    }
    else if (std::dynamic_pointer_cast<ECondition>(query) != nullptr) {
        wasACond = false;
        cond = (*std::dynamic_pointer_cast<SimpleQuantifierCondition>(query))[0];
    }
    else {
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

    cond = pushNegation(initialMarkingRW([&]() { return cond; }, stats, evalContext, false, false, true),
                       stats, evalContext, false, false, true);

    if (options.printstatistics) {
        out << "RWSTATS PRE:";
        stats.print(out);
        out << std::endl;
    }

    try {
        auto simp_cond = PetriEngine::PQL::simplify(cond, simplificationContext);
        cond = pushNegation(simp_cond.formula, stats, evalContext, false, false, true);
    }
    catch (std::bad_alloc &ba) {
        std::cerr << "Query reduction failed." << std::endl;
        std::cerr << "Exception information: " << ba.what() << std::endl;

        std::exit(ErrorCode);
    }

    cond = initialMarkingRW([&]() {
#ifdef VERIFYPN_MC_Simplification
        std::scoped_lock scopedLock{spot_mutex};
#endif
        return LTL::simplify(pushNegation(cond, stats, evalContext, false, false, true), options);
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
        std::vector<std::string> &querynames, std::string filename, uint32_t binary_query_io){
    std::vector<uint32_t> reorder(queries.size());
    for(uint32_t i = 0; i < queries.size(); ++i) reorder[i] = i;
    std::sort(reorder.begin(), reorder.end(), [&](auto a, auto b){

        if(queries[a]->isReachability() != queries[b]->isReachability())
            return queries[a]->isReachability() > queries[b]->isReachability();
        if(queries[a]->isLoopSensitive() != queries[b]->isLoopSensitive())
            return queries[a]->isLoopSensitive() < queries[b]->isLoopSensitive();
        if(queries[a]->containsNext() != queries[b]->containsNext())
            return queries[a]->containsNext() < queries[b]->containsNext();
        return queries[a]->formulaSize() < queries[b]->formulaSize();
    });
    writeQueries(queries, querynames, reorder, filename, binary_query_io & 2, builder.getPlaceNames());
}

void outputCompactQueries(const PetriNetBuilder &builder, const PetriNetBuilder &b2, const PetriNet *net,
        const PetriEngine::ColoredPetriNetBuilder& cpnBuilder, const std::vector<PetriEngine::PQL::Condition_ptr> &queries,
        std::vector<std::string> &querynames, std::string filename){
    //Don't know if this is needed
    std::vector<uint32_t> reorder(queries.size());
    for(uint32_t i = 0; i < queries.size(); ++i) reorder[i] = i;

    writeCompactQueries(queries, querynames, reorder, filename,cpnBuilder, b2, net);
}


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
    if(parseModel(cpnBuilder, options) != ContinueCode)
    {
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
            if (!q->isReachability() || q->isLoopSensitive() || stats.negated_fireability) {
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
        //cpnBuilder.printSymmetricVariables();
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
    std::unique_ptr<PetriNet> qnet(b2.makePetriNet(false));
    std::unique_ptr<MarkVal[]> qm0(qnet->makeInitialMarking());
    ResultPrinter p2(&b2, &options, querynames);

    if(options.unfold_query_out_file.size() > 0) {
        outputCompactQueries(builder, b2, qnet.get(), cpnBuilder, queries, querynames, options.unfold_query_out_file);
    }


    if(queries.size() == 0 || contextAnalysis(cpnBuilder, b2, qnet.get(), queries) != ContinueCode)
    {
        std::cerr << "Could not analyze the queries" << std::endl;
        return ErrorCode;
    }

    // simplification. We always want to do negation-push and initial marking check.
    {
        // simplification. We always want to do negation-push and initial marking check.
        std::vector<LPCache> caches(options.cores);
        std::atomic<uint32_t> to_handle(queries.size());
        auto begin = std::chrono::high_resolution_clock::now();
        auto end = std::chrono::high_resolution_clock::now();
        std::vector<bool> hadTo(queries.size(), true);

        do
        {
            auto qt = (options.queryReductionTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / ( 1 + (to_handle / options.cores));
            if((to_handle <= options.cores || options.cores == 1) && to_handle > 0)
                qt = (options.queryReductionTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / to_handle;
            std::atomic<uint32_t> cnt(0);
#ifdef VERIFYPN_MC_Simplification

            std::vector<std::thread> threads;
#endif
            std::vector<std::stringstream> tstream(queries.size());
            uint32_t old = to_handle;
            for(size_t c = 0; c < std::min<uint32_t>(options.cores, old); ++c)
            {
#ifdef VERIFYPN_MC_Simplification
                threads.push_back(std::thread([&,c](){
#else
                auto simplify = [&,c](){
#endif
                    auto& out = tstream[c];
                    auto& cache = caches[c];
                    LTL::FormulaToSpotSyntax printer{out};
                    while(true)
                    {
                    auto i = cnt++;
                    if(i >= queries.size()) return;
                    if(!hadTo[i]) continue;
                    hadTo[i] = false;
                    negstat_t stats;
                    EvaluationContext context(qm0.get(), qnet.get());

                    if(options.printstatistics && options.queryReductionTimeout > 0)
                    {
                        out << "\nQuery before reduction: ";
                        queries[i]->toString(out);
                        out << std::endl;
                    }

#ifndef VERIFYPN_MC_Simplification
                    qt = (options.queryReductionTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / (queries.size() - i);
#endif
                    // this is used later, we already know that this is a plain reachability (or AG)
                    int preSize=queries[i]->formulaSize();

                    bool wasAGCPNApprox = dynamic_cast<NotCondition*>(queries[i].get()) != nullptr;
                    if (options.logic == TemporalLogic::LTL) {
                        if (options.queryReductionTimeout == 0) continue;
                        SimplificationContext simplificationContext(qm0.get(), qnet.get(), qt,
                                                                    options.lpsolveTimeout, &cache);
                        queries[i] = simplify_ltl_query(queries[i], options,
                                           context, simplificationContext, out);
                        continue;
                    }
                    queries[i] = pushNegation(initialMarkingRW([&](){ return queries[i]; }, stats,  context, false, false, true),
                                            stats, context, false, false, true);
                    queries[i]->toString(std::cout);std::cout << std::endl;
                    wasAGCPNApprox |= dynamic_cast<NotCondition*>(queries[i].get()) != nullptr;

                    if(options.queryReductionTimeout > 0 && options.printstatistics) {
                        out << "RWSTATS PRE:";
                        stats.print(out);
                        out << std::endl;
                    }


                    if (options.queryReductionTimeout > 0 && qt > 0)
                    {
                        SimplificationContext simplificationContext(qm0.get(), qnet.get(), qt,
                                options.lpsolveTimeout, &cache);
                        try {
                            negstat_t stats;
                            auto simp_cond = PetriEngine::PQL::simplify(queries[i], simplificationContext);
                            queries[i] = pushNegation(simp_cond.formula, stats, context, false, false, true);
                            wasAGCPNApprox |= dynamic_cast<NotCondition*>(queries[i].get()) != nullptr;
                            if(options.printstatistics)
                            {
                                out << "RWSTATS POST:";
                                stats.print(out);
                                out << std::endl;
                            }
                        } catch (std::bad_alloc& ba){
                            std::cerr << "Query reduction failed." << std::endl;
                            std::cerr << "Exception information: " << ba.what() << std::endl;

                            std::exit(ErrorCode);
                        }

                        if(options.printstatistics)
                        {
                            out << "\nQuery after reduction: ";
                            queries[i]->toString(out);
                            out << std::endl;
                        }
                        if(simplificationContext.timeout()){
                            if(options.printstatistics)
                                out << "Query reduction reached timeout.\n";
                            hadTo[i] = true;
                        } else {
                            if(options.printstatistics)
                                out << "Query reduction finished after " << simplificationContext.getReductionTime() << " seconds.\n";
                            --to_handle;
                        }
                    }
                    else if(options.printstatistics)
                    {
                        out << "Skipping linear-programming (-q 0)" << std::endl;
                    }
                    if(options.cpnOverApprox && wasAGCPNApprox)
                    {
                        if(queries[i]->isTriviallyTrue())
                            queries[i] = std::make_shared<BooleanCondition>(false);
                        else if(queries[i]->isTriviallyFalse())
                            queries[i] = std::make_shared<BooleanCondition>(true);
                        queries[i]->setInvariant(wasAGCPNApprox);
                    }


                    if(options.printstatistics)
                    {
                        int postSize=queries[i]->formulaSize();
                        double redPerc = preSize-postSize == 0 ? 0 : ((double)(preSize-postSize)/(double)preSize)*100;
                        out << "Query size reduced from " << preSize << " to " << postSize << " nodes ( " << redPerc << " percent reduction).\n";
                    }
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
            std::cout << tstream[0].str() << std::endl;
            break;
#else
            for(size_t i = 0; i < std::min<uint32_t>(options.cores, old); ++i)
            {
                threads[i].join();
                std::cout << tstream[i].str();
                std::cout << std::endl;
            }
#endif
            end = std::chrono::high_resolution_clock::now();

        } while(std::any_of(hadTo.begin(), hadTo.end(), [](auto a) { return a;}) && std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() < options.queryReductionTimeout && to_handle > 0);
    }

    if(options.query_out_file.size() > 0) {
        outputQueries(builder, queries, querynames, options.query_out_file, options.binary_query_io);

    }

    qnet = nullptr;
    qm0 = nullptr;

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
            } else if (options.strategy == PetriEngine::Reachability::OverApprox){
                results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Unknown).first;
                if (options.printstatistics) {
                    std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                }
            } else if (options.noreach || !queries[i]->isReachability()) {
                results[i] = options.logic == TemporalLogic::CTL ? ResultPrinter::CTL : ResultPrinter::LTL;
                alldone = false;
            } else {
                queries[i] = queries[i]->prepareForReachability();
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

    if(options.doVerification){

        //----------------------- Verify CTL queries -----------------------//
        std::vector<size_t> ctl_ids;
        std::vector<size_t> ltl_ids;
        for(size_t i = 0; i < queries.size(); ++i)
        {
            if(results[i] == ResultPrinter::CTL)
            {
                ctl_ids.push_back(i);
            }
            else if (results[i] == ResultPrinter::LTL) {
                ltl_ids.push_back(i);
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
            options.usedctl=true;
            PetriEngine::Reachability::Strategy reachabilityStrategy=options.strategy;

            // Assign indexes
            if(queries.empty() || contextAnalysis(cpnBuilder, builder, net.get(), queries) != ContinueCode)
            {
                std::cerr << "An error occurred while assigning indexes" << std::endl;
                return ErrorCode;
            }
            if(options.strategy == DEFAULT) options.strategy = PetriEngine::Reachability::DFS;
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
            options.strategy=reachabilityStrategy;
        }
        options.usedctl=false;

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
        if(options.strategy == DEFAULT) options.strategy = PetriEngine::Reachability::HEUR;

        if(options.tar && net->numberOfPlaces() > 0)
        {
            //Create reachability search strategy
            TARReachabilitySearch strategy(printer, *net, builder.getReducer(), options.kbound);

            // Change default place-holder to default strategy
            fprintf(stdout, "Search strategy option was ignored as the TAR engine is called.\n");
            options.strategy = PetriEngine::Reachability::DFS;

            //Reachability search
            strategy.reachable(queries, results,
                    options.printstatistics,
                    options.trace != TraceLevel::None);
        }
        else
        {
            ReachabilitySearch strategy(*net, printer, options.kbound);

            // Change default place-holder to default strategy
            if(options.strategy == DEFAULT) options.strategy = PetriEngine::Reachability::HEUR;

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
