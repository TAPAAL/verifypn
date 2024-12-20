#include "PetriEngine/ExplicitColored/ColoredPetriNetBuilder.h"
#include <PetriEngine/ExplicitColored/ArcCompiler.h>
#include "PetriEngine/ExplicitColored/ValidVariableGenerator.h"
#include "PetriEngine/ExplicitColored/GuardCompiler.h"

namespace PetriEngine {
    namespace ExplicitColored {
        ColoredPetriNetBuilder::ColoredPetriNetBuilder() {
            auto dotBasicColorType = std::make_shared<BaseColorType>();
            dotBasicColorType->colors = 1;
            
            _dotColorType = std::make_shared<ColorType>();
            _dotColorType->size = 1;
            _dotColorType->basicColorTypes.emplace_back(std::move(dotBasicColorType));
            _colors = std::make_shared<Colored::ColorTypeMap>();
            _variableMap = std::make_shared<std::unordered_map<std::string, Variable_t>>();
        }

        void ColoredPetriNetBuilder::addPlace(const std::string& name, uint32_t tokens, double, double) {
            _currentNet._places.push_back({
                _dotColorType
            });

            _placeIndices[name] = _currentNet._places.size() - 1;

            auto multiSet = CPNMultiSet();
            multiSet.setCount(ColorSequence({DOT_COLOR}), tokens);

            _currentNet._initialMarking.markings.push_back(multiSet);
        }

        void ColoredPetriNetBuilder::addTransition(const std::string& name, int32_t, double, double) {
            _currentNet._transitions.push_back({
                nullptr
            });
            _transitionIndices[name] = _currentNet._transitions.size() - 1;
        }

        //Dot color input arc
         void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, bool inhibitor, uint32_t weight) {
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

                 _inputArcs.push_back(ColoredPetriNetArc {
                     from,
                     to,
                     _currentNet._places[from].colorType,
                     ArcCompiler::compileArc(expr, *_variableMap, *_colors)
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
                auto arc = ArcCompiler::compileArc(expr, *_variableMap, *_colors);

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
            _outputArcs.push_back(ColoredPetriNetArc {
                _transitionIndices.find(transition)->second,
                placeIndex,
                _currentNet._places[placeIndex].colorType,
                ArcCompiler::compileArc(expr, *_variableMap, *_colors)
            });
        }

        //Colored output arc
        void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, const Colored::ArcExpression_ptr& expr) {
            auto placeIndex = _placeIndices.find(place)->second;
            auto arc = ArcCompiler::compileArc(expr, *_variableMap, *_colors);

            _outputArcs.push_back(ColoredPetriNetArc {
                _transitionIndices.find(transition)->second,
                placeIndex,
                _currentNet._places[placeIndex].colorType,
                std::move(arc)
            });
        }

        void ColoredPetriNetBuilder::addPlace(const std::string& name, const Colored::ColorType* type, Colored::Multiset&& tokens, double, double) {
            ColoredPetriNetPlace place;
            place.colorType = _colorTypeMap.find(type->getName())->second;

            CPNMultiSet multiSet;
            for (const auto& [tokenColor, count] : tokens) {
                std::vector<Color_t> colorSequence;
                if (tokenColor->isTuple()) {
                    for (const auto& color : tokenColor->getTupleColors()) {
                        colorSequence.push_back(color->getId());
                    }
                } else {
                    colorSequence.push_back(tokenColor->getId());
                }

                multiSet.setCount(ColorSequence(colorSequence), count);
            }

            _currentNet._places.push_back(std::move(place));
            _currentNet._initialMarking.markings.push_back(multiSet);

            _placeIndices[name] = _currentNet._places.size() - 1;
        }

        void ColoredPetriNetBuilder::addTransition(const std::string& name, const Colored::GuardExpression_ptr& guard, int32_t, double, double) {
            ColoredPetriNetTransition transition;
            GuardCompiler compiler(*_variableMap, *_colors);
            if (guard != nullptr) {
                auto compiled = compiler.compile(*guard);
                transition.guardExpression = std::move(compiled.first);
                transition.variables = std::move(compiled.second);
            } else {
                transition.guardExpression = nullptr;
            }
            _currentNet._transitions.emplace_back(std::move(transition));
            _transitionIndices.emplace(name, _currentNet._transitions.size() - 1);
        }

        void ColoredPetriNetBuilder::addColorType(const std::string& id, const Colored::ColorType* type) {
            if (auto productType = dynamic_cast<const Colored::ProductType*>(type)) {
                auto productColorType = std::make_shared<ColorType>();
                for (size_t i = 0; i < productType->getConstituentsSizes().size(); i++) {
                    auto baseColorType = _baseColorType.find(productType->getNestedColorType(i)->getName())->second;
                    productColorType->basicColorTypes.push_back(std::move(baseColorType));
                    productColorType->size++;
                }
                _colorTypeMap.emplace(id, std::move(productColorType));
            } else {
                auto colorType = std::make_shared<ColorType>();
                auto baseColorType = std::make_shared<BaseColorType>();
                baseColorType->colors = type->size();
                colorType->size = 1;
                _baseColorType[id] = baseColorType;
                colorType->basicColorTypes.push_back(std::move(baseColorType));
                _colorTypeMap[id] = std::move(colorType);
            }
            (*_colors)[id] = type;
        }

        void ColoredPetriNetBuilder::addVariable(const PetriEngine::Colored::Variable* variable) {
            (*_variableMap)[variable->name] = _variableMap->size();
            Variable var;
            var.colorType = _baseColorType.find(variable->colorType->getName())->second;
            _currentNet._variables.emplace_back(std::move(var));
        }

        ColoredPetriNetBuilderStatus ColoredPetriNetBuilder::build() {
            auto transitions = _currentNet._transitions.size();
            std::vector<std::pair<uint32_t,uint32_t>> transIndices = std::vector<std::pair<uint32_t,uint32_t>>(transitions + 1);
            std::vector<ColoredPetriNetArc> arcs = std::vector<ColoredPetriNetArc>{};
            std::vector<uint32_t> inhibIndices = std::vector<uint32_t>(transitions + 1, _currentNet._inhibitorArcs.size());

            std::sort(_currentNet._inhibitorArcs.begin(), _currentNet._inhibitorArcs.end(),
                      [](ColoredPetriNetInhibitor a, ColoredPetriNetInhibitor b){
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
                auto to = _currentNet._inhibitorArcs[inhibIndex].to;
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
            auto result = ValidVariableGenerator{_currentNet}.generateValidColorsForTransitions();
            if (result == ValidVariableGeneratorStatus::TOO_MANY_BINDINGS) {
                return ColoredPetriNetBuilderStatus::TOO_MANY_BINDINGS;
            }
            return ColoredPetriNetBuilderStatus::OK;
        }

        ColoredPetriNet ColoredPetriNetBuilder::takeNet() {
            return std::move(_currentNet);
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
}