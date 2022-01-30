#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PredicateCheckers.h"

#define BOOST_TEST_MODULE PredicateCheckerTests
#include <boost/test/included/unit_test.hpp>

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
