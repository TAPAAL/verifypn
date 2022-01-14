#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE PQL tests
#include <boost/test/included/unit_test.hpp>
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/BinaryPrinter.h"

BOOST_AUTO_TEST_SUITE(BinaryPrinterTests)

using namespace PetriEngine::PQL;

//BOOST_AUTO_TEST_CASE(AirplaneLD_PT_0050_x3) {
//    auto identifier = std::make_shared<IdentifierExpr>("a");
//    auto literal = std::make_shared<LiteralExpr>(1);
//    auto condition = std::make_shared<NotCondition>(
//            std::make_shared<LessThanCondition>(
//                    identifier,
//                    literal
//            )
//    );
//
//    std::ostringstream oss;
//    BinaryPrinter bp(oss);
//
//
//}

BOOST_AUTO_TEST_SUITE_END()