#include "PetriEngine/ExplicitColored/ArcExpression.h"
#include "PetriEngine/ExplicitColored/VariableExtractorVisitor.h"
#include "PetriEngine/ExplicitColored/CompiledArcExpression.hpp"
namespace PetriEngine {
    namespace ExplicitColored {
        ArcExpression::ArcExpression(const Colored::ArcExpression_ptr& arcExpression, const Colored::ColorTypeMap& colorTypeMap, const std::unordered_map<std::string, Variable_t>& variableMap)
            : _compiledArc(arcExpression, colorTypeMap, variableMap) {
            VariableExtractorVisitor variableExtractor(variableMap);
            arcExpression->visit(variableExtractor);
            _variables = std::move(variableExtractor.collectedVariables);
        }

        CPNMultiSet ArcExpression::eval(const Binding& binding) const {
            return _compiledArc.eval(binding);
        }

        void ArcExpression::addToExisting(CPNMultiSet &existing, const Binding &binding) const {
            _compiledArc.addToExisting(existing, binding);
        }

        void ArcExpression::subFromExisting(CPNMultiSet &existing, const Binding &binding) const {
            _compiledArc.subFromExisting(existing, binding);
        }

        bool ArcExpression::isSubSet(const CPNMultiSet &superset, const Binding &binding) const {
            return _compiledArc.isSubSet(superset, binding);
        }

        const std::set<Variable_t>& ArcExpression::getVariables() const {
            return _variables;
        }
    }
}