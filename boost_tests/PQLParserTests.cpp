#define BOOST_TEST_MODULE PQLParserTests

#include <boost/test/unit_test.hpp>
#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Expressions.h"

using namespace PetriEngine::PQL;

BOOST_AUTO_TEST_CASE(fireable_condition) {
    std::string query = R"(is-fireable("t1","t2"))";
    std::vector<std::string> _;

    auto actual = ParseQuery(query);

    std::shared_ptr<OrCondition> orCondition;
    BOOST_REQUIRE(orCondition = std::dynamic_pointer_cast<OrCondition>(actual));

    std::shared_ptr<FireableCondition> fireable1;
    std::shared_ptr<FireableCondition> fireable2;
    BOOST_TEST(fireable1 = std::dynamic_pointer_cast<FireableCondition>((*orCondition)[0]));
    BOOST_TEST(fireable2 = std::dynamic_pointer_cast<FireableCondition>((*orCondition)[1]));

    BOOST_TEST(fireable1->getName() == "t1");
    BOOST_TEST(fireable2->getName() == "t2");
}

BOOST_AUTO_TEST_CASE(or_condition) {
    std::string query = R"(true || false)";
    std::vector<std::string> _;

    auto actual = ParseQuery(query);

    std::shared_ptr<OrCondition> orCondition;
    BOOST_REQUIRE(orCondition = std::dynamic_pointer_cast<OrCondition>(actual));
    std::shared_ptr<BooleanCondition> b1;
    std::shared_ptr<BooleanCondition> b2;
    BOOST_TEST(b1 = std::dynamic_pointer_cast<BooleanCondition>((*orCondition)[0]));
    BOOST_TEST(b2 = std::dynamic_pointer_cast<BooleanCondition>((*orCondition)[1]));
    BOOST_REQUIRE(b1->value);
    BOOST_REQUIRE(!b2->value);
}

BOOST_AUTO_TEST_CASE(question_mark_fireable) {
    std::string query = R"("t1"?)";
    std::vector<std::string> _;
    //auto expected = std::make_shared<AndCondition>(std::make_shared<FireableCondition>("t1"), std::make_shared<FireableCondition>("t2"));

    auto actual = ParseQuery(query);

    std::shared_ptr<FireableCondition> fireable;
    BOOST_TEST(fireable = std::dynamic_pointer_cast<FireableCondition>(actual));

    BOOST_TEST(fireable->getName() == "t1");
}

BOOST_AUTO_TEST_CASE(E_F_deadlock) {
    for(auto query : {R"(EF (deadlock))", R"(E F (deadlock))"})
    {
        std::vector<std::string> _;
        //auto expected = std::make_shared<AndCondition>(std::make_shared<FireableCondition>("t1"), std::make_shared<FireableCondition>("t2"));

        auto actual = ParseQuery(query);

        std::shared_ptr<ECondition> eCondition;
        BOOST_REQUIRE(eCondition = std::dynamic_pointer_cast<ECondition>(actual));

        std::shared_ptr<FCondition> fCondition;
        BOOST_REQUIRE(fCondition = std::dynamic_pointer_cast<FCondition>((*eCondition)[0]));

        std::shared_ptr<DeadlockCondition> deadlockCondition;
        BOOST_TEST(deadlockCondition = std::dynamic_pointer_cast<DeadlockCondition>((*fCondition)[0]));
    }
}

