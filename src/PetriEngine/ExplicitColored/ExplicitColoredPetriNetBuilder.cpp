#include "PetriEngine/ExplicitColored/ExplicitColoredPetriNetBuilder.h"
#include "PetriEngine/ExplicitColored/ExpressionCompilers/ArcCompiler.h"
#include "PetriEngine/ExplicitColored/ExpressionCompilers/GuardCompiler.h"
#include <algorithm>

namespace PetriEngine::ExplicitColored {
    ExplicitColoredPetriNetBuilder::ExplicitColoredPetriNetBuilder() {
        _dotColorType = std::make_shared<ColorType>(1);
        _colors = std::make_shared<Colored::ColorTypeMap>();
        _variableMap = std::make_shared<std::unordered_map<std::string, Variable_t>>();
    }

    void ExplicitColoredPetriNetBuilder::addPlace(const std::string& name, const uint32_t tokens, double, double)
    {
        _currentNet._places.push_back({
            _dotColorType
        });

        _placeIndices[name] = _currentNet._places.size() - 1;

        auto multiSet = CPNMultiSet();
        multiSet.setCount(ColorSequence{DOT_COLOR}, tokens);

        _currentNet._initialMarking.markings.push_back(multiSet);
    }

    void ExplicitColoredPetriNetBuilder::addTransition(const std::string& name, int32_t, double, double) {
        _currentNet._transitions.push_back({
            nullptr
        });
        _transitionIndices[name] = _currentNet._transitions.size() - 1;
    }

