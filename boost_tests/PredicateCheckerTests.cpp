#include <boost/test/unit_test.hpp>
#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PredicateCheckers.h"

using namespace PetriEngine::PQL;

BOOST_AUTO_TEST_SUITE(IsReachabilityTests)

BOOST_AUTO_TEST_CASE(not_A_G_deadlock_true) {
    std::vector<std::string> _;
    auto cond = ParseQuery(R"(! A G (deadlock))", _);

    BOOST_TEST(isReachability(cond));
}

BOOST_AUTO_TEST_CASE(E_F_compare_true) {
    std::vector<std::string> _;
    auto cond = ParseQuery(R"(E F ("p1" < 4))", _);
    bool actual = isReachability(cond);

    BOOST_TEST(actual);
}

BOOST_AUTO_TEST_CASE(AG_deadlock_or_EF_deadlock_false) {
    std::vector<std::string> _;
    auto cond = ParseQuery(R"(A G (deadlock) || E F (deadlock))", _);
    bool actual = isReachability(cond);

    BOOST_TEST(!actual);
}

BOOST_AUTO_TEST_CASE(EF_AG_deadlock_or_deadlock_false) {
    std::vector<std::string> _;
    auto cond = ParseQuery(R"(E F (A G (deadlock) || (deadlock)) )", _);
    bool actual = isReachability(cond);

    BOOST_TEST(!actual);
}

BOOST_AUTO_TEST_CASE(deadlock_false) {
    std::vector<std::string> _;
    auto cond = ParseQuery(R"(deadlock)", _);
    bool actual = isReachability(cond);

    BOOST_TEST(!actual);
}


BOOST_AUTO_TEST_SUITE_END()