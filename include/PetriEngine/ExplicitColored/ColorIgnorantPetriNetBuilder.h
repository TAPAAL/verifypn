#ifndef COLORIGNORANTPETRINETBUILDER_H
#define COLORIGNORANTPETRINETBUILDER_H
#include "ExpressionCompilers/ArcCompiler.h"
#include "AtomicTypes.h"
#include "PetriEngine/AbstractPetriNetBuilder.h"
#include "PetriEngine/PetriNetBuilder.h"

namespace PetriEngine::ExplicitColored {
    enum class ColoredIgnorantPetriNetBuilderStatus {
        OK,
        CONTAINS_NEGATIVE
    };
    class ColorIgnorantPetriNetBuilder final : public AbstractPetriNetBuilder {
    public:
        ColorIgnorantPetriNetBuilder(shared_string_set& string_set) : _builder(string_set), _foundNegative(false) {}

        ColorIgnorantPetriNetBuilder(ColorIgnorantPetriNetBuilder&&) = default;
        ColorIgnorantPetriNetBuilder(const ColorIgnorantPetriNetBuilder&) = default;
        ColorIgnorantPetriNetBuilder& operator=(ColorIgnorantPetriNetBuilder&&) = default;
        ColorIgnorantPetriNetBuilder& operator=(const ColorIgnorantPetriNetBuilder&) = default;

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

        ColoredIgnorantPetriNetBuilderStatus build();
        PetriNetBuilder getUnderlying();
    private:
        struct TransitionStore {
            std::vector<std::pair<std::string, std::unique_ptr<CompiledArcExpression>>> inputs;
            std::vector<std::pair<std::unique_ptr<CompiledArcExpression>, std::string>> outputs;
        };
        PetriNetBuilder _builder;
        bool _foundNegative;
        std::unordered_map<std::string, Variable_t> _variableMap;
        std::map<std::string, TransitionStore> _transitions;
        std::vector<std::tuple<std::string, std::string, int>> _inhibitors;
        Colored::ColorTypeMap _colors;
        Variable_t _nextVariable;
    };
}
#endif //COLORIGNORANTPETRINETBUILDER_H
