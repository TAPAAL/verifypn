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
#include <map>

#include "utils.h"
#include "PetriEngine/Colored/PnmlWriter.h"

using namespace PetriEngine;
using namespace PetriEngine::Colored;
namespace utf = boost::unit_test;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
}

BOOST_AUTO_TEST_CASE(InitialMarkingMismatch, * utf::timeout(10)) {

    std::string model("/models/color_mismatch.pnml");
    std::string query("/models/color_mismatch.xml");
    std::set<size_t> qnums{1};
    bool saw_exception = false;
    try {
        auto [pn, conditions, qstrings] = load_pn(model.c_str(),
            query.c_str(), qnums);
    } catch(base_error& ex)
    {
        saw_exception = true;
    }
    BOOST_REQUIRE(saw_exception);
}

BOOST_AUTO_TEST_CASE(InitialMarkingMatch, * utf::timeout(10)) {

    std::string model("/models/color_match.pnml");
    std::string query("/models/color_match.xml");
    std::set<size_t> qnums{1};
    bool saw_exception = false;
    try {
        auto [pn, conditions, qstrings] = load_pn(model.c_str(),
            query.c_str(), qnums);
        BOOST_REQUIRE_GE(pn->numberOfPlaces(), 5);

        BOOST_REQUIRE_GE(pn->numberOfTransitions(), 5);
    } catch(base_error& ex)
    {
        saw_exception = true;
    }

    BOOST_REQUIRE(!saw_exception);
}

BOOST_AUTO_TEST_CASE(InitialMarkingAllSubtractionHandled, * utf::timeout(10)) {

    std::string model("/models/initial-all-expression.pnml");

    std::set<size_t> qnums{1};
    bool saw_exception = false;
    try {
        shared_string_set sset;
        ColoredPetriNetBuilder originalCpnBuilder(sset);
        auto modelStream = loadFile(model.c_str());
        originalCpnBuilder.parse_model(modelStream);
        originalCpnBuilder.sort();

        std::stringstream writtenNet;
        PnmlWriter writer(originalCpnBuilder, writtenNet);
        writer.toColPNML();

        IntialMarkingCollector initialMarkingCollector;
        initialMarkingCollector.parse_model(writtenNet);
        auto p0 = initialMarkingCollector.getInitialMarking("P0");
        auto p2 = initialMarkingCollector.getInitialMarking("P2");
        auto p3 = initialMarkingCollector.getInitialMarking("P3");

        //test P0
        BOOST_REQUIRE_EQUAL(p0.size(), 2);
        for (const auto& [color, count] : p0) {
            if (color->getColorName() == "a") {
                BOOST_REQUIRE_EQUAL(count, 0);
            } else if (color->getColorName() == "b") {
                BOOST_REQUIRE_EQUAL(count, 1);
            } else if (color->getColorName() == "c") {
                BOOST_REQUIRE_EQUAL(count, 1);
            } else {
                BOOST_REQUIRE(false);
            }
        }

        //test P2
        BOOST_REQUIRE_EQUAL(p2.size(), 0);

        //test P3
        BOOST_REQUIRE_EQUAL(p3.size(), 3);
        for (const auto& [color, count] : p3) {
            if (color->getColorName() == "a") {
                BOOST_REQUIRE_EQUAL(count, 1);
            } else if (color->getColorName() == "b") {
                BOOST_REQUIRE_EQUAL(count, 1);
            } else if (color->getColorName() == "c") {
                BOOST_REQUIRE_EQUAL(count, 1);
            } else {
                BOOST_REQUIRE(false);
            }
        }
    } catch(base_error& ex)
    {
        saw_exception = true;
    }

    BOOST_REQUIRE(!saw_exception);
}

BOOST_AUTO_TEST_CASE(PhilosophersDynCOL03, * utf::timeout(100)) {
    std::string model("/models/PhilosophersDyn-COL-03/model.pnml");
    std::string query("/models/PhilosophersDyn-COL-03/ReachabilityCardinality.xml");
    std::set<size_t> qnums{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
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
        Reachability::ResultPrinter::Satisfied
    };
    runReachabilityMatrixTest(
        model,
        query,
        multiQueryExpect(expected, qnums),
        TemporalLogic::CTL,
        qnums
    );
}

