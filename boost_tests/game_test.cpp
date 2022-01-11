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

#define BOOST_TEST_MODULE games

#include <boost/test/unit_test.hpp>
#include <string>
#include <fstream>
#include <sstream>

#include "utils.h"
#include "PetriEngine/Synthesis/ReachabilitySynthesis.h"

using namespace PetriEngine;
using namespace PetriEngine::Synthesis;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
}

void test_single_game(const char* fn, Reachability::ResultPrinter::Result expected, size_t quid = 0)
{
    std::string model =  std::string("/models/games/") + fn + ".pnml";
    std::string query =  std::string("/models/games/") + fn + ".xml";
    std::set<size_t> qnums{quid};
    auto [pn, conditions, qstrings] = load_pn(model.c_str(),
        query.c_str(), qnums);

    for (auto search : {Strategy::BFS, Strategy::DFS, Strategy::RDFS}) {
        Synthesis::ReachabilitySynthesis strategy(*pn, 0);
        for(auto permissive : {false, true})
        {
            for(auto stubborn : {false, true})
            {
                std::cerr << "Running " << fn << " query " << quid << " " << " permissive: " << std::boolalpha << permissive << " stubborn: " <<  stubborn << " search: " << (int)search << std::endl;
                conditions[0]->toString(std::cerr);
                std::cerr << std::endl;
                auto r = strategy.synthesize(*conditions[0], search, stubborn, permissive, nullptr);
                BOOST_REQUIRE_EQUAL(expected, r);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(Algorithm1Counterexample) {
    std::cerr << "Q1" << std::endl;
    test_single_game("algorithm 1 counterexample", Reachability::ResultPrinter::NotSatisfied, 0);
    std::cerr << "Q2" << std::endl;
    test_single_game("algorithm 1 counterexample", Reachability::ResultPrinter::NotSatisfied, 1);
    std::cerr << "Q3" << std::endl;
    test_single_game("algorithm 1 counterexample", Reachability::ResultPrinter::NotSatisfied, 2);
    std::cerr << "Q4" << std::endl;
    test_single_game("algorithm 1 counterexample", Reachability::ResultPrinter::Satisfied, 3);
}

BOOST_AUTO_TEST_CASE(Algorithm1Counterexample2) {
    test_single_game("algorithm 1 counterexample 2", Reachability::ResultPrinter::Satisfied);
}

BOOST_AUTO_TEST_CASE(CycleTestFalse) {
    test_single_game("cycle test false", Reachability::ResultPrinter::Satisfied);
}

BOOST_AUTO_TEST_CASE(CycleTestTrue2) {
    test_single_game("cycle test true 2", Reachability::ResultPrinter::Satisfied);
}

BOOST_AUTO_TEST_CASE(CycleTestTrue3) {
    test_single_game("cycle test true 3", Reachability::ResultPrinter::Satisfied);
}


BOOST_AUTO_TEST_CASE(Player2LessReduction) {
    test_single_game("player 2 less reduction", Reachability::ResultPrinter::NotSatisfied);
}

BOOST_AUTO_TEST_CASE(Player2) {
    test_single_game("player 2", Reachability::ResultPrinter::Satisfied);
}

BOOST_AUTO_TEST_CASE(SafeTest) {
    test_single_game("safe test", Reachability::ResultPrinter::Satisfied);
}

BOOST_AUTO_TEST_CASE(UnsafeTest) {
    test_single_game("unsafe test", Reachability::ResultPrinter::Satisfied);
}