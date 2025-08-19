/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   utils.h
 * Author: pgj
 *
 * Created on 7 January 2022, 18.20
 */



#ifndef UTILS_H
#define UTILS_H

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "VerifyPN.h"
#include "PetriEngine/ExplicitColored/ColoredResultPrinter.h"

using namespace PetriEngine;
using namespace PetriEngine::Colored;
using namespace PetriEngine::ExplicitColored;

std::ifstream loadFile(const char* file) {
    std::stringstream ss;
    ss << getenv("TEST_FILES") << file;
    auto in = std::ifstream(ss.str());
    assert(in);
    return in;
}

class ResultHandler : public Reachability::AbstractHandler {

    virtual std::pair<Result, bool> handle(
        size_t index,
        PQL::Condition* query,
        Result result,
        const std::vector<uint32_t>* maxPlaceBound = nullptr,
        size_t expandedStates = 0,
        size_t exploredStates = 0,
        size_t discoveredStates = 0,
        int maxTokens = 0,
        Structures::StateSetInterface* stateset = nullptr, size_t lastmarking = 0, const MarkVal* initialMarking = nullptr, bool = true) {
        if (result == Unknown) return std::make_pair(Unknown, false);
        auto retval = Satisfied;
        if (result == Satisfied)
            retval = query->isInvariant() ? NotSatisfied : Satisfied;
        else if (result == NotSatisfied)
            retval = query->isInvariant() ? Satisfied : NotSatisfied;
        return std::make_pair(retval, false);
    }
};

auto load_builder(std::string model, std::string queries, const std::set<size_t>& qnums,
        bool partition = false, bool symmetry = false, bool cfp = false, bool over_approx = false,
        int32_t partitionTimeout = 10, int32_t max_intervals = 100, int32_t intervals_reduced = 10, int32_t interval_timeout = 10) {
    shared_string_set sset;
    ColoredPetriNetBuilder cpnBuilder(sset);
    auto f = loadFile(model.c_str());
    cpnBuilder.parse_model(f);
    auto [builder, trans_names, place_names] = unfold(cpnBuilder, partition, symmetry, cfp, std::cerr, partitionTimeout, max_intervals, intervals_reduced, interval_timeout, over_approx);
    builder.sort();
    auto q = loadFile(queries.c_str());
    std::vector<std::string> qstrings;
    auto conditions = parseXMLQueries(sset, qstrings, q, qnums, false);
    std::unique_ptr<PetriNet> pn{builder.makePetriNet()};
    contextAnalysis(cpnBuilder.isColored() && !over_approx, trans_names, place_names, builder, pn.get(), conditions);
    return std::make_tuple(std::move(conditions), std::move(builder), std::move(qstrings), std::move(trans_names), std::move(place_names));
}

auto load_pn(std::string model, std::string queries, const std::set<size_t>& qnums, TemporalLogic logic = TemporalLogic::CTL,
        bool reduce = false, bool partition = false, bool symmetry = false, bool cfp = false, bool over_approx = false,
        uint32_t reduceTimeout = 5, int32_t partitionTimeout = 10, int32_t max_intervals = 100,
        int32_t intervals_reduced = 10, int32_t interval_timeout = 10)
{

    shared_string_set sset;
    ColoredPetriNetBuilder cpnBuilder(sset);
    auto f = loadFile(model.c_str());
    cpnBuilder.parse_model(f);
    auto q = loadFile(queries.c_str());
    std::vector<std::string> qstrings;
    auto conditions = parseXMLQueries(sset, qstrings, q, qnums, false);
    std::vector<uint32_t> reductions {};
    reduceColored(cpnBuilder, conditions, logic, reduceTimeout, std::cerr, reduce ? 1 : 0, reductions);
    auto [builder, trans_names, place_names] = unfold(cpnBuilder, partition, symmetry, cfp, std::cerr, partitionTimeout, max_intervals, intervals_reduced, interval_timeout, over_approx);
    builder.sort();
    std::unique_ptr<PetriNet> pn{builder.makePetriNet()};
    contextAnalysis(cpnBuilder.isColored() && !over_approx, trans_names, place_names, builder, pn.get(), conditions);
    return std::make_tuple(std::move(pn), std::move(conditions), std::move(qstrings));
}

auto load_explicit(std::string model, std::string queries, const std::set<size_t>& qnums) {
    shared_string_set sset;
    ColoredPetriNetBuilder cpnBuilder(sset);
    auto f = loadFile(model.c_str());
    cpnBuilder.parse_model(f);
    auto q = loadFile(queries.c_str());
    std::vector<std::string> qstrings;
    auto conditions = parseXMLQueries(sset, qstrings, q, qnums, false);

    options_t options;
    options.modelfile = model.data();
    options.isCPN = true;
    options.explicit_colored = true;
    options.doUnfolding = false;
    options.enablereduction = 0;
    options.enablecolreduction = 0;
    options.queryReductionTimeout = 0;
    options.colReductionTimeout = 0;
    options.stubbornreduction = false;

    return std::make_tuple(std::move(conditions), std::move(qstrings), std::move(cpnBuilder), std::move(sset), std::move(options));
}

class IntialMarkingCollector : public AbstractPetriNetBuilder {
public:
    Multiset getInitialMarking(std::string placeName) {
        auto it = _initialMarkings.find(placeName);
        if (it == _initialMarkings.end()) {
            return Multiset();
        }
        return it->second;
    }

    void addPlace(const std::string& name, uint32_t tokens, double x, double y) override {}
    void addPlace(const std::string& name, const Colored::ColorType* type, Colored::Multiset&& tokens, double x, double y) override {
        _initialMarkings.emplace(name, std::move(tokens));
    }
    void addTransition(const std::string& name, int32_t player, double x, double y) override {}
    void addTransition(const std::string& name, const Colored::GuardExpression_ptr& guard, int32_t player, double x, double y) override {}
    void addInputArc(const std::string& place, const std::string& transition, bool inhibitor, uint32_t weight) override {}
    void addInputArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr &expr, uint32_t inhib_weight) override {}
    void addOutputArc(const std::string& transition, const std::string& place, uint32_t weight) override {}
    void addOutputArc(const std::string& transition, const std::string& place, const Colored::ArcExpression_ptr& expr) override {}
    void addColorType(const std::string& id, const Colored::ColorType* type) override {}
    void sort() override {}
private:
    std::map<std::string, Colored::Multiset> _initialMarkings;
};


#endif /* UTILS_H */

