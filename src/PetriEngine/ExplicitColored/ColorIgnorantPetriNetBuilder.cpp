#include <VerifyPN.h>
#include "PetriEngine/ExplicitColored/ColorIgnorantPetriNetBuilder.h"
#include <PetriEngine/ExplicitColored/ExpressionCompilers/ArcCompiler.h>


namespace PetriEngine::ExplicitColored {
    class ColorOrVar {
    public:
        [[nodiscard]] static ColorOrVar fromColor(const Color_t color) {
            ColorOrVar rv;
            rv._value.color = color;
            rv._isColor = true;
            return rv;
        }

        [[nodiscard]] static ColorOrVar fromVariable(const Variable_t variable) {
            ColorOrVar rv;
            rv._value.variable = variable;
            rv._isColor = true;
            return rv;
        }

        [[nodiscard]] Color_t getColor() const {
            assert(_isColor);
            return _value.color;
        }

        [[nodiscard]] Variable_t getVariable() const {
            assert(!_isColor);
            return _value.variable;
        }

        [[nodiscard]] bool isColor() const {
            return _isColor;
        }
    private:
        ColorOrVar() {
            _value.color = 0;
            _isColor = true;
        }
        bool _isColor;
        union {
            Color_t color;
            Variable_t variable;
        } _value;
    };

    class Constraint {
    public:
        enum class ConstraintType {
            Equal,
            NotEqual
        };

        explicit Constraint(const ConstraintType constraintType) {
            _constraintType = constraintType;
        }
        ConstraintType getConstraintType() {
            return _constraintType;
        }
    private:
        ConstraintType _constraintType;
    };

    class Minus {

    };

    class Atom final : Minus {
    public:
        explicit Atom(const ColorOrVar value) : _value(value) {}
        const ColorOrVar& getValue() const {
            return _value;
        }

    private:
        ColorOrVar _value;
    };

    class SimpleMinus final : Minus {
    public:
        SimpleMinus(
            std::vector<std::unique_ptr<Minus>> strongSet,
            std::vector<std::unique_ptr<Minus>> weakSet
        ) : _strongSet(std::move(strongSet)), _weakSet(std::move(weakSet)) {};
    private:
        std::vector<std::unique_ptr<Minus>> _strongSet;
        std::vector<std::unique_ptr<Minus>> _weakSet;
    };


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
        _transitions.emplace(name, TransitionStore {});
        _builder.addTransition(name, player, x, y);
    }

    void ColorIgnorantPetriNetBuilder::addInputArc(const std::string &place, const std::string &transition,
        const Colored::ArcExpression_ptr &expr, uint32_t inhib_weight) {
        if (inhib_weight > 0) {
            _inhibitors.emplace_back(place, transition, inhib_weight);
            return;
        }
        ArcCompiler compiler(_variableMap, _colors);
        auto compiled = compiler.compile(expr);
        if (compiled->containsNegative()) {
            _foundNegative = true;
        }
        _transitions[transition].inputs.emplace_back(place, std::move(compiled));
    }

    void ColorIgnorantPetriNetBuilder::addOutputArc(const std::string &transition, const std::string &place,
        const Colored::ArcExpression_ptr &expr) {
        ArcCompiler compiler(_variableMap, _colors);
        auto compiled = compiler.compile(expr);
        if (compiled->containsNegative()) {
            _foundNegative = true;
        }
        _transitions[transition].outputs.emplace_back(std::move(compiled), place);
    }

    void ColorIgnorantPetriNetBuilder::addColorType(const std::string &id, const Colored::ColorType *type) {
        _colors[id]= type;
    }

    void ColorIgnorantPetriNetBuilder::addVariable(const Colored::Variable *variable) {
        _variableMap[variable->name] = _nextVariable++;
    }

    void ColorIgnorantPetriNetBuilder::addToColorType(Colored::ProductType *colorType,
        const Colored::ColorType *newConstituent) {
        //Not related to building our net but the PNML writer needs the pointer updated
        colorType->addType(newConstituent);
        _colors[colorType->getName()] = colorType;
    }

    void ColorIgnorantPetriNetBuilder::sort() {
        _builder.sort();
    }

    struct ArcRange {
        bool isInput;
        std::string place;
        MarkingCount_t lowerWeight;
        MarkingCount_t upperWeight;
        MarkingCount_t current;
    };

    ColoredIgnorantPetriNetBuilderStatus ColorIgnorantPetriNetBuilder::build() {
        std::map<std::string, std::vector<size_t>> transitionVariationCount;
        if (_foundNegative) {
            for (const auto& [transitionName, transitionStore] : _transitions) {
                std::vector<ArcRange> arcRanges;

                for (const auto& [place, arcExpr] : transitionStore.inputs) {
                    arcRanges.push_back({
                        true,
                        place,
                        arcExpr->getMinimalMarkingCount(),
                        arcExpr->getUpperBoundMarkingCount(),
                        arcExpr->getMinimalMarkingCount()
                    });
                }

                for (const auto& [arcExpr, place] : transitionStore.outputs) {
                    arcRanges.push_back({
                        false,
                        place,
                        arcExpr->getMinimalMarkingCount(),
                        arcExpr->getUpperBoundMarkingCount(),
                        arcExpr->getMinimalMarkingCount()
                    });
                }

                bool carry = false;
                size_t transitionCount = 0;
                while (!carry) {
                    std::string currentTransitionName = transitionName;
                    if (transitionCount++ != 0) {
                        currentTransitionName += "___" + std::to_string(transitionCount);
                    }

                    _builder.addTransition(currentTransitionName, 0, 0, 0);
                    carry = true;
                    for (auto& arcRange : arcRanges) {
                        if (arcRange.current > 0) {
                            if (arcRange.isInput) {
                                _builder.addInputArc(arcRange.place, currentTransitionName, false, arcRange.current);
                            } else {
                                _builder.addOutputArc(currentTransitionName, arcRange.place, arcRange.current);
                            }
                        }
                        if (carry) {
                            if (arcRange.current >= arcRange.upperWeight) {
                                arcRange.current = arcRange.lowerWeight;
                                carry = true;
                            } else {
                                arcRange.current += 1;
                                carry = false;
                            }
                        }
                    }
                }
                transitionVariationCount.emplace(transitionName, transitionCount);
            }
        } else {
            for (const auto& [transitionName, transitionStore] : _transitions) {
                _builder.addTransition(transitionName, 0, 0, 0);
                for (const auto& [place, arcExpr] : transitionStore.inputs) {
                    _builder.addInputArc(place, transitionName, false, arcExpr->getMinimalMarkingCount());
                }
                for (const auto& [arcExpr, place] : transitionStore.outputs) {
                    _builder.addOutputArc(transitionName, place, arcExpr->getMinimalMarkingCount());
                }
            }
        }
        for (const auto& [place, transition, weight] : _inhibitors) {
            auto transitionVariationCountIt = transitionVariationCount.find(transition);
            if (transitionVariationCountIt == transitionVariationCount.end()) {
                _builder.addInputArc(place, transition, true, weight);
            } else {
                for (size_t i = 0; i < transitionVariationCountIt->second.size(); i++) {
                    _builder.addInputArc(place, transition + "___" + std::to_string(i), true, weight);
                }
            }
        }
        return ColoredIgnorantPetriNetBuilderStatus::OK;
    }

    PetriNetBuilder ColorIgnorantPetriNetBuilder::getUnderlying() {
        return _builder;
    }
}
