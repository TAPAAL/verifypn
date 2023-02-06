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

using namespace PetriEngine;
using namespace PetriEngine::Colored;
namespace utf = boost::unit_test;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
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
                    strategy.reachable(vec, results, search, stub, false, false, trace, 0);
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
                    strategy.reachable(vec, results, search, stub, false, false, trace, 0);
                    BOOST_REQUIRE_EQUAL(expected[i], results[0]);
                }
            }
        }
    }
}
