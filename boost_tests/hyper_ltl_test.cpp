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

#define BOOST_TEST_MODULE hyper_ltl

#include <boost/test/unit_test.hpp>
#include <string>
#include <fstream>
#include <sstream>

#include "utils.h"
#include "LTL/LTLSearch.h"
#include "CTL/SearchStrategy/HeuristicSearch.h"

using namespace PetriEngine;
using namespace PetriEngine::Colored;
namespace utf = boost::unit_test;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
}

BOOST_AUTO_TEST_CASE(SimpleHyperTest, * utf::timeout(300)) {
    std::set<size_t> qnums{0, 1};
    std::vector<Reachability::ResultPrinter::Result> expected{
        ResultPrinter::Satisfied,
        ResultPrinter::NotSatisfied
    };

    auto [pn, conditions, qstrings] = load_pn("/models/hyper1.pnml",
        "/models/hyper1.xml", qnums);

    for (auto i : qnums) {
        for (bool trace : {false, true}) {
            for (auto alg : {LTL::Algorithm::NDFS /*, LTL::Algorithm::Tarjan*/}) {
                for (auto por :{LTL::LTLPartialOrder::None/*, LTL::LTLPartialOrder::Liebke,
                        LTL::LTLPartialOrder::Visible, LTL::LTLPartialOrder::Automaton*/}) {
                    if (alg == LTL::Algorithm::NDFS && por != LTL::LTLPartialOrder::None)
                        continue;
                    for (auto heur : {LTL::LTLHeuristic::DFS/*, LTL::LTLHeuristic::Automaton, LTL::LTLHeuristic::Distance,
                            LTL::LTLHeuristic::FireCount*/}) {

                        std::cerr << "Q[" << i << "] trace=" << std::boolalpha << trace
                            << " por=" << to_underlying(por) << " alg=" << to_underlying(alg) << " heur=" << to_underlying(heur) << std::endl;
                        Strategy strategy = Strategy::HEUR;
                        if (heur == LTL::LTLHeuristic::DFS)
                            strategy = Strategy::HEUR;
                        LTL::LTLSearch search(*pn, conditions[i], LTL::BuchiOptimization::Low, LTL::APCompression::None);
                        auto r = search.solve(trace, 0, alg, por, strategy, heur, true);
                        auto result = r ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
                        BOOST_REQUIRE_EQUAL(expected[i], result);
                    }
                }
            }
        }
    }
}