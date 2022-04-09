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

#define BOOST_TEST_MODULE reduction

#include <boost/test/unit_test.hpp>
#include <string>
#include <fstream>
#include <sstream>

#include "LTL/LTLSearch.h"
#include "utils.h"

using namespace PetriEngine;
using namespace PetriEngine::Colored;
namespace utf = boost::unit_test;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
}

BOOST_AUTO_TEST_CASE(ruleD, * utf::timeout(60)) {

    std::set<size_t> qnums{0};
    std::vector<Reachability::ResultPrinter::Result> expected{
        Reachability::ResultPrinter::Satisfied};
    std::vector<Reachability::ResultPrinter::Result> results{
        Reachability::ResultPrinter::Unknown};

    auto [conditions, builder, qstrings, trans_names, place_names] = load_builder("/models/rule_D.pnml",
        "/models/rule_D.xml", qnums);
    PetriNetBuilder& bld = builder;
    std::vector<uint32_t> reds;
    std::unique_ptr<PetriNet> net{bld.makePetriNet(false)};
    contextAnalysis(false, trans_names, place_names, builder, net.get(), conditions);
    bld.reduce(conditions, results, 3, false, net.get(), 10, reds);
    net.reset(builder.makePetriNet(false));
    contextAnalysis(false, trans_names, place_names, builder, net.get(), conditions);


    for (auto i : qnums) {
        std::vector<Reachability::ResultPrinter::Result> results{Reachability::ResultPrinter::Unknown};
        LTL::LTLSearch search(*net, conditions[i], LTL::BuchiOptimization::Low, LTL::APCompression::None);
        auto r = search.solve(false, 0, LTL::Algorithm::Tarjan,  LTL::LTLPartialOrder::None, Strategy::DFS, LTL::LTLHeuristic::DFS, true);
        auto result = r ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
        BOOST_REQUIRE_EQUAL(expected[i], result);
    }
}
