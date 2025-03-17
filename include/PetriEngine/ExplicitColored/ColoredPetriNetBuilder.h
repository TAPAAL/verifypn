#ifndef COLORED_PETRI_NET_BUILDER_H
#define COLORED_PETRI_NET_BUILDER_H
#include "PetriEngine/AbstractPetriNetBuilder.h"
#include "ColoredPetriNet.h"

namespace PetriEngine::ExplicitColored {
    enum class ColoredPetriNetBuilderStatus {
        OK,
        TOO_MANY_BINDINGS
    };
    class ColoredPetriNetBuilder final : public AbstractPetriNetBuilder {
    public:
        ColoredPetriNetBuilder();
        ColoredPetriNetBuilder(ColoredPetriNetBuilder&&) = default;
        ColoredPetriNetBuilder& operator=(const ColoredPetriNetBuilder&) = default;

        void addPlace(const std::string& name, uint32_t tokens, double, double) override;
        void addTransition(const std::string& name, int32_t, double, double) override;
        void addInputArc(const std::string& place, const std::string& transition, bool inhibitor, uint32_t weight) override;
        void addOutputArc(const std::string& transition, const std::string& place, uint32_t weight) override;

        void addPlace(const std::string& name, const Colored::ColorType* type, Colored::Multiset&& tokens, double, double) override;
        void addTransition(const std::string& name, const Colored::GuardExpression_ptr& guard, int32_t, double, double) override;
        void addInputArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr, uint32_t inhib_weight) override;
        void addOutputArc(const std::string& transition, const std::string& place, const Colored::ArcExpression_ptr& expr) override;
        void addColorType(const std::string& id, const Colored::ColorType* type) override;
        void addVariable(const Colored::Variable* variable) override;
        void addToColorType(Colored::ProductType* colorType, const Colored::ColorType* newConstituent) override;
        void sort() override;

        std::unordered_map<std::string, uint32_t> takePlaceIndices();
        std::unordered_map<std::string, Transition_t> takeTransitionIndices();
        ColoredPetriNetBuilderStatus build();
        ColoredPetriNet takeNet();
    private:
        std::unordered_map<std::string, uint32_t> _placeIndices;
        std::unordered_map<std::string, uint32_t> _transitionIndices;
        std::shared_ptr<std::unordered_map<std::string, Variable_t>> _variableMap;
        std::unordered_map<std::string, std::shared_ptr<ColorType>> _colorTypeMap;
        std::vector<ColoredPetriNetArc> _outputArcs;
        std::vector<ColoredPetriNetArc> _inputArcs;
        ColoredPetriNet _currentNet;
        std::shared_ptr<ColorType> _dotColorType;
        std::shared_ptr<Colored::ColorTypeMap> _colors;

        void _createArcsAndTransitions();
        ColoredPetriNetBuilderStatus _calculateTransitionVariables();
        void _calculatePrePlaceConstraints();
    };
}



#endif