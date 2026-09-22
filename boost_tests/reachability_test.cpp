/* Copyright (C) 2021 Peter G. Jensen <root@petergjoel.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_MODULE reachability

#include <boost/test/unit_test.hpp>
#include <string>
#include <fstream>
#include <sstream>

#include "utils.h"
#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/PrepareForReachability.h"
#include "utils/errors.h"

using namespace PetriEngine;
using namespace PetriEngine::Colored;
namespace utf = boost::unit_test;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
}

BOOST_AUTO_TEST_CASE(SudokuLTLFireabilityStubbornRegression) {
    for (int queryReductionTimeout : {0, 10}) {
        BOOST_TEST_CONTEXT("query reduction timeout " << queryReductionTimeout) {
            auto [pn, conditions, qstrings] = load_pn(
                "/models/sudoku_ltl_fireability.pnml",
                "/models/sudoku_ltl_fireability.xml", {0}, TemporalLogic::LTL);

            options_t options;
            options.logic = TemporalLogic::LTL;
            options.queryReductionTimeout = queryReductionTimeout;
            options.printstatistics = StatisticsLevel::None;
            std::ostringstream output;
            simplify_queries(pn->initial(), pn.get(), conditions, options, output);

            if (conditions[0]->isTriviallyFalse()) {
                continue;
            }

            BOOST_REQUIRE(!conditions[0]->isTriviallyTrue());
            auto query = prepareForReachability(conditions[0]);
            BOOST_REQUIRE(query);

            ResultHandler handler;
            ReachabilitySearch search(*pn, handler, 0);
            std::vector<Condition_ptr> queries{query};
            std::vector<ResultPrinter::Result> results{ResultPrinter::Unknown};
            search.reachable(queries, results, Strategy::DFS, true, false,
                             StatisticsLevel::None, false, 0);
            BOOST_REQUIRE_EQUAL(ResultPrinter::NotSatisfied, results[0]);
        }
    }
}

BOOST_AUTO_TEST_CASE(AngiogenesisPT01ReachabilityCardinality, * utf::timeout(60)) {

    std::set<size_t> qnums{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    std::vector<Reachability::ResultPrinter::Result> expected{
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied};

    auto [pn, conditions, qstrings] = load_pn("/models/Angiogenesis-PT-01/model.pnml",
        "/models/Angiogenesis-PT-01/ReachabilityCardinality.xml", qnums);

    ResultHandler handler;

    for (auto i : qnums) {
        for (auto search :{Strategy::BFS, Strategy::DFS, Strategy::HEUR, Strategy::RDFS}) {
            for (bool stub :{true, false}) {
                for (bool trace :{true, false}) {
                    auto c2 = prepareForReachability(conditions[i]);
                    ReachabilitySearch strategy(*pn, handler, 0);
                    std::vector<Condition_ptr> vec{c2};
                    std::vector<Reachability::ResultPrinter::Result> results{Reachability::ResultPrinter::Unknown};
                    strategy.reachable(vec, results, search, stub, false, StatisticsLevel::None, trace, 0);
                    BOOST_REQUIRE_EQUAL(expected[i], results[0]);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(AngiogenesisPT01ReachabilityFireability, * utf::timeout(60)) {

    std::set<size_t> qnums{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    std::vector<Reachability::ResultPrinter::Result> expected{
        ResultPrinter::NotSatisfied,
        ResultPrinter::NotSatisfied,
        ResultPrinter::Satisfied,
        ResultPrinter::NotSatisfied,
        ResultPrinter::NotSatisfied,
        ResultPrinter::Satisfied,
        ResultPrinter::Satisfied,
        ResultPrinter::Satisfied,
        ResultPrinter::Satisfied,
        ResultPrinter::NotSatisfied,
        ResultPrinter::Satisfied,
        ResultPrinter::NotSatisfied,
        ResultPrinter::Satisfied,
        ResultPrinter::NotSatisfied,
        ResultPrinter::Satisfied,
        ResultPrinter::NotSatisfied};

    auto [pn, conditions, qstrings] = load_pn("/models/Angiogenesis-PT-01/model.pnml",
        "/models/Angiogenesis-PT-01/ReachabilityFireability.xml", qnums);

    ResultHandler handler;

    for (auto i : qnums) {
        for (auto search :{Strategy::BFS, Strategy::DFS, Strategy::HEUR, Strategy::RDFS}) {
            for (bool stub :{true, false}) {
                for (bool trace :{true, false}) {
                    auto c2 = prepareForReachability(conditions[i]);
                    ReachabilitySearch strategy(*pn, handler, 0);
                    std::vector<Condition_ptr> vec{c2};
                    std::vector<Reachability::ResultPrinter::Result> results{Reachability::ResultPrinter::Unknown};
                    strategy.reachable(vec, results, search, stub, false, StatisticsLevel::None, trace, 0);
                    BOOST_REQUIRE_EQUAL(expected[i], results[0]);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(LargeMarkingRejectedWithoutInt64) {
    shared_string_set sset;
    PetriNetBuilder builder(sset);
    BOOST_REQUIRE_THROW(builder.addPlace("P", 3000000000ULL, 0, 0), base_error);
}

BOOST_AUTO_TEST_CASE(LargeMarkingAcceptedWithInt64) {
    shared_string_set sset;
    PetriNetBuilder builder(sset);
    builder.setInt64(true);
    builder.addPlace("P", 3000000000ULL, 0, 0);
    builder.addTransition("t", 0, 0, 0);
    std::unique_ptr<PetriNet> net(builder.makePetriNet());
    BOOST_REQUIRE(net->int64());
    BOOST_REQUIRE_EQUAL(net->initial(0), 3000000000ULL);

    auto query = ParseQuery(R"("P" >= 3000000000)", true);
    BOOST_REQUIRE(query);
    std::vector<Condition_ptr> analyzed{query};
    contextAnalysis(false, {}, {}, builder, net.get(), analyzed);
    auto reach = prepareForReachability(analyzed[0]);
    ResultHandler handler;
    ReachabilitySearch search(*net, handler, 0);
    std::vector<Condition_ptr> queries{reach};
    std::vector<ResultPrinter::Result> results{ResultPrinter::Unknown};
    search.reachable(queries, results, Strategy::DFS, false, false,
                     StatisticsLevel::None, false, 0);
    BOOST_REQUIRE_EQUAL(ResultPrinter::Satisfied, results[0]);
}
