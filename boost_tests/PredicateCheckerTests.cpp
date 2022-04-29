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

#define BOOST_TEST_MODULE PredicateCheckerTests

#include <boost/test/unit_test.hpp>

#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PredicateCheckers.h"

#include <string>
#include <fstream>
#include <sstream>

#include "utils.h"

using namespace PetriEngine;
namespace utf = boost::unit_test;

using namespace PetriEngine::PQL;

BOOST_AUTO_TEST_CASE(not_A_G_deadlock_true) {
    auto cond = ParseQuery(R"(! A G (deadlock))");

    BOOST_TEST(isReachability(cond));
}

BOOST_AUTO_TEST_CASE(E_F_compare_true) {
    auto cond = ParseQuery(R"(E F ("p1" < 4))");
    bool actual = isReachability(cond);

    BOOST_TEST(actual);
}

BOOST_AUTO_TEST_CASE(AG_deadlock_or_EF_deadlock_false) {
    auto cond = ParseQuery(R"(A G (deadlock) || E F (deadlock))");
    bool actual = isReachability(cond);

    BOOST_TEST(!actual);
}

BOOST_AUTO_TEST_CASE(EF_AG_deadlock_or_deadlock_false) {
    auto cond = ParseQuery(R"(E F (A G (deadlock) || (deadlock)) )");
    bool actual = isReachability(cond);

    BOOST_TEST(!actual);
}

BOOST_AUTO_TEST_CASE(deadlock_false) {
    auto cond = ParseQuery(R"(deadlock)");
    bool actual = isReachability(cond);

    BOOST_TEST(!actual);
}
