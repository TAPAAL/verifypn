#include <boost/test/unit_test.hpp>
#include <PetriEngine/PQL/Simplifier.h>
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PushNegation.h"

using namespace PetriEngine::PQL;

using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(push_negation)

BOOST_AUTO_TEST_CASE(not_less_than_literal_and_identifier) {
    auto identifier = std::make_shared<IdentifierExpr>("a");
    auto literal = std::make_shared<LiteralExpr>(1);
    auto condition = std::make_shared<NotCondition>(
            std::make_shared<LessThanCondition>(
                    identifier,
                    literal
            )
    );
    auto stats = negstat_t();


    auto res = pushNegation(condition, stats, EvaluationContext(), false, false, false);


    auto lte = std::dynamic_pointer_cast<LessThanOrEqualCondition>(res);
    BOOST_REQUIRE_MESSAGE(lte != nullptr, "Top level condition should be LessThanOrEqual");

    BOOST_REQUIRE_MESSAGE(lte->getExpr1() == literal && lte->getExpr2() == identifier,
                          "Less than operands should be swapped");
}

//BOOST_AUTO_TEST_CASE(AirplaneLD_PT_0050_3) {
//    auto cond = std::make_shared<NotCondition>(std::make_shared<NotCondition>(
//            std::make_shared<EGCondition>(
//                    std::make_shared<AFCondition>(
//                            std::make_shared<NotCondition>(
//                                    std::make_shared<ECondition>(
//                                            std::make_shared<UntilCondition>(
//                                                    std::make_shared<LessThanOrEqualCondition>(
//                                                            std::make_shared<PlusExpr>(std::vector<Expr_ptr>{
//                                                                    std::make_shared<IdentifierExpr>(
//                                                                            "P3"),
//                                                                    std::make_shared<IdentifierExpr>(
//                                                                            "stp5")
//                                                            }),
//                                                            std::make_shared<LiteralExpr>(12)
//                                                    ),
//                                                    std::make_shared<LessThanOrEqualCondition>(
//                                                            std::make_shared<PlusExpr>(std::vector<Expr_ptr>{
//                                                                    std::make_shared<IdentifierExpr>(
//                                                                            "SpeedPossibleVal_38"),
//                                                                    std::make_shared<IdentifierExpr>(
//                                                                            "SpeedPossibleVal_26")
//                                                            }),
//                                                            std::make_shared<LiteralExpr>(35)
//                                                    )
//                                            )
//                                    )
//                            )
//                    )
//            )
//    ));
//}

BOOST_AUTO_TEST_SUITE_END()