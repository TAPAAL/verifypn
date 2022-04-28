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

#define BOOST_TEST_MODULE color

#include <boost/test/unit_test.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>

#include "utils.h"

using namespace PetriEngine;
using namespace PetriEngine::Colored;
namespace utf = boost::unit_test;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
}


BOOST_AUTO_TEST_CASE(InitialMarkingMismatch, * utf::timeout(5)) {

    std::string model("/models/color_mismatch.pnml");
    std::string query("/models/color_mismatch.xml");
    std::set<size_t> qnums{1};
    bool saw_exception = false;
    try {
        auto [pn, conditions, qstrings] = load_pn(model.c_str(),
            query.c_str(), qnums);
    } catch(base_error ex)
    {
        saw_exception = true;
    }
    BOOST_REQUIRE(saw_exception);
}

BOOST_AUTO_TEST_CASE(InitialMarkingMatch, * utf::timeout(5)) {

    std::string model("/models/color_match.pnml");
    std::string query("/models/color_match.xml");
    std::set<size_t> qnums{1};
    bool saw_exception = false;
    try {
        auto [pn, conditions, qstrings] = load_pn(model.c_str(),
            query.c_str(), qnums);
        BOOST_REQUIRE_GE(pn->numberOfPlaces(), 5);

        BOOST_REQUIRE_GE(pn->numberOfTransitions(), 5);
    } catch(base_error ex)
    {
        saw_exception = true;
    }

    BOOST_REQUIRE(!saw_exception);
}


