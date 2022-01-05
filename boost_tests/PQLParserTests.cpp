#include <boost/test/unit_test.hpp>
#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Expressions.h"

using namespace PetriEngine::PQL;

BOOST_AUTO_TEST_SUITE(PQLParserTests)

BOOST_AUTO_TEST_CASE(fireable_condition) {
    std::string query = R"(is-fireable("t1","t2"))";
    std::vector<std::string> _;
    //auto expected = std::make_shared<AndCondition>(std::make_shared<FireableCondition>("t1"), std::make_shared<FireableCondition>("t2"));

    auto actual = ParseQuery(query, _);

    std::shared_ptr<AndCondition> andCondition;
    BOOST_REQUIRE(andCondition = std::dynamic_pointer_cast<AndCondition>(actual));

    std::shared_ptr<FireableCondition> fireable1;
    std::shared_ptr<FireableCondition> fireable2;
    BOOST_ASSERT(fireable1 = std::dynamic_pointer_cast<FireableCondition>((*andCondition)[0]));
    BOOST_ASSERT(fireable2 = std::dynamic_pointer_cast<FireableCondition>((*andCondition)[1]));

    BOOST_ASSERT(fireable1->getName() == "t1");
    BOOST_ASSERT(fireable2->getName() == "t2");
}

    BOOST_AUTO_TEST_CASE(question_mark_fireable) {
        std::string query = R"("t1"?)";
        std::vector<std::string> _;
        //auto expected = std::make_shared<AndCondition>(std::make_shared<FireableCondition>("t1"), std::make_shared<FireableCondition>("t2"));

        auto actual = ParseQuery(query, _);

        std::shared_ptr<FireableCondition> fireable;
        BOOST_ASSERT(fireable = std::dynamic_pointer_cast<FireableCondition>(actual));

        BOOST_ASSERT(fireable->getName() == "t1");
    }

BOOST_AUTO_TEST_CASE(E_deadlock) {
        std::string query = R"(E deadlock)";
        std::vector<std::string> _;
        //auto expected = std::make_shared<AndCondition>(std::make_shared<FireableCondition>("t1"), std::make_shared<FireableCondition>("t2"));

        auto actual = ParseQuery(query, _);

        std::shared_ptr<ECondition> andCondition;
        BOOST_REQUIRE(andCondition = std::dynamic_pointer_cast<ECondition>(actual));

        std::shared_ptr<DeadlockCondition> deadlockCondition;
        BOOST_ASSERT(deadlockCondition = std::dynamic_pointer_cast<DeadlockCondition>((*andCondition)[0]));
}

BOOST_AUTO_TEST_SUITE_END()