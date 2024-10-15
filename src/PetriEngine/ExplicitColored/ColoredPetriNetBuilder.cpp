#include "PetriEngine/ExplicitColored/ColoredPetriNetBuilder.h"

namespace PetriEngine {
    namespace ExplicitColored {
        ColoredPetriNetBuilder::ColoredPetriNetBuilder() {
            auto dotBasicColorType = std::make_shared<BaseColorType>();
            dotBasicColorType->colors = 1;
            
            _dotColorType = std::make_shared<ColorType>();
            _dotColorType->size = 1;
            _dotColorType->basicColorTypes.emplace_back(std::move(dotBasicColorType));
            _colors = std::make_shared<Colored::ColorTypeMap>();
        }

        void ColoredPetriNetBuilder::addPlace(const std::string& name, uint32_t tokens, double, double) {
            _currentNet._places.push_back({
                _dotColorType
            });

            _placeIndices[name] = _currentNet._places.size() - 1;

            auto multiSet = CPNMultiSet();
            multiSet.SetCount({DOT_COLOR}, tokens);

            _currentNet._initialMarking.markings.push_back(multiSet);
        }

        void ColoredPetriNetBuilder::addPlace(const std::string& name, const Colored::ColorType* type, Colored::Multiset&& tokens, double, double) {
            
            _currentNet._places.push_back({
                type
            })
        }

        void ColoredPetriNetBuilder::addTransition(const std::string& name, int32_t, double, double) {
            _currentNet._transitions.push_back({
                nullptr
            });
            _transitionIndices[name] = _currentNet._transitions.size();
        }

        void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, bool inhibitor, uint32_t weight) {
            UnresolvedInputArc inputArc;
            inputArc.place = place;
            inputArc.transition = transition;
            inputArc.weight = weight;
            inputArc.inhibitor = inhibitor;

            _unresolvedInputArcs.push_back(std::move(inputArc));
        }

        void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, uint32_t weight) {
            UnresolvedOutputArc outputArc;
            outputArc.transition = transition;
            outputArc.place = place;
            outputArc.weight = weight;

            _unresolvedOutputArcs.push_back(std::move(outputArc));
        }

        void ColoredPetriNetBuilder::addColorType(const std::string& id, const Colored::ColorType* type) {
            if (auto productType = dynamic_cast<const Colored::ProductType*>(type)) {
                std::vector<std::string> productColorType;
                for (size_t i = 0; i < productType->getConstituentsSizes().size(); i++) {
                    auto colorType = productType->getNestedColorType(i);
                    productColorType.push_back(colorType->getName());
                }
                _productTypes[id] = std::move(productColorType);
            } else {
                auto colorType = std::make_shared<ColorType>();
                auto baseColorType = std::make_shared<BaseColorType>();
                for (const auto& color : *type) {
                    baseColorType->colors += 1;
                }
                colorType->size += 1;
                _baseColorType[id] = baseColorType;
                colorType->basicColorTypes.push_back(std::move(baseColorType));
                _colorTypeMap[id] = std::move(colorType);
            }
            (*_colors)[id] = type;
        }

        void ColoredPetriNetBuilder::addVariable(const PetriEngine::Colored::Variable* variable) {
            _variableTypes[variable->name] = variable->colorType->getName();
        }

        ColoredPetriNet ColoredPetriNetBuilder::build() {
            for (const auto& productColor : _productTypes) {
                ColorType colorType;
                for (const std::string& colorId : productColor.second) {
                    const auto colorTypeIt = _baseColorType.find(colorId);
                    if (colorTypeIt != _baseColorType.cend()) {
                        throw base_error("Product color uses unknown color type ", colorId);
                    }
                    colorType.basicColorTypes.push_back(colorTypeIt->second);
                    colorType.size += 1;
                }
            }

            for (const auto& variableName : _variableTypes) {
                Variable variable;
                
                auto colorTypeIt = _baseColorType.find(variableName.second);
                if (colorTypeIt == _baseColorType.end()) {
                    throw base_error("Variable uses unknown base type ", variableName.first);
                }

                variable.colorType = colorTypeIt->second;

                variable.id = _currentNet._variables.size();

                _currentNet._variables.push_back(std::move(variable));
            }

            for (const auto& outputArc : _unresolvedOutputArcs) {
                ColoredPetriNetArc arc;
                size_t placeIndex = _placeIndices.find(outputArc.place)->second;
                
                arc.from = _transitionIndices.find(outputArc.transition)->second;
                arc.to  = placeIndex;
                arc.colorType = _currentNet._places[placeIndex].colorType;

                _currentNet._outputArcs.push_back(std::move(arc));
            }

            for (const auto& inputArc : _unresolvedInputArcs) {
                ColoredPetriNetArc arc;
                size_t placeIndex = _placeIndices.find(inputArc.place)->second;
                
                arc.from = placeIndex;
                arc.to = _transitionIndices.find(inputArc.transition)->second;
                arc.colorType = _currentNet._places[placeIndex].colorType;

                if (inputArc.arcExpression == nullptr) {
                    auto expr = std::make_shared<Colored::NumberOfExpression>(
                        std::vector<Colored::ColorExpression_ptr>{
                            std::make_shared<Colored::DotConstantExpression>()
                        },
                        inputArc.weight
                    );
                    arc.arcExpression = std::make_unique<ArcExpression>(expr, _colors);
                } else {
                    arc.arcExpression = std::make_unique<ArcExpression>(inputArc.arcExpression, _colors);
                }

                _currentNet._inputArcs.push_back(std::move(arc));
                
            }

            for (const auto& inhibitorArc : _unresolvedInhibitorArcs) {
                ColoredPetriNetInhibitor inhibitor;
                inhibitor.from = _placeIndices.find(inhibitorArc.place)->second;
                inhibitor.to = _transitionIndices.find(inhibitorArc.transition)->second;
                inhibitor.weight = inhibitorArc.weight;
                _currentNet._inhibitorArcs.push_back(inhibitor);
            }

            return std::move(_currentNet);
        }
    }
}