BOOST_AUTO_TEST_CASE(PetersonCOL2, * utf::timeout(100)) {
    std::string model("/models/Peterson-COL-2/model.pnml");
    std::string query("/models/Peterson-COL-2/ReachabilityCardinality.xml");
    std::set<size_t> qnums{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
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
        Reachability::ResultPrinter::Satisfied
    };
    runReachabilityMatrixTest(
        model,
        query,
        multiQueryExpect(expected, qnums),
        TemporalLogic::CTL,
        qnums
    );
}

BOOST_AUTO_TEST_CASE(UtilityControlRoomCOLZ2T3N04, * utf::timeout(100)) {

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

BOOST_AUTO_TEST_CASE(NeoElectionCOL3, * utf::timeout(100)) {

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

BOOST_AUTO_TEST_CASE(RangeNotOne, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/intrangeNotOne/model.pnml",
        "/models/intrangeNotOne/query.xml",
        singleQueryExpect(Reachability::ResultPrinter::Satisfied)
    );
}

BOOST_AUTO_TEST_CASE(UnfoldLoop, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/unfolding_loop.pnml",
        "/models/unfolding_loop.xml",
        singleQueryExpect(Reachability::ResultPrinter::NotSatisfied)
    );
}

BOOST_AUTO_TEST_CASE(AllPlaceInterval, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/all_place_interval.pnml",
        "/models/all_place_interval.xml",
        singleQueryExpect(Reachability::ResultPrinter::Satisfied)
    );
}

BOOST_AUTO_TEST_CASE(AllPlaceProduct, * utf::timeout(2)) {
    std::string model("/models/all_place_product.pnml");
    std::string query("/models/all_place_product.xml");
    std::set<size_t> qnums{0, 1, 2};
    std::vector<Reachability::ResultPrinter::Result> expected{
        Reachability::ResultPrinter::Satisfied,
        Reachability::ResultPrinter::NotSatisfied,
        Reachability::ResultPrinter::Satisfied
    };
    runReachabilityMatrixTest(
        model,
        query,
        multiQueryExpect(expected, qnums),
        TemporalLogic::CTL,
        qnums
    );
}

BOOST_AUTO_TEST_CASE(TokenRingAll, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/error-all-token-ring.pnml",
        "/models/error-all-token-ring.xml",
        singleQueryExpect(Reachability::ResultPrinter::Satisfied)
    );
}

BOOST_AUTO_TEST_CASE(TokenRingAll2, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/error-all-token-ring-2.pnml",
        "/models/error-all-token-ring-2.xml",
        singleQueryExpect(Reachability::ResultPrinter::NotSatisfied)
    );
}

BOOST_AUTO_TEST_CASE(SubtractionUnfolding, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/subtraction_unfolding.pnml",
        "/models/subtraction_unfolding.xml",
        singleQueryExpect(Reachability::ResultPrinter::Satisfied)
    );
}

BOOST_AUTO_TEST_CASE(UnfoldingAndOverapproxCpnSubtractionError, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/unfolding_and_overapprox_cpn_subtraction_error.pnml",
        "/models/unfolding_and_overapprox_cpn_subtraction_error.xml",
        singleQueryExpect(Reachability::ResultPrinter::Satisfied)
    );
}

BOOST_AUTO_TEST_CASE(SubtractionBug, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/subtraction_bug.pnml",
        "/models/subtraction_bug.xml",
        singleQueryExpect(Reachability::ResultPrinter::Satisfied)
    );
}

BOOST_AUTO_TEST_CASE(FixpointAndPartitioningError, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/fixpoint_and_partitioning_error.pnml",
        "/models/fixpoint_and_partitioning_error.xml",
        singleQueryExpect(Reachability::ResultPrinter::Satisfied)
    );
}

BOOST_AUTO_TEST_CASE(SubtractionErrorUnfolding, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/subtraction_error_unfolding.pnml",
        "/models/subtraction_error_unfolding.xml",
        singleQueryExpect(Reachability::ResultPrinter::Satisfied)
    );
}

BOOST_AUTO_TEST_CASE(SubtractionErrorUnfoldingNonEmptyMarking, * utf::timeout(2)) {
    runReachabilityMatrixTest(
        "/models/subtraction_error_unfolding_non_empty_marking.pnml",
        "/models/subtraction_error_unfolding_non_empty_marking.xml",
        singleQueryExpect(Reachability::ResultPrinter::Satisfied)
    );
}

