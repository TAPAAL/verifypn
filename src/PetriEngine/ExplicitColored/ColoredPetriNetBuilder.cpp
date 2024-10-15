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
            _variableMap = std::make_shared<std::unordered_map<std::string, Variable_t>>();
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

        void ColoredPetriNetBuilder::addTransition(const std::string& name, int32_t, double, double) {
            _currentNet._transitions.push_back({
                nullptr
            });
            _transitionIndices[name] = _currentNet._transitions.size();
        }

         void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, bool inhibitor, uint32_t weight) {
            ColoredPetriNetArc arc;
            size_t placeIndex = _placeIndices.find(place)->second;
                
            arc.from = placeIndex;
            arc.to = _transitionIndices.find(transition)->second;
            arc.colorType = _currentNet._places[placeIndex].colorType;

            auto expr = std::make_shared<Colored::NumberOfExpression>(
                std::vector<Colored::ColorExpression_ptr>{
                    std::make_shared<Colored::DotConstantExpression>()
                },
                weight
            );
            arc.arcExpression = std::make_unique<ArcExpression>(expr, _colors);
            

            _currentNet._inputArcs.push_back(std::move(arc));
        }

        void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, uint32_t weight) {
            ColoredPetriNetArc arc;
            size_t placeIndex = _placeIndices.find(place)->second;
                
            arc.from = _transitionIndices.find(transition)->second;
            arc.to  = placeIndex;
            arc.colorType = _currentNet._places[placeIndex].colorType;

            _currentNet._outputArcs.push_back(std::move(arc));
        }

        void ColoredPetriNetBuilder::addPlace(const std::string& name, const Colored::ColorType* type, Colored::Multiset&& tokens, double, double) {
            ColoredPetriNetPlace place;
            place.colorType = _colorTypeMap.find(name)->second;
            _currentNet._places.push_back(std::move(place));
            
            CPNMultiSet multiSet;
            
            for (const auto& token : tokens) {
                std::vector<Color_t> colorSequrence;
                for (const auto& color : token.first->getTupleColors()) {
                    colorSequrence.push_back(color->getId());
                }
                multiSet.SetCount(colorSequrence, token.second);
            }
        }

        void ColoredPetriNetBuilder::addTransition(const std::string& name, const Colored::GuardExpression_ptr& guard, int32_t, double, double) {
            ColoredPetriNetTransition transition;
            transition.guardExpression = std::make_unique<GuardExpression>(_colors, guard, _variableMap);
            _currentNet._transitions.emplace_back(std::move(transition));
            _transitionIndices.emplace(std::make_pair(name, _currentNet._transitions.size() - 1));
        }

        void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr, uint32_t inhib_weight) {    
            ColoredPetriNetArc arc;
            size_t placeIndex = _placeIndices.find(place)->second;
                
            arc.from = placeIndex;
            arc.to = _transitionIndices.find(transition)->second;
            arc.colorType = _currentNet._places[placeIndex].colorType;
            arc.arcExpression = std::make_unique<ArcExpression>(expr, _colors);
            
            _currentNet._inputArcs.push_back(std::move(arc));
        }

        void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, const Colored::ArcExpression_ptr& expr) {
            ColoredPetriNetArc arc;
            size_t placeIndex = _placeIndices.find(place)->second;

            arc.from = _transitionIndices.find(transition)->second;
            arc.to = placeIndex;
            arc.colorType = _currentNet._places[placeIndex].colorType;
            arc.arcExpression = std::make_unique<ArcExpression>(expr, _colors);

            _currentNet._outputArcs.push_back(std::move(arc));
        }

        void ColoredPetriNetBuilder::addColorType(const std::string& id, const Colored::ColorType* type) {
            if (auto productType = dynamic_cast<const Colored::ProductType*>(type)) {
                auto productColorType = std::make_shared<ColorType>();
                for (size_t i = 0; i < productType->getConstituentsSizes().size(); i++) {
                    auto baseColorType = _baseColorType.find(productType->getNestedColorType(i)->getName())->second;
                    productColorType->basicColorTypes.push_back(std::move(baseColorType));
                    productColorType->size++;
                }
                _colorTypeMap.emplace(std::make_pair(id, std::move(productColorType)));
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
            (*_variableMap)[variable->name] = _variableMap->size();
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
            return std::move(_currentNet);
        }

        void ColoredPetriNetBuilder::sort() {
            
        }
    }
}