    //Dot color input arc
     void ExplicitColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, const bool inhibitor, uint32_t weight) {
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
    void ExplicitColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr, uint32_t inhib_weight) {
        auto from = _placeIndices.find(place)->second;
        auto to = _transitionIndices.find(transition)->second;
        if (inhib_weight != 0) {
            _currentNet._inhibitorArcs.emplace_back(from, to, inhib_weight);
        } else {
            _inputArcsToCompile.emplace_back(from, to, expr);
        }

    }

    //Non colored output arc
    void ExplicitColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, uint32_t weight) {
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
    void ExplicitColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, const Colored::ArcExpression_ptr& expr) {
        const auto from = _transitionIndices.find(transition)->second;
        const auto to = _placeIndices.find(place)->second;
        _outputArcsToCompile.emplace_back(from, to, expr);
    }

    void ExplicitColoredPetriNetBuilder::addPlace(const std::string& name, const Colored::ColorType* type, Colored::Multiset&& tokens, double, double) {
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
        _underlyingColorType[_currentNet._places.size() - 1] = type;
    }

    void ExplicitColoredPetriNetBuilder::addTransition(const std::string& name, const Colored::GuardExpression_ptr& guard, int32_t, double, double) {
        ColoredPetriNetTransition transition;
        if (guard != nullptr) {
            _guardsToCompile.emplace_back(_currentNet._transitions.size(), guard);
        }
        _currentNet._transitions.emplace_back(std::move(transition));
        _transitionIndices.emplace(name, _currentNet._transitions.size() - 1);
    }
    void ExplicitColoredPetriNetBuilder::_compileUncompiledGuards() {
        const GuardCompiler compiler(*_variableMap, *_colors);
        for (const auto& [tid, guard] : _guardsToCompile) {
            _currentNet._transitions[tid].guardExpression = compiler.compile(*guard);
        }
        _guardsToCompile.clear();
    }

    void ExplicitColoredPetriNetBuilder::addColorType(const std::string& id, const Colored::ColorType* type) {
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

    void ExplicitColoredPetriNetBuilder::addToColorType(Colored::ProductType* colorType, const Colored::ColorType* newConstituent) {
        const auto& productType = _colorTypeMap[colorType->getName()];
        const auto colorSize = newConstituent->getConstituentsSizes()[0];
        productType->addBaseColorSize(colorSize);
        //Not related to building our net but the PNML writer needs the pointer updated
        colorType->addType(newConstituent);
    }

    void ExplicitColoredPetriNetBuilder::addVariable(const Colored::Variable* variable) {
        (*_variableMap)[variable->name] = _variableMap->size();
        const auto colorType = _colorTypeMap.find(variable->colorType->getName())->second;
        _variablesToAdd.push_back(colorType);
        _underlyingVariableColorTypes.push_back(variable->colorType);
    }

    ColoredPetriNetBuilderStatus ExplicitColoredPetriNetBuilder::build() {
        _addVariables();
        _compileUncompiledGuards();
        _compileUncompiledArcs();
        _createArcsAndTransitions();
        const auto status = _calculateTransitionVariables();
        if (status != ColoredPetriNetBuilderStatus::OK) {
            return status;
        }

        _calculatePrePlaceConstraints();

        return ColoredPetriNetBuilderStatus::OK;
    }

    ColoredPetriNet ExplicitColoredPetriNetBuilder::takeNet() {
        return std::move(_currentNet);
    }

    void ExplicitColoredPetriNetBuilder::_createArcsAndTransitions() {
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

    ColoredPetriNetBuilderStatus ExplicitColoredPetriNetBuilder::_calculateTransitionVariables() {
        for (auto tid = 0; tid < _currentNet._transitions.size(); tid++) {
            std::set<Variable_t> relevantVariables;
            _currentNet.extractGuardVariables(tid, relevantVariables);
            _currentNet.extractInputVariables(tid, relevantVariables);
            _currentNet.extractOutputVariables(tid, relevantVariables);

            Binding_t totalBindings = 0;
            bool first = true;
            for (const auto variableIndex : relevantVariables) {
                const auto varColorCount = _currentNet._variables[variableIndex].colorSize;
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

    void ExplicitColoredPetriNetBuilder::_calculatePrePlaceConstraints() {
        //create preplace constraints for transitions
        for (Transition_t tid = 0; tid < _currentNet._transitions.size(); tid++) {
            auto& transition = _currentNet._transitions[tid];
            for (auto i = _currentNet._transitionArcs[tid].first; i < _currentNet._transitionArcs[tid].second; i++) {
                const auto& arc = _currentNet._arcs[i];
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

    void ExplicitColoredPetriNetBuilder::_compileUncompiledArcs() {
        const ArcCompiler arcCompiler(*_variableMap, *_colors);

        auto inputIterator = _inputArcs.cbegin();
        Place_t inputPlaceId = 0;
        while (!_inputArcsToCompile.empty()) {
            const auto [from, to, expr] = _inputArcsToCompile.back();
            _inputArcsToCompile.pop_back();
            while (inputIterator != _inputArcs.cend() && inputPlaceId < to) {
                inputPlaceId = inputIterator->to;
                ++inputIterator;
            }
            auto arc = arcCompiler.compile(expr);
            inputIterator = _inputArcs.emplace(inputIterator, ColoredPetriNetArc {
                from,
                to,
                _currentNet._places[from].colorType,
                std::move(arc)
            });
        }

        auto outputIterator = _outputArcs.cbegin();
        Place_t outputPlaceId = 0;
        while (!_outputArcsToCompile.empty()) {
            const auto [from, to, expr] = _outputArcsToCompile.back();
            for (;outputIterator != _outputArcs.cend() && outputPlaceId < from;++outputIterator) {
                outputPlaceId = outputIterator->from;
            }
            auto arc = arcCompiler.compile(expr);
            outputIterator = _outputArcs.emplace(outputIterator, ColoredPetriNetArc {
                from,
                to,
                _currentNet._places[to].colorType,
                std::move(arc)
            });
            _outputArcsToCompile.pop_back();
        }
    }

    void ExplicitColoredPetriNetBuilder::_addVariables() {
        for (auto it = _variablesToAdd.begin(); it != _variablesToAdd.end(); ++it ) {
            Variable var{};
            var.colorSize = (*it)->colorSize;
            _currentNet._variables.emplace_back(var);
        }
        _variablesToAdd.clear();
    }

    void ExplicitColoredPetriNetBuilder::sort() {
    }

    const std::unordered_map<std::string, Place_t>& ExplicitColoredPetriNetBuilder::getPlaceIndices() const {
        return _placeIndices;
    }

    const std::unordered_map<std::string, Transition_t>& ExplicitColoredPetriNetBuilder::getTransitionIndices() const {
        return std::move(_transitionIndices);
    }

    const std::shared_ptr<std::unordered_map<std::string, Variable_t>>& ExplicitColoredPetriNetBuilder::getVariableIndices() const {
        return _variableMap;
    }

    const std::vector<const Colored::ColorType *>& ExplicitColoredPetriNetBuilder::getUnderlyingVariableColorTypes() const {
        return std::move(_underlyingVariableColorTypes);
    }

    const Colored::ColorType * ExplicitColoredPetriNetBuilder::getPlaceUnderlyingColorType(Place_t place) const {
        return _underlyingColorType.find(place)->second;
    }
}