BOOST_AUTO_TEST_CASE(PhilosophersDynCOL03, * utf::timeout(60)) {

    std::string model("/models/PhilosophersDyn-COL-03/model.pnml");
    std::string query("/models/PhilosophersDyn-COL-03/ReachabilityCardinality.xml");
    std::vector<Reachability::ResultPrinter::Result> expected{
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::Satisfied};
    ResultHandler handler;
    std::set<size_t> qnums{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    for(auto reduce : {false, true})
    {
        for(auto partition : {false, true})
        {
            for(auto symmetry : {false, true})
            {
                for(auto cfp : {false, true})
                {
                    for(auto approx : {false, true})
                    {
                        std::cerr << "\t" << model << ", " << query << std::boolalpha << " reduce=" << reduce << " partition=" << partition << " sym=" << symmetry << " cfp=" << cfp << " approx=" << approx << std::endl;
                        try {
                            auto [pn, conditions, qstrings] = load_pn(model.c_str(),
                                query.c_str(), qnums, TemporalLogic::CTL, reduce, partition, symmetry, cfp, approx);
                            for(auto i : qnums)
                            {
                                std::cerr << "\t\tQ[" << i << "] " << std::endl;
                                auto c2 = prepareForReachability(conditions[i]);
                                ReachabilitySearch strategy(*pn, handler, 0);
                                std::vector<Condition_ptr> vec{c2};
                                std::vector<Reachability::ResultPrinter::Result> results{Reachability::ResultPrinter::Unknown};
                                strategy.reachable(vec, results, Strategy::DFS, false, false, false, false, 0);
                                BOOST_REQUIRE(!approx);
                                BOOST_REQUIRE_EQUAL(expected[i], results[0]);
                            }
                        } catch (const base_error& er) {
                            BOOST_REQUIRE(approx);
                        }
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(PetersonCOL2, * utf::timeout(60)) {

    std::string model("/models/Peterson-COL-2/model.pnml");
    std::string query("/models/Peterson-COL-2/ReachabilityCardinality.xml");
    std::vector<Reachability::ResultPrinter::Result> expected{
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied};
    ResultHandler handler;
    std::set<size_t> qnums{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    for(auto reduce : {false, true})
    {
        for(auto partition : {false, true})
        {
            for(auto symmetry : {false, true})
            {
                for(auto cfp : {false, true})
                {
                    for(auto approx : {false, true})
                    {
                        std::cerr << "\t" << model << ", " << query << std::boolalpha << " reduce=" << reduce << " partition=" << partition << " sym=" << symmetry << " cfp=" << cfp << " approx=" << approx << std::endl;
                        try {
                            auto [pn, conditions, qstrings] = load_pn(model.c_str(),
                                query.c_str(), qnums, TemporalLogic::CTL, reduce, partition, symmetry, cfp, approx);
                            for(auto i : qnums)
                            {
                                std::cerr << "\t\tQ[" << i << "] " << std::endl;
                                auto c2 = prepareForReachability(conditions[i]);
                                ReachabilitySearch strategy(*pn, handler, 0);
                                std::vector<Condition_ptr> vec{c2};
                                std::vector<Reachability::ResultPrinter::Result> results{Reachability::ResultPrinter::Unknown};
                                strategy.reachable(vec, results, Strategy::DFS, false, false, false, false, 0);
                                if(!approx)
                                    BOOST_REQUIRE_EQUAL(expected[i], results[0]);
                                else
                                {
                                    // the solver does not know that it is solving approx.
                                    // result-printer actually deals with it, so cannot test for now
                                    //BOOST_REQUIRE(expected[i] == results[0] || // either solved correctly or not determined.
                                    //    (results[0] != Reachability::ResultPrinter::Satisfied && results[0] != Reachability::ResultPrinter::NotSatisfied));
                                }
                            }
                        } catch (const base_error& er) {
                            std::cerr << er.what() << std::endl;
                            BOOST_REQUIRE(false);
                        }
                    }
                }
            }
        }
    }
}


BOOST_AUTO_TEST_CASE(UtilityControlRoomCOLZ2T3N04, * utf::timeout(60)) {

    std::string model("/models/UtilityControlRoom-COL-Z2T3N04/model.pnml");
    std::string query("/models/UtilityControlRoom-COL-Z2T3N04/ReachabilityCardinality.xml");
    // this model is to large too verify as a test, but unfolding should be ok.
    // the unfolding provoked an error in the CFP of colored nets.
    std::set<size_t> qnums{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    for(auto reduce : {false, true})
    {
        for(auto partition : {false, true})
        {
            for(auto symmetry : {false, true})
            {
                for(auto cfp : {false, true})
                {
                    for(auto approx : {false, true})
                    {
                        std::cerr << "\t" << model << ", " << query << std::boolalpha << " reduce=" << reduce << " partition=" << partition << " sym=" << symmetry << " cfp=" << cfp << " approx=" << approx << std::endl;
                        try {
                            auto [pn, conditions, qstrings] = load_pn(model.c_str(),
                                query.c_str(), qnums, TemporalLogic::CTL, reduce, partition, symmetry, cfp, approx);
                            BOOST_REQUIRE(pn->numberOfTransitions() > 0);
                            BOOST_REQUIRE(pn->numberOfPlaces() > 0);
                        } catch (const base_error& er) {
                            std::cerr << er.what() << std::endl;
                            BOOST_REQUIRE(false);
                        }
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(NeoElectionCOL3, * utf::timeout(60)) {

    std::string model("/models/NeoElection-COL-3/model.pnml");
    std::string query("/models/NeoElection-COL-3/ReachabilityCardinality.xml");
    // this model is to large too verify as a test, but unfolding should be ok.
    // the unfolding provoked an error in the CFP of colored nets.
    std::set<size_t> qnums{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    for(auto reduce : {false, true})
    {
        for(auto partition : {false, true})
        {
            for(auto symmetry : {false, true})
            {
                for(auto cfp : {false, true})
                {
                    for(auto approx : {false, true})
                    {
                        std::cerr << "\t" << model << ", " << query << std::boolalpha << " reduce=" << reduce << " partition=" << partition << " sym=" << symmetry << " cfp=" << cfp << " approx=" << approx << std::endl;
                        try {
                            auto [pn, conditions, qstrings] = load_pn(model.c_str(),
                                query.c_str(), qnums, TemporalLogic::CTL, reduce, partition, symmetry, cfp, approx);
                            BOOST_REQUIRE(pn->numberOfTransitions() > 0);
                            BOOST_REQUIRE(pn->numberOfPlaces() > 0);
                        } catch (const base_error& er) {
                            std::cerr << er.what() << std::endl;
                            BOOST_REQUIRE(false);
                        }
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(RangeNotOne, * utf::timeout(1)) {

    std::string model("/models/intrangeNotOne/model.pnml");
    std::string query("/models/intrangeNotOne/query.xml");
    std::set<size_t> qnums{0};
    ResultHandler handler;
    for(auto reduce : {false, true})
    {
        for(auto partition : {false, true})
        {
            for(auto symmetry : {false, true})
            {
                for(auto cfp : {false, true})
                {
                    for(auto approx : {false, true})
                    {
                        std::cerr << "\t" << model << ", " << query << std::boolalpha << " reduce=" << reduce << " partition=" << partition << " sym=" << symmetry << " cfp=" << cfp << " approx=" << approx << std::endl;
                        try {
                            auto [pn, conditions, qstrings] = load_pn(model.c_str(),
                                query.c_str(), qnums, TemporalLogic::CTL, reduce, partition, symmetry, cfp, approx);
                            BOOST_REQUIRE(pn->numberOfTransitions() > 0);
                            BOOST_REQUIRE(pn->numberOfPlaces() > 0);
                            auto c2 = prepareForReachability(conditions[0]);
                            ReachabilitySearch strategy(*pn, handler, 0);
                            std::vector<Condition_ptr> vec{c2};
                            std::vector<Reachability::ResultPrinter::Result> results{Reachability::ResultPrinter::Unknown};
                            strategy.reachable(vec, results, Strategy::DFS, false, false, false, false, 0);
                            if(!approx)
                                BOOST_REQUIRE_EQUAL(Reachability::ResultPrinter::Satisfied, results[0]);
                        } catch (const base_error& er) {
                            std::cerr << er.what() << std::endl;
                            BOOST_REQUIRE(false);
                        }
                    }
                }
            }
        }
    }
}


BOOST_AUTO_TEST_CASE(UnfoldLoop, * utf::timeout(1)) {

    std::string model("/models/unfolding_loop.pnml");
    std::string query("/models/unfolding_loop.xml");
    std::set<size_t> qnums{0};
    ResultHandler handler;
    for(auto reduce : {false, true})
    {
        for(auto partition : {false, true})
        {
            for(auto symmetry : {false, true})
            {
                for(auto cfp : {false, true})
                {
                    for(auto approx : {false, true})
                    {
                        std::cerr << "\t" << model << ", " << query << std::boolalpha << " reduce=" << reduce << " partition=" << partition << " sym=" << symmetry << " cfp=" << cfp << " approx=" << approx << std::endl;
                        try {
                            auto [pn, conditions, qstrings] = load_pn(model.c_str(),
                                query.c_str(), qnums, TemporalLogic::CTL, reduce, partition, symmetry, cfp, approx);
                            BOOST_REQUIRE(pn->numberOfPlaces() > 0);
                            auto c2 = prepareForReachability(conditions[0]);
                            ReachabilitySearch strategy(*pn, handler, 0);
                            std::vector<Condition_ptr> vec{c2};
                            std::vector<Reachability::ResultPrinter::Result> results{Reachability::ResultPrinter::Unknown};
                            strategy.reachable(vec, results, Strategy::DFS, false, false, false, false, 0);
                            if(!approx)
                                BOOST_REQUIRE_EQUAL(Reachability::ResultPrinter::NotSatisfied, results[0]);
                        } catch (const base_error& er) {
                            std::cerr << er.what() << std::endl;
                            BOOST_REQUIRE(false);
                        }
                    }
                }
            }
        }
    }
}