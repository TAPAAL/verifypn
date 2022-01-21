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

#include "utils.h"

using namespace PetriEngine;
using namespace PetriEngine::Colored;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
}


BOOST_AUTO_TEST_CASE(InitialMarkingMismatch) {

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

BOOST_AUTO_TEST_CASE(InitialMarkingMatch) {

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