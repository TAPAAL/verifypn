/* Copyright (C) 2021 Peter G. Jensen <root@petergjoel.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_MODULE reduction

#include <boost/test/unit_test.hpp>
#include <string>
#include <fstream>
#include <sstream>

#include "LTL/LTLSearch.h"
#include "utils.h"
#include "CTL/CTLResult.h"
#include "CTL/CTLEngine.h"

using namespace PetriEngine;
using namespace PetriEngine::Colored;
namespace utf = boost::unit_test;

BOOST_AUTO_TEST_CASE(DirectoryTest) {
    BOOST_REQUIRE(getenv("TEST_FILES"));
}

BOOST_AUTO_TEST_CASE(ruleD, * utf::timeout(60)) {

    std::set<size_t> qnums{0};
    std::vector<Reachability::ResultPrinter::Result> expected{
        Reachability::ResultPrinter::Satisfied};
    std::vector<Reachability::ResultPrinter::Result> results{
        Reachability::ResultPrinter::Unknown};

    auto [conditions, builder, qstrings, trans_names, place_names] = load_builder("/models/rule_D.pnml",
        "/models/rule_D.xml", qnums);
    std::vector<uint32_t> reds;
    std::unique_ptr<PetriNet> net{builder.makePetriNet(false)};
    contextAnalysis(false, trans_names, place_names, builder, net.get(), conditions);
    builder.reduce(conditions, results, 1, false, net.get(), 10, reds);
    net.reset(builder.makePetriNet(false));
    contextAnalysis(false, trans_names, place_names, builder, net.get(), conditions);


    for (auto i : qnums) {
        std::vector<Reachability::ResultPrinter::Result> results{Reachability::ResultPrinter::Unknown};
        LTL::LTLSearch search(*net, conditions[i], LTL::BuchiOptimization::Low, LTL::APCompression::None);
        auto r = search.solve(false, 0, LTL::Algorithm::Tarjan,  LTL::LTLPartialOrder::None, Strategy::DFS, LTL::LTLHeuristic::DFS, true);
        auto result = r ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
        BOOST_REQUIRE_EQUAL(expected[i], result);
    }
}


BOOST_AUTO_TEST_CASE(ruleD2, * utf::timeout(60)) {

    std::set<size_t> qnums{15};
    std::vector<Reachability::ResultPrinter::Result> expected{
        Reachability::ResultPrinter::NotSatisfied};
    std::vector<Reachability::ResultPrinter::Result> results{
        Reachability::ResultPrinter::Unknown};

    auto [conditions, builder, qstrings, trans_names, place_names] = load_builder("/models/Referendum-PT-0015/model.pnml",
        "/models/Referendum-PT-0015/LTLCardinality.xml", qnums);
    std::vector<uint32_t> reds;
    std::unique_ptr<PetriNet> net{builder.makePetriNet(false)};
    contextAnalysis(false, trans_names, place_names, builder, net.get(), conditions);
    builder.reduce(conditions, results, 1, false, net.get(), 10, reds);
    net.reset(builder.makePetriNet(false));
    contextAnalysis(false, trans_names, place_names, builder, net.get(), conditions);


    for (size_t i = 0; i < conditions.size(); ++i) {
        std::vector<Reachability::ResultPrinter::Result> results{Reachability::ResultPrinter::Unknown};
        LTL::LTLSearch search(*net, conditions.at(i), LTL::BuchiOptimization::Low, LTL::APCompression::None);
        auto r = search.solve(false, 0, LTL::Algorithm::Tarjan,  LTL::LTLPartialOrder::None, Strategy::DFS, LTL::LTLHeuristic::DFS, true);
        auto result = r ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
        BOOST_REQUIRE_EQUAL(expected[i], result);
        ++i;
    }
}

BOOST_AUTO_TEST_CASE(ruleD3, * utf::timeout(60)) {

    const std::set<size_t> qnums{0};
    const std::vector<Reachability::ResultPrinter::Result> expected{
        Reachability::ResultPrinter::NotSatisfied};
    for(size_t rmode : {0,1})
    {
        std::vector<Reachability::ResultPrinter::Result> results{
            Reachability::ResultPrinter::Unknown};

        auto [conditions, builder, qstrings, trans_names, place_names] = load_builder("/models/DiscoveryGPU-PT-15a/model.pnml",
            "/models/DiscoveryGPU-PT-15a/ruleDerr.xml", qnums);
        std::vector<uint32_t> reds;
        std::unique_ptr<PetriNet> net{builder.makePetriNet(false)};
        contextAnalysis(false, trans_names, place_names, builder, net.get(), conditions);
        builder.reduce(conditions, results, rmode, false, net.get(), 10, reds);
        net.reset(builder.makePetriNet(false));
        contextAnalysis(false, trans_names, place_names, builder, net.get(), conditions);


        for (size_t i = 0; i < conditions.size(); ++i) {
            CTLResult cres(conditions[i].get());
            AsCTL v;
            Visitor::visit(v, conditions[i]);
            std::cerr << std::endl;
            auto p = PetriEngine::PQL::pushNegation(v._ctl_query);
            bool res = CTLSingleSolve(p.get(), net.get(), CTL::CZero, Strategy::DFS, false, cres);
            auto result = res ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
            BOOST_REQUIRE_EQUAL(expected[i], result);
            ++i;
        }
    }
}

BOOST_AUTO_TEST_CASE(ruleGFail, * utf::timeout(60)) {

    const std::set<size_t> qnums{0};
    const std::vector<Reachability::ResultPrinter::Result> expected{
        Reachability::ResultPrinter::NotSatisfied};
    for(size_t rmode : {0,1})
    {
        std::vector<Reachability::ResultPrinter::Result> results{
            Reachability::ResultPrinter::Unknown};

        auto [conditions, builder, qstrings, trans_names, place_names] = load_builder("/models/Kanban-PT-02000/model.pnml",
            "/models/Kanban-PT-02000/errG.xml", qnums);
        std::vector<uint32_t> reds;
        std::unique_ptr<PetriNet> net{builder.makePetriNet(false)};
        conditions[0]->toString(std::cerr);
        std::cerr << std::endl;
        contextAnalysis(false, trans_names, place_names, builder, net.get(), conditions);
        builder.reduce(conditions, results, rmode, false, net.get(), 10, reds);
        net.reset(builder.makePetriNet(false));
        contextAnalysis(false, trans_names, place_names, builder, net.get(), conditions);


        for (size_t i = 0; i < conditions.size(); ++i) {
            std::vector<Reachability::ResultPrinter::Result> results{Reachability::ResultPrinter::Unknown};
            LTL::LTLSearch search(*net, conditions.at(i), LTL::BuchiOptimization::Low, LTL::APCompression::None);
            auto r = search.solve(false, 0, LTL::Algorithm::Tarjan,  LTL::LTLPartialOrder::None, Strategy::DFS, LTL::LTLHeuristic::DFS, true);
            auto result = r ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
            BOOST_REQUIRE_EQUAL(expected[i], result);
            ++i;
        }
    }
}