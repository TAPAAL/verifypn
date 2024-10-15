#ifndef COLORED_PETRI_NET_BUILDER_H
#define COLORED_PETRI_NET_BUILDER_H
#include "PetriEngine/AbstractPetriNetBuilder.h"
#include "ColoredPetriNet.h"

namespace PetriEngine {
    namespace ExplicitColored {
        struct UnresolvedInputArc {
            std::string place;
            std::string transition;
            uint32_t weight;
            bool inhibitor;
            Colored::ArcExpression_ptr arcExpression;
        };

        struct UnresolvedOutputArc {
            std::string transition;
            std::string place;
            uint32_t weight;
            Colored::ArcExpression_ptr arcExpression;
        };

        struct UnresolvedInhibitorArc {
            std::string place;
            std::string transition;
            MarkingCount_t weight;
        };

        class ColoredPetriNetBuilder : public AbstractPetriNetBuilder {
        public:
            ColoredPetriNetBuilder();
            ColoredPetriNetBuilder(ColoredPetriNetBuilder&&) = default;
            ColoredPetriNetBuilder& operator=(const ColoredPetriNetBuilder&) = default;

            void addPlace(const std::string& name, uint32_t tokens, double, double) override;
            void addPlace(const std::string& name,
                const Colored::ColorType* type,
                Colored::Multiset&& tokens,
                double,
                double);
            void addTransition(const std::string& name, int32_t, double, double) override;
            void addInputArc(const std::string& place, const std::string& transition, bool inhibitor, uint32_t weight) override;
            void addOutputArc(const std::string& transition, const std::string& place, uint32_t weight) override;
            void addColorType(const std::string& id, const Colored::ColorType* type) override;
            void addVariable(const PetriEngine::Colored::Variable* variable) override;
            void sort() override;

            ColoredPetriNet build();
        private:
            std::unordered_map<std::string, uint32_t> _placeIndices;
            std::unordered_map<std::string, uint32_t> _transitionIndices;
            std::unordered_map<std::string, std::string> _variableTypes;
            std::unordered_map<std::string, std::shared_ptr<ColorType>> _colorTypeMap;
            std::unordered_map<std::string, std::shared_ptr<BaseColorType>> _baseColorType;
            std::unordered_map<std::string, std::vector<std::string>> _productTypes;
            std::vector<UnresolvedInputArc> _unresolvedInputArcs;
            std::vector<UnresolvedOutputArc> _unresolvedOutputArcs;
            std::vector<UnresolvedInhibitorArc> _unresolvedInhibitorArcs;
            ColoredPetriNet _currentNet;
            std::shared_ptr<ColorType> _dotColorType;
            std::shared_ptr<Colored::ColorTypeMap> _colors;
        };
    }
}


#endif