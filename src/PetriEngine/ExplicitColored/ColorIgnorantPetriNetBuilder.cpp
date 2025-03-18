#include "PetriEngine/ExplicitColored/ColorIgnorantPetriNetBuilder.h"

#include <VerifyPN.h>
#include <PetriEngine/ExplicitColored/ArcCompiler.h>

namespace PetriEngine::ExplicitColored {
    void ColorIgnorantPetriNetBuilder::addPlace(const std::string &name, const uint32_t tokens, const double x, const double y) {
        _builder.addPlace(name, tokens, x, y);
    }

    void ColorIgnorantPetriNetBuilder::addTransition(const std::string &name, const int32_t player, const double x, const double y) {
        _builder.addTransition(name, player, x, y);
    }

    void ColorIgnorantPetriNetBuilder::addInputArc(const std::string &place, const std::string &transition,
        const bool inhibitor, const uint32_t weight) {
        _builder.addInputArc(place, transition, inhibitor, weight);
    }

    void ColorIgnorantPetriNetBuilder::addOutputArc(const std::string &transition, const std::string &place,
        uint32_t weight) {
        _builder.addOutputArc(transition, place, weight);
    }

    void ColorIgnorantPetriNetBuilder::addPlace(const std::string &name, const Colored::ColorType *type,
        Colored::Multiset &&tokens, double x, double y) {
        uint32_t sum = 0;
        for (const auto& [_,count] : tokens) {
            sum += count;
        }
        _builder.addPlace(name, sum, x, y);
    }

    void ColorIgnorantPetriNetBuilder::addTransition(const std::string &name, const Colored::GuardExpression_ptr &guard,
        int32_t player, double x, double y) {
        _builder.addTransition(name, player, x, y);
    }

    void ColorIgnorantPetriNetBuilder::addInputArc(const std::string &place, const std::string &transition,
        const Colored::ArcExpression_ptr &expr, uint32_t inhib_weight) {
        if (inhib_weight > 0) {
            _builder.addInputArc(place, transition, true, inhib_weight);
        }
        ArcCompiler compiler(_variableMap, _colors);
        auto compiled = compiler.compile(expr);
        if (compiled->containsNegative()) {
            _foundNegative = true;
        }
        _builder.addInputArc(place, transition, false, compiled->getMinimalMarkingCount());

    }

    void ColorIgnorantPetriNetBuilder::addOutputArc(const std::string &transition, const std::string &place,
        const Colored::ArcExpression_ptr &expr) {
        ArcCompiler compiler(_variableMap, _colors);
        auto compiled = compiler.compile(expr);
        if (compiled->containsNegative()) {
            _foundNegative = true;
        }
        _builder.addOutputArc(transition, place, compiled->getMinimalMarkingCount());
    }

    void ColorIgnorantPetriNetBuilder::addColorType(const std::string &id, const Colored::ColorType *type) {
        _colors[id]= type;
    }

    void ColorIgnorantPetriNetBuilder::addVariable(const Colored::Variable *variable) {
        _variableMap[variable->name] = _nextVariable++;
    }

    void ColorIgnorantPetriNetBuilder::sort() {
        _builder.sort();
    }

    ColoredIgnorantPetriNetBuilderStatus ColorIgnorantPetriNetBuilder::build() {
        if (_foundNegative) {
            return ColoredIgnorantPetriNetBuilderStatus::CONTAINS_NEGATIVE;
        }
        return ColoredIgnorantPetriNetBuilderStatus::OK;
    }

    PetriNetBuilder ColorIgnorantPetriNetBuilder::getUnderlying() {
        return _builder;
    }
}
