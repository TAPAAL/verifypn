#include "PetriEngine/ExplicitColored/ColoredPetriNetBuilder.h"
#include <PetriEngine/ExplicitColored/ArcCompiler.h>
#include "PetriEngine/ExplicitColored/GuardCompiler.h"

namespace PetriEngine::ExplicitColored {
    ColoredPetriNetBuilder::ColoredPetriNetBuilder() {
        _dotColorType = std::make_shared<ColorType>(1);
        _colors = std::make_shared<Colored::ColorTypeMap>();
        _variableMap = std::make_shared<std::unordered_map<std::string, Variable_t>>();
    }

    void ColoredPetriNetBuilder::addPlace(const std::string& name, const uint32_t tokens, double, double)
    {
        _currentNet._places.push_back({
            _dotColorType
        });

        _placeIndices[name] = _currentNet._places.size() - 1;

        auto multiSet = CPNMultiSet();
        multiSet.setCount(ColorSequence{DOT_COLOR}, tokens);

        _currentNet._initialMarking.markings.push_back(multiSet);
    }

    void ColoredPetriNetBuilder::addTransition(const std::string& name, int32_t, double, double) {
        _currentNet._transitions.push_back({
            nullptr
        });
        _transitionIndices[name] = _currentNet._transitions.size() - 1;
    }