BOOST_AUTO_TEST_CASE(A_G_compare) {
    for(auto query : {R"(A G ("p0" <= 4))", R"(AG ("p0" <= 4))"})
    {
        std::vector<std::string> _;
        //auto expected = std::make_shared<AndCondition>(std::make_shared<FireableCondition>("t1"), std::make_shared<FireableCondition>("t2"));

        auto actual = ParseQuery(query);

        std::shared_ptr<ACondition> aCondition;
        BOOST_REQUIRE(aCondition = std::dynamic_pointer_cast<ACondition>(actual));

        std::shared_ptr<GCondition> gCondition;
        BOOST_REQUIRE(gCondition = std::dynamic_pointer_cast<GCondition>((*aCondition)[0]));

        std::shared_ptr<LessThanOrEqualCondition> leqCondition;
        BOOST_REQUIRE(leqCondition = std::dynamic_pointer_cast<LessThanOrEqualCondition>((*gCondition)[0]));

        std::shared_ptr<IdentifierExpr> identifierExpr;
        std::shared_ptr<LiteralExpr> literalExpr;
        BOOST_REQUIRE(identifierExpr = std::dynamic_pointer_cast<IdentifierExpr>((*leqCondition)[0]));
        BOOST_REQUIRE(literalExpr = std::dynamic_pointer_cast<LiteralExpr>((*leqCondition)[1]));

        BOOST_TEST(identifierExpr->name() == "p0");
        BOOST_TEST(literalExpr->value() == 4);
    }
}

BOOST_AUTO_TEST_CASE(A_not_F_deadlock_or_E_G_deadlock) {
    std::string query = R"(A !F (deadlock) || E G (deadlock) )";
    std::vector<std::string> _;

    auto actual = ParseQuery(query);

    // Left hand side of OR
    std::shared_ptr<ACondition> aCondition;
    BOOST_REQUIRE(aCondition = std::dynamic_pointer_cast<ACondition>(actual));

    std::shared_ptr<NotCondition> notCondition;
    BOOST_REQUIRE(notCondition = std::dynamic_pointer_cast<NotCondition>((*aCondition)[0]));

    std::shared_ptr<OrCondition> orCondition;
    BOOST_TEST(orCondition = std::dynamic_pointer_cast<OrCondition>((*notCondition)[0]));

    std::shared_ptr<FCondition> fCondition;
    BOOST_REQUIRE(fCondition = std::dynamic_pointer_cast<FCondition>((*orCondition)[0]));

    std::shared_ptr<DeadlockCondition> deadlockCondition;
    BOOST_TEST(deadlockCondition = std::dynamic_pointer_cast<DeadlockCondition>((*fCondition)[0]));

    // Right hand side of OR
    std::shared_ptr<ECondition> eCondition;
    BOOST_REQUIRE(eCondition = std::dynamic_pointer_cast<ECondition>((*orCondition)[1]));

    std::shared_ptr<GCondition> gCondition;
    BOOST_REQUIRE(gCondition = std::dynamic_pointer_cast<GCondition>((*eCondition)[0]));

    std::shared_ptr<DeadlockCondition> deadlockCondition2;
    BOOST_TEST(deadlockCondition2 = std::dynamic_pointer_cast<DeadlockCondition>((*gCondition)[0]));
}

BOOST_AUTO_TEST_CASE(control_AG) {
    auto query = R"(control: A G ("p0" <= 4))";
    std::vector<std::string> _;

    auto actual = ParseQuery(query);

    std::shared_ptr<ControlCondition> cCondition;
    BOOST_REQUIRE(cCondition = std::dynamic_pointer_cast<ControlCondition>(actual));

    std::shared_ptr<ACondition> aCondition;
    BOOST_REQUIRE(aCondition = std::dynamic_pointer_cast<ACondition>((*cCondition)[0]));

    std::shared_ptr<GCondition> gCondition;
    BOOST_REQUIRE(gCondition = std::dynamic_pointer_cast<GCondition>((*aCondition)[0]));

    std::shared_ptr<LessThanOrEqualCondition> leqCondition;
    BOOST_REQUIRE(leqCondition = std::dynamic_pointer_cast<LessThanOrEqualCondition>((*gCondition)[0]));

    std::shared_ptr<IdentifierExpr> identifierExpr;
    std::shared_ptr<LiteralExpr> literalExpr;
    BOOST_REQUIRE(identifierExpr = std::dynamic_pointer_cast<IdentifierExpr>((*leqCondition)[0]));
    BOOST_REQUIRE(literalExpr = std::dynamic_pointer_cast<LiteralExpr>((*leqCondition)[1]));

    BOOST_TEST(identifierExpr->name() == "p0");
    BOOST_TEST(literalExpr->value() == 4);
}
