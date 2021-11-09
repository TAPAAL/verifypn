#include <boost/test/unit_test.hpp>
#include <PetriEngine/PQL/XMLPrinter.h>

#include "PetriEngine/PQL/Expressions.h"

using namespace PetriEngine::PQL;
using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(XML_PRINTER_TESTS)

BOOST_AUTO_TEST_CASE(LESSTHAN_LITERAL_LITERAL) {
    auto condition = std::make_shared<NotCondition>(
            std::make_shared<LessThanCondition>(
                    std::make_shared<LiteralExpr>(1),
                    std::make_shared<LiteralExpr>(2)
            )
    );

    std::ostringstream os;
    XMLPrinter xmlPrinter(os, 0, 2);

    condition->visit(xmlPrinter);

    auto expected = "<negation>\n"
                    "  <integer-lt>\n"
                    "    <integer-constant>1</integer-constant>\n"
                    "    <integer-constant>2</integer-constant>\n"
                    "  </integer-lt>\n"
                    "</negation>\n";

    BOOST_REQUIRE(strcmp(os.str().c_str(), expected) == 0);
}

BOOST_AUTO_TEST_SUITE_END()