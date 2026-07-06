#define BOOST_TEST_MODULE push_negation

#include <boost/test/unit_test.hpp>
#include <PetriEngine/PQL/Simplifier.h>
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PushNegation.h"

using namespace PetriEngine::PQL;


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

// Regression test for wrong answer in bug #2156598: (P0 - P1) - P2 == 0 must not be rewritten to (P0 - P1) - P2 <= 0, since the subtraction can be negative.
BOOST_AUTO_TEST_CASE(equal_zero_with_subtraction_is_not_rewritten) {
    auto subtraction = std::make_shared<SubtractExpr>(std::vector<Expr_ptr>{
            std::make_shared<SubtractExpr>(std::vector<Expr_ptr>{
                    std::make_shared<IdentifierExpr>("P0"),
                    std::make_shared<IdentifierExpr>("P1")
            }),
            std::make_shared<IdentifierExpr>("P2")
    });
    auto condition = std::make_shared<EqualCondition>(
            subtraction,
            std::make_shared<LiteralExpr>(0)
    );
    auto stats = negstat_t();

    auto res = pushNegation(condition, stats, EvaluationContext(), false, false, false);

    BOOST_REQUIRE_MESSAGE(std::dynamic_pointer_cast<EqualCondition>(res) != nullptr,
                          "Equality with possibly negative expression should stay an EqualCondition");
}

BOOST_AUTO_TEST_CASE(not_equal_zero_with_subtraction_is_not_rewritten) {
    auto subtraction = std::make_shared<SubtractExpr>(std::vector<Expr_ptr>{
            std::make_shared<IdentifierExpr>("P0"),
            std::make_shared<IdentifierExpr>("P1")
    });
    auto condition = std::make_shared<NotCondition>(
            std::make_shared<EqualCondition>(
                    subtraction,
                    std::make_shared<LiteralExpr>(0)
            )
    );
    auto stats = negstat_t();

    auto res = pushNegation(condition, stats, EvaluationContext(), false, false, false);

    BOOST_REQUIRE_MESSAGE(std::dynamic_pointer_cast<NotEqualCondition>(res) != nullptr,
                          "Negated equality with possibly negative expression should become NotEqualCondition");
}

// Keep rewrite for non-negative expressions: P0 + P1 == 0  ->  P0 + P1 <= 0
BOOST_AUTO_TEST_CASE(equal_zero_with_plus_is_rewritten_to_leq) {
    auto sum = std::make_shared<PlusExpr>(std::vector<Expr_ptr>{
            std::make_shared<IdentifierExpr>("P0"),
            std::make_shared<IdentifierExpr>("P1")
    });
    auto condition = std::make_shared<EqualCondition>(
            sum,
            std::make_shared<LiteralExpr>(0)
    );
    auto stats = negstat_t();

    auto res = pushNegation(condition, stats, EvaluationContext(), false, false, false);

    BOOST_REQUIRE_MESSAGE(std::dynamic_pointer_cast<LessThanOrEqualCondition>(res) != nullptr,
                          "Equality with non-negative expression should be rewritten to LessThanOrEqual");
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
