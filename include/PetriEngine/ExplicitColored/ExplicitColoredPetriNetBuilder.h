#ifndef EXPLICIT_COLORED_PETRI_NET_BUILDER_H
#define EXPLICIT_COLORED_PETRI_NET_BUILDER_H
#include "PetriEngine/AbstractPetriNetBuilder.h"
#include "ColoredPetriNet.h"

namespace PetriEngine::ExplicitColored {
    enum class ColoredPetriNetBuilderStatus {
        OK,
        TOO_MANY_BINDINGS
    };

    class ExplicitColoredPetriNetBuilder final : public AbstractPetriNetBuilder {
    public:
        ExplicitColoredPetriNetBuilder();
        ExplicitColoredPetriNetBuilder(ExplicitColoredPetriNetBuilder&&) = default;
        ExplicitColoredPetriNetBuilder& operator=(const ExplicitColoredPetriNetBuilder&) = default;

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

        const std::unordered_map<std::string, uint32_t>& getPlaceIndices() const;
        const std::unordered_map<std::string, Transition_t>& getTransitionIndices() const;
        const std::shared_ptr<std::unordered_map<std::string, Variable_t>>& getVariableIndices() const;
        const std::vector<const Colored::ColorType*>& getUnderlyingVariableColorTypes() const;
        const Colored::ColorType* getPlaceUnderlyingColorType(Place_t place) const;

        const std::string& getPlaceName(Place_t placeIndex) const;
        const std::string& getVariableName(Variable_t variableIndex) const;
        const std::string& getTransitionName(Transition_t transitionIndex) const;

        size_t getPlaceCount() const;
        size_t getVariableCount() const;
        size_t getTransitionCount() const;

        ColoredPetriNetBuilderStatus build();
        ColoredPetriNet takeNet();
    private:
        std::unordered_map<Place_t, const Colored::ColorType*> _underlyingColorType;
        std::unordered_map<std::string, Place_t> _placeIndices;
        std::unordered_map<std::string, Transition_t> _transitionIndices;
        std::shared_ptr<std::unordered_map<std::string, Variable_t>> _variableMap;
        std::unordered_map<std::string, std::shared_ptr<ColorType>> _colorTypeMap;
        std::vector<ColoredPetriNetArc> _outputArcs;
        std::vector<ColoredPetriNetArc> _inputArcs;
        std::vector<std::tuple<Transition_t, Transition_t, Colored::ArcExpression_ptr>> _inputArcsToCompile;
        std::vector<std::tuple<Transition_t, Transition_t, Colored::ArcExpression_ptr>> _outputArcsToCompile;
        std::vector<std::pair<Transition_t, const Colored::GuardExpression_ptr>> _guardsToCompile;
        std::vector<std::shared_ptr<ColorType>> _variablesToAdd;
        ColoredPetriNet _currentNet;
        std::shared_ptr<ColorType> _dotColorType;
        std::shared_ptr<Colored::ColorTypeMap> _colors;
        std::vector<const Colored::ColorType*> _underlyingVariableColorTypes;

        std::unordered_map<Place_t, std::string> _placeToId;
        std::unordered_map<Transition_t, std::string> _transitionToId;
        std::unordered_map<Variable_t, std::string> _variableToId;

        void _createArcsAndTransitions();
        ColoredPetriNetBuilderStatus _calculateTransitionVariables();
        void _calculatePrePlaceConstraints();
        void _compileUncompiledArcs();
        void _compileUncompiledGuards();
        void _addVariables();
        void _fillLookupTables();
    };
}



#endif