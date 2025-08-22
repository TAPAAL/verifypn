#define BOOST_TEST_MODULE explicit_engine

#include <boost/test/unit_test.hpp>
#include <string>
#include <vector>
#include <set>

#include "utils.h"
#include "PetriEngine/ExplicitColored/ExplicitColoredModelChecker.h"

using namespace PetriEngine;
using namespace PetriEngine::ExplicitColored;
namespace utf = boost::unit_test;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
}

void test_subtraction_with_vars(const char* fn, ExplicitColoredModelChecker::Result expected, size_t quid = 0) { 
    std::string model = std::string("/models/explicit-engine/") + fn + ".pnml";
    std::string query = std::string("/models/explicit-engine/") + fn + ".xml";
    std::set<size_t> qnums{quid};
    auto [queries, querynames, sset, options] = load_explicit(model, query, qnums);
    options.kbound = 4;

    ExplicitColoredModelChecker checker(sset, std::cout);
    
    auto result = checker.checkQuery(queries[0], options);
    
    BOOST_REQUIRE_EQUAL(expected, result);
}

BOOST_AUTO_TEST_CASE(SubtractionWithVars, * utf::timeout(5)) {
    test_subtraction_with_vars("subtraction_with_vars", ExplicitColoredModelChecker::Result::SATISFIED);
}
