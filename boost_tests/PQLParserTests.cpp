#define BOOST_TEST_MODULE PQLParserTests

#include <boost/test/unit_test.hpp>
#include <limits>
#include <sstream>
#include <string>
#include <cstdint>

#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriParse/QueryXMLParser.h"
#include "utils/errors.h"

using namespace PetriEngine::PQL;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
}

BOOST_AUTO_TEST_CASE(double_fireable_condition) {
    std::string query = R"(is-fireable("t1","t2"))";
    std::vector<std::string> _;

    auto actual = ParseQuery(query);

    std::shared_ptr<OrCondition> orCondition;
    BOOST_REQUIRE(orCondition = std::dynamic_pointer_cast<OrCondition>(actual));

    std::shared_ptr<FireableCondition> fireable1;
    std::shared_ptr<FireableCondition> fireable2;
    BOOST_TEST(fireable1 = std::dynamic_pointer_cast<FireableCondition>((*orCondition)[0]));
    BOOST_TEST(fireable2 = std::dynamic_pointer_cast<FireableCondition>((*orCondition)[1]));

    BOOST_TEST(*fireable1->getName() == "t1");
    BOOST_TEST(*fireable2->getName() == "t2");
}

BOOST_AUTO_TEST_CASE(or_condition) {
    std::string query = R"(true || false)";

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

BOOST_AUTO_TEST_CASE(fireable_condition) {
    std::string query = R"(is-fireable("t1"))";

    auto actual = ParseQuery(query);

    std::shared_ptr<FireableCondition> fireable;
    BOOST_TEST(fireable = std::dynamic_pointer_cast<FireableCondition>(actual));

    BOOST_TEST(*fireable->getName() == "t1");
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

        BOOST_TEST(*identifierExpr->name() == "p0");
        BOOST_TEST(literalExpr->value() == 4);
    }
}