    //Dot color input arc
     void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, const bool inhibitor, uint32_t weight) {
         auto from = _placeIndices.find(place)->second;
         auto to = _transitionIndices.find(transition)->second;
         if (inhibitor){
             _currentNet._inhibitorArcs.emplace_back(from, to, weight);
         } else {
            const auto expr = std::make_shared<Colored::NumberOfExpression>(
                std::vector<Colored::ColorExpression_ptr> {
                    std::make_shared<Colored::DotConstantExpression>()
                },
                weight
            );
            const ArcCompiler arcCompiler(*_variableMap, *_colors);
             _inputArcs.push_back(ColoredPetriNetArc {
                 from,
                 to,
                 _currentNet._places[from].colorType,
                 arcCompiler.compile(expr)
             });
         }
    }

    //Colored input arc
    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr, uint32_t inhib_weight) {
        auto from = _placeIndices.find(place)->second;
        auto to = _transitionIndices.find(transition)->second;
        if (inhib_weight != 0) {
            _currentNet._inhibitorArcs.emplace_back(from, to, inhib_weight);
        } else {
            const ArcCompiler arcCompiler(*_variableMap, *_colors);
            auto arc = arcCompiler.compile(expr);

            _inputArcs.push_back(ColoredPetriNetArc {
                from,
                to,
                _currentNet._places[from].colorType,
                std::move(arc)
            });
        }

    }

    //Non colored output arc
    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, uint32_t weight) {
        const auto placeIndex = _placeIndices.find(place)->second;

        const auto expr = std::make_shared<Colored::NumberOfExpression>(
            std::vector<Colored::ColorExpression_ptr>{
                std::make_shared<Colored::DotConstantExpression>()
            },
            weight
        );

        const ArcCompiler arcCompiler(*_variableMap, *_colors);

        _outputArcs.push_back(ColoredPetriNetArc {
            _transitionIndices.find(transition)->second,
            placeIndex,
            _currentNet._places[placeIndex].colorType,
            arcCompiler.compile(expr)
        });
    }

    //Colored output arc
    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, const Colored::ArcExpression_ptr& expr) {
        const auto placeIndex = _placeIndices.find(place)->second;

        const ArcCompiler arcCompiler(*_variableMap, *_colors);

        auto arc = arcCompiler.compile(expr);

        _outputArcs.push_back(ColoredPetriNetArc {
            _transitionIndices.find(transition)->second,
            placeIndex,
            _currentNet._places[placeIndex].colorType,
            std::move(arc)
        });
    }

    void ColoredPetriNetBuilder::addPlace(const std::string& name, const Colored::ColorType* type, Colored::Multiset&& tokens, double, double) {
        ColoredPetriNetPlace place;
        CPNMultiSet multiSet;
        place.colorType = _colorTypeMap.find(type->getName())->second;
        for (const auto& [tokenColor, count] : tokens) {
            std::vector<Color_t> colorSequence;
            if (tokenColor->isTuple()) {
                for (const auto& color : tokenColor->getTupleColors()) {
                    colorSequence.push_back(color->getId());
                }
            } else {
                colorSequence.push_back(tokenColor->getId());
            }
            multiSet.setCount(ColorSequence{colorSequence, *place.colorType}, count);
        }

        _currentNet._places.push_back(std::move(place));
        _currentNet._initialMarking.markings.push_back(multiSet);

        _placeIndices[name] = _currentNet._places.size() - 1;
    }

    void ColoredPetriNetBuilder::addTransition(const std::string& name, const Colored::GuardExpression_ptr& guard, int32_t, double, double) {
        ColoredPetriNetTransition transition;
        const GuardCompiler compiler(*_variableMap, *_colors);
        if (guard != nullptr) {
            transition.guardExpression = compiler.compile(*guard);
        } else {
            transition.guardExpression = nullptr;
        }
        _currentNet._transitions.emplace_back(std::move(transition));
        _transitionIndices.emplace(name, _currentNet._transitions.size() - 1);
    }

    void ColoredPetriNetBuilder::addColorType(const std::string& id, const Colored::ColorType* type) {
        if (const auto productType = dynamic_cast<const Colored::ProductType*>(type)) {
            std::vector<uint32_t> basicColorSizes;
            uint32_t totalSize = 1;
            for (const auto& colorSize : productType->getConstituentsSizes()) {
                basicColorSizes.push_back(colorSize);
                totalSize *= colorSize;
            }
            auto productColorType = std::make_shared<ColorType>(totalSize, std::move(basicColorSizes));
            _colorTypeMap.emplace(id, std::move(productColorType));
        } else {
            auto colorType = std::make_shared<ColorType>(type->size());
            _colorTypeMap[id] = std::move(colorType);
        }
        (*_colors)[id] = type;
    }

    void ColoredPetriNetBuilder::addToColorType(Colored::ProductType* colorType, const Colored::ColorType* newConstituent) {
        auto& productType = _colorTypeMap[colorType->getName()];
        const auto colorSize = newConstituent->getConstituentsSizes()[0];
        productType->colorSize *= colorSize;
        productType->basicColorSizes.push_back(colorSize);
        //Not related to building our net but the PNML writer needs the pointer updated
        colorType->addType(newConstituent);
    }

    void ColoredPetriNetBuilder::addVariable(const Colored::Variable* variable) {
        (*_variableMap)[variable->name] = _variableMap->size();
        Variable var{};
        var.colorType = _colorTypeMap.find(variable->colorType->getName())->second->colorSize;
        _currentNet._variables.emplace_back(var);
    }

    ColoredPetriNetBuilderStatus ColoredPetriNetBuilder::build() {
        _createArcsAndTransitions();

        const auto status = _calculateTransitionVariables();
        if (status != ColoredPetriNetBuilderStatus::OK) {
            return status;
        }

        _calculatePrePlaceConstraints();

        return ColoredPetriNetBuilderStatus::OK;
    }

    ColoredPetriNet ColoredPetriNetBuilder::takeNet() {
        return std::move(_currentNet);
    }

    void ColoredPetriNetBuilder::_createArcsAndTransitions() {
        const auto transitions = _currentNet._transitions.size();
        auto transIndices = std::vector<std::pair<uint32_t,uint32_t>>(transitions + 1);
        auto arcs = std::vector<ColoredPetriNetArc>{};
        auto inhibIndices = std::vector<uint32_t>(transitions + 1, _currentNet._inhibitorArcs.size());

        std::sort(_currentNet._inhibitorArcs.begin(), _currentNet._inhibitorArcs.end(),
                  [](const ColoredPetriNetInhibitor a, const ColoredPetriNetInhibitor b){
            return a.to < b.to;
        });
        auto arcIndex = 0;
        for (size_t i = 0; i < transitions; i++){
            auto inputIndex= arcIndex;
            for (auto& a : _inputArcs){
                if (a.to == i){
                    arcs.push_back(std::move(a));
                    arcIndex++;
                }
            }
            transIndices[i] = std::pair{inputIndex, arcIndex};
            for (auto& a : _outputArcs){
                if (a.from == i){
                    arcs.push_back(std::move(a));
                    arcIndex++;
                }
            }
        }
        size_t transIndex = 0;
        size_t inhibIndex = 0;
        while (transIndex < transitions && inhibIndex < _currentNet._inhibitorArcs.size()){
            const auto to = _currentNet._inhibitorArcs[inhibIndex].to;
            while (transIndex <= to){
                inhibIndices[transIndex] = inhibIndex;
                transIndex++;
            }
            inhibIndex++;
        }

        transIndices.back() = std::pair(arcIndex,arcIndex);
        _currentNet._transitionInhibitors = std::move(inhibIndices);
        _currentNet._arcs = std::move(arcs);
        _currentNet._transitionArcs = std::move(transIndices);
    }

    ColoredPetriNetBuilderStatus ColoredPetriNetBuilder::_calculateTransitionVariables() {
        for (auto tid = 0; tid < _currentNet._transitions.size(); tid++) {
            std::set<Variable_t> relevantVariables;
            _currentNet.extractGuardVariables(tid, relevantVariables);
            _currentNet.extractInputVariables(tid, relevantVariables);
            _currentNet.extractOutputVariables(tid, relevantVariables);

            Binding_t totalBindings = 0;
            bool first = true;
            for (auto variableIndex : relevantVariables) {
                auto varColorCount = _currentNet._variables[variableIndex].colorType;

                if (first) {
                    totalBindings = 1;
                    first = false;
                }
                if (totalBindings > std::numeric_limits<Binding_t>::max() / varColorCount) {
                    return ColoredPetriNetBuilderStatus::TOO_MANY_BINDINGS;
                }
                totalBindings *= varColorCount;
            }
            _currentNet._transitions[tid].variables = std::move(relevantVariables);
            _currentNet._transitions[tid].totalBindings = std::move(totalBindings);
        }
        return ColoredPetriNetBuilderStatus::OK;
    }

    void ColoredPetriNetBuilder::_calculatePrePlaceConstraints() {
        //create preplace constraints for transitions
        for (Transition_t tid = 0; tid < _currentNet._transitions.size(); tid++) {
            auto& transition = _currentNet._transitions[tid];
            for (auto i = _currentNet._transitionArcs[tid].first; i < _currentNet._transitionArcs[tid].second; i++) {
                auto& arc = _currentNet._arcs[i];
                for (Variable_t var : arc.expression->getVariables()) {
                    const auto constraints = arc.expression->calculateVariableConstraints(var, arc.from);
                    auto entry = transition.preplacesVariableConstraints.find(var);
                    if (entry == transition.preplacesVariableConstraints.end()) {
                        entry = transition.preplacesVariableConstraints.emplace(var, std::vector<VariableConstraint> {}).first;
                    }
                    entry->second.insert(entry->second.begin(), constraints.cbegin(), constraints.cend());
                }
            }
        }
    }

    void ColoredPetriNetBuilder::sort() {

    }

    std::unordered_map<std::string, uint32_t> ColoredPetriNetBuilder::takePlaceIndices() {
        return std::move(_placeIndices);
    }

    std::unordered_map<std::string, Transition_t> ColoredPetriNetBuilder::takeTransitionIndices() {
        return std::move(_transitionIndices);
    }
}
