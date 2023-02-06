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

BOOST_AUTO_TEST_CASE(HyperStutter, * utf::timeout(300)) {
    std::set<size_t> qnums{0, 1};
    std::vector<Reachability::ResultPrinter::Result> expected{
        ResultPrinter::Satisfied,
        ResultPrinter::Satisfied
    };

    auto [pn, conditions, qstrings] = load_pn("/models/hyper_stutter.pnml",
        "/models/hyper_stutter.xml", qnums);

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
                        if(trace)
                        {
                            auto& raw = search.raw_trace();


                            if(i == 0)
                            {
                                BOOST_REQUIRE_EQUAL(raw.size(), 6);
                                if(*pn->transitionNames()[raw[0][1]] == "T0")
                                    BOOST_REQUIRE_EQUAL(*pn->transitionNames()[raw[1][1]], "T1");
                                else
                                {
                                    BOOST_REQUIRE_EQUAL(*pn->transitionNames()[raw[0][1]], "T2");
                                    BOOST_REQUIRE_EQUAL(*pn->transitionNames()[raw[1][1]], "T3");
                                }
                                BOOST_REQUIRE_EQUAL(*pn->transitionNames()[raw[0][0]], "T2");
                                for(size_t k = 1; k < raw.size(); ++k)
                                    BOOST_REQUIRE_EQUAL(*pn->transitionNames()[raw[k][0]], "T4");
                            }

                            if(i == 1)
                            {
                                BOOST_REQUIRE_EQUAL(raw.size(), 2);
                                BOOST_REQUIRE_EQUAL(raw[0].size(), 2);
                                BOOST_REQUIRE_EQUAL(raw[1].size(), 2);
                                if(*pn->transitionNames()[raw[0][0]] == "T0")
                                    BOOST_REQUIRE_EQUAL(*pn->transitionNames()[raw[1][0]], "T1");
                                else
                                {
                                    BOOST_REQUIRE_EQUAL(*pn->transitionNames()[raw[0][0]], "T2");
                                    BOOST_REQUIRE_EQUAL(*pn->transitionNames()[raw[1][0]], "T3");
                                }
                                BOOST_REQUIRE_EQUAL(*pn->transitionNames()[raw[0][1]], "T2");
                                BOOST_REQUIRE_EQUAL(*pn->transitionNames()[raw[1][1]], "T4");
                            }
                        }
                    }
                }
            }
        }
    }
}