BOOST_AUTO_TEST_CASE(A_not_F_deadlock_or_E_G_deadlock) {
    std::string query = R"(A !F (deadlock) || E G (deadlock) )";

    auto actual = ParseQuery(query);
    actual->toString(std::cerr);
    std::cerr << std::endl;

    // Left hand side of OR
    std::shared_ptr<OrCondition> orCondition;
    BOOST_TEST(orCondition = std::dynamic_pointer_cast<OrCondition>(actual));


    std::shared_ptr<ACondition> aCondition;
    BOOST_REQUIRE(aCondition = std::dynamic_pointer_cast<ACondition>((*orCondition)[0]));

    std::shared_ptr<NotCondition> notCondition;
    BOOST_REQUIRE(notCondition = std::dynamic_pointer_cast<NotCondition>((*aCondition)[0]));

    std::shared_ptr<FCondition> fCondition;
    BOOST_REQUIRE(fCondition = std::dynamic_pointer_cast<FCondition>((*notCondition)[0]));

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
BOOST_AUTO_TEST_CASE(control_condition) {
    auto query = R"(control: A G ("p0" <= 4))";

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

    BOOST_TEST(*identifierExpr->name() == "p0");
    BOOST_TEST(literalExpr->value() == 4);
}

BOOST_AUTO_TEST_CASE(large_query_and) {
    auto query = R"(control: AF ( true and (Inter0laneWtoEp0 = 16) and (Inter0laneEtoWp0 = 16) and (Inter0laneNtoSp0 = 16) and (Inter0laneStoNp0 = 16) and (Inter1laneWtoEp0 = 16) and (Inter1laneEtoWp0 = 16) and (Inter1laneNtoSp0 = 16) and (Inter1laneStoNp0 = 16) and (Inter2laneWtoEp0 = 16) and (Inter2laneEtoWp0 = 16) and (Inter2laneNtoSp0 = 16) and (Inter2laneStoNp0 = 16) and (Inter3laneWtoEp0 = 16) and (Inter3laneEtoWp0 = 16) and (Inter3laneNtoSp0 = 16) and (Inter3laneStoNp0 = 16) and (Inter4laneWtoEp0 = 16) and (Inter4laneEtoWp0 = 16) and (Inter4laneNtoSp0 = 16) and (Inter4laneStoNp0 = 16) and (Inter5laneWtoEp0 = 16) and (Inter5laneEtoWp0 = 16) and (Inter5laneNtoSp0 = 16) and (Inter5laneStoNp0 = 16) and (Inter6laneWtoEp0 = 16) and (Inter6laneEtoWp0 = 16) and (Inter6laneNtoSp0 = 16) and (Inter6laneStoNp0 = 16) ))";
    auto actual = ParseQuery(query);
    std::shared_ptr<ControlCondition> cCondition;
    BOOST_REQUIRE(cCondition = std::dynamic_pointer_cast<ControlCondition>(actual));

    std::shared_ptr<ACondition> aCondition;
    BOOST_REQUIRE(aCondition = std::dynamic_pointer_cast<ACondition>((*cCondition)[0]));

    std::shared_ptr<FCondition> fCondition;
    BOOST_REQUIRE(fCondition = std::dynamic_pointer_cast<FCondition>((*aCondition)[0]));
}

BOOST_AUTO_TEST_CASE(large_query_mix_and_or) {
    auto query = R"(control: AF ( true or (Inter0laneWtoEp0 = 16) or (Inter0laneEtoWp0 = 16) or (Inter0laneNtoSp0 = 16) or (Inter0laneStoNp0 = 16) or (Inter1laneWtoEp0 = 16) or (Inter1laneEtoWp0 = 16) or (Inter1laneNtoSp0 = 16) or (Inter1laneStoNp0 = 16) and (Inter2laneWtoEp0 = 16) and (Inter2laneEtoWp0 = 16) and (Inter2laneNtoSp0 = 16) and (Inter2laneStoNp0 = 16) and (Inter3laneWtoEp0 = 16) and (Inter3laneEtoWp0 = 16) and (Inter3laneNtoSp0 = 16) and (Inter3laneStoNp0 = 16) and (Inter4laneWtoEp0 = 16) and (Inter4laneEtoWp0 = 16) and (Inter4laneNtoSp0 = 16) and (Inter4laneStoNp0 = 16) and (Inter5laneWtoEp0 = 16) and (Inter5laneEtoWp0 = 16) and (Inter5laneNtoSp0 = 16) and (Inter5laneStoNp0 = 16) and (Inter6laneWtoEp0 = 16) and (Inter6laneEtoWp0 = 16) and (Inter6laneNtoSp0 = 16) and (Inter6laneStoNp0 = 16) ))";
    auto actual = ParseQuery(query);
    std::shared_ptr<ControlCondition> cCondition;
    BOOST_REQUIRE(cCondition = std::dynamic_pointer_cast<ControlCondition>(actual));

    std::shared_ptr<ACondition> aCondition;
    BOOST_REQUIRE(aCondition = std::dynamic_pointer_cast<ACondition>((*cCondition)[0]));

    std::shared_ptr<FCondition> fCondition;
    BOOST_REQUIRE(fCondition = std::dynamic_pointer_cast<FCondition>((*aCondition)[0]));
}

namespace {
std::string xmlQueryWithConstant(const std::string& value) {
    return std::string(R"(<property-set><property><id>q</id><formula>
<integer-le>
<integer-constant>)") + value + R"(</integer-constant>
<integer-constant>0</integer-constant>
</integer-le>
</formula></property></property-set>)";
}

void parseXmlQuery(const std::string& xml) {
    shared_string_set strings;
    QueryXMLParser parser(strings);
    std::stringstream ss(xml);
    BOOST_REQUIRE(parser.parse(ss, {0}));
}
}

BOOST_AUTO_TEST_CASE(PqlIntegerConstantOverflowIsParseError) {
    const auto maxInt = std::to_string(std::numeric_limits<int32_t>::max());
    const auto overflowInt = std::to_string(static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 1);

    BOOST_CHECK_NO_THROW(ParseQuery("\"p0\" <= " + maxInt));
    auto parsed = ParseQuery("\"p0\" <= " + maxInt);
    auto compare = std::dynamic_pointer_cast<LessThanOrEqualCondition>(parsed);
    BOOST_REQUIRE(compare);
    auto literal = std::dynamic_pointer_cast<LiteralExpr>((*compare)[1]);
    BOOST_REQUIRE(literal);
    BOOST_CHECK_EQUAL(literal->value(), std::numeric_limits<int32_t>::max());

    BOOST_CHECK_THROW(ParseQuery("\"p0\" <= " + overflowInt), base_error);
    BOOST_CHECK_THROW(ParseQuery("\"p0\" <= 5000000000"), base_error);
    BOOST_CHECK_THROW(ParseQuery("EF \"p0\" > 4294967296"), base_error);
}

BOOST_AUTO_TEST_CASE(XmlIntegerConstantOverflowIsParseError) {
    const auto maxInt = std::to_string(std::numeric_limits<int32_t>::max());
    const auto overflowInt = std::to_string(static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 1);
    const auto minInt = std::to_string(std::numeric_limits<int32_t>::min());

    BOOST_CHECK_NO_THROW(parseXmlQuery(xmlQueryWithConstant(maxInt)));
    BOOST_CHECK_NO_THROW(parseXmlQuery(xmlQueryWithConstant(minInt)));

    BOOST_CHECK_THROW(parseXmlQuery(xmlQueryWithConstant(overflowInt)), base_error);
    BOOST_CHECK_THROW(parseXmlQuery(xmlQueryWithConstant("5000000000")), base_error);
    BOOST_CHECK_THROW(parseXmlQuery(xmlQueryWithConstant("4294967296")), base_error);
    BOOST_CHECK_THROW(parseXmlQuery(xmlQueryWithConstant("-2147483649")), base_error);
}
