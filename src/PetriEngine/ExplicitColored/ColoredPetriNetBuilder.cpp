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
            multiSet.setCount(ColorSequence({DOT_COLOR}), tokens);

            _currentNet._initialMarking.markings.push_back(multiSet);
        }

        void ColoredPetriNetBuilder::addTransition(const std::string& name, int32_t, double, double) {
            _currentNet._transitions.push_back({
                nullptr
            });
            _transitionIndices[name] = _currentNet._transitions.size() - 1;
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
            arc.arcExpression = std::make_unique<ArcExpression>(expr, _colors, _variableMap);

            _currentNet._inputArcs.push_back(std::move(arc));
        }

        void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, uint32_t weight) {
            ColoredPetriNetArc arc;
            size_t placeIndex = _placeIndices.find(place)->second;
                
            arc.from = _transitionIndices.find(transition)->second;
            arc.to = placeIndex;
            arc.colorType = _currentNet._places[placeIndex].colorType;

            auto expr = std::make_shared<Colored::NumberOfExpression>(
                std::vector<Colored::ColorExpression_ptr>{
                    std::make_shared<Colored::DotConstantExpression>()
                },
                weight
            );
            arc.arcExpression = std::make_unique<ArcExpression>(expr, _colors, _variableMap);

            _currentNet._outputArcs.push_back(std::move(arc));
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
            transition.guardExpression = guard == nullptr
                ? nullptr
                : std::make_unique<GuardExpression>(_colors, guard, _variableMap);
            _currentNet._transitions.emplace_back(std::move(transition));
            _transitionIndices.emplace(name, _currentNet._transitions.size() - 1);
        }

        void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr, uint32_t inhib_weight) {    
            ColoredPetriNetArc arc;
            size_t placeIndex = _placeIndices.find(place)->second;

            arc.from = placeIndex;
            arc.to = _transitionIndices.find(transition)->second;
            arc.colorType = _currentNet._places[placeIndex].colorType;
            arc.arcExpression = std::make_unique<ArcExpression>(expr, _colors, _variableMap);
            
            _currentNet._inputArcs.push_back(std::move(arc));
        }

        void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, const Colored::ArcExpression_ptr& expr) {
            ColoredPetriNetArc arc;
            size_t placeIndex = _placeIndices.find(place)->second;

            arc.from = _transitionIndices.find(transition)->second;
            arc.to = placeIndex;
            arc.colorType = _currentNet._places[placeIndex].colorType;
            arc.arcExpression = std::make_unique<ArcExpression>(expr, _colors, _variableMap);

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
                _colorTypeMap.emplace(id, std::move(productColorType));
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
            Variable var;
            var.colorType = _baseColorType.find(variable->colorType->getName())->second;
            _currentNet._variables.emplace_back(std::move(var));
        }

        ColoredPetriNet ColoredPetriNetBuilder::build() {
            for (const auto& productColor : _productTypes) {
                ColorType colorType;
                colorType.size = 0;
                for (const std::string& colorId : productColor.second) {//HMM
                    const auto colorTypeIt = _baseColorType.find(colorId);
                    if (colorTypeIt != _baseColorType.cend()) {
                        throw base_error("Product color uses unknown color type ", colorId);
                    }
                    colorType.basicColorTypes.push_back(colorTypeIt->second);
                    colorType.size += 1;
                }
            }
            _currentNet.fillValidVariables();
            auto transitions = _currentNet._transitions.size();
            std::vector<std::pair<uint32_t,uint32_t>> transIndices = std::vector<std::pair<uint32_t,uint32_t>>(transitions + 1);
            std::vector<ColoredPetriNetArc*> arcPointers = std::vector<ColoredPetriNetArc*>(_currentNet._outputArcs.size() + _currentNet._inputArcs.size());
            auto arcIndex = 0;
            for (size_t i = 0; i < transitions; i++){
                auto inputIndex= arcIndex;
                for (auto& a : _currentNet._inputArcs){
                    if (a.to == i){
                        arcPointers[arcIndex]= &a;
                        arcIndex++;
                    }
                }
                transIndices[i] = std::pair{inputIndex, arcIndex};
                //The fewest possible variable combinations are sorted first to hopefully minimize the testing of bindings
                if (arcIndex - inputIndex > 1){
                    std::sort(arcPointers.begin() + inputIndex, arcPointers.begin() + arcIndex - 1,
                              [](const ColoredPetriNetArc* a, const ColoredPetriNetArc* b){
                        uint32_t aPossible = 1;
                        uint32_t bPossible = 1;
                        for (auto& v : a->validVariables){
                            aPossible *= v.second.size();
                        }
                        for (auto& v : b->validVariables){
                            bPossible *= v.second.size();
                        }
                        return aPossible < bPossible;
                    });
                }

                for (auto& a : _currentNet._outputArcs){
                    if (a.from == i){
                        arcPointers[arcIndex]= &a;
                        arcIndex++;
                    }
                }
            }
            transIndices.back() = std::pair(arcIndex,arcIndex);
            _currentNet._invariants = std::move(arcPointers);
            _currentNet._transitionArcs = std::move(transIndices);
            return std::move(_currentNet);
        }

        void ColoredPetriNetBuilder::sort() {
            
        }

        std::unordered_map<std::string, uint32_t> ColoredPetriNetBuilder::takePlaceIndices() {
            return std::move(_placeIndices);
        }
    }
}