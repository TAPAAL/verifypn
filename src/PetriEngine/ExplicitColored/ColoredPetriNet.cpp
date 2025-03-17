#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
namespace PetriEngine::ExplicitColored {
    void ColoredPetriNet::extractOutputVariables(const Transition_t transition, std::set<Variable_t>& out) const {
        for (auto i = _transitionArcs[transition].second; i < _transitionArcs[transition + 1].first; i++){
            const auto& vars = _arcs[i].expression->getVariables();
            out.insert(vars.begin(), vars.end());
        }
    }

    void ColoredPetriNet::extractGuardVariables(const Transition_t transition, std::set<Variable_t>& out) const {
        if (_transitions[transition].guardExpression == nullptr) {
            return;
        }
        _transitions[transition].guardExpression->collectVariables(out);
    }

    void ColoredPetriNet::extractInputVariables(const Transition_t transition, std::set<Variable_t>& out) const {
        for (auto i = _transitionArcs[transition].first; i < _transitionArcs[transition].second; i++) {
            const auto& vars = _arcs[i].expression->getVariables();
            out.insert(vars.begin(), vars.end());
        }
    }

    const std::set<Variable_t>& ColoredPetriNet::getAllTransitionVariables(const Transition_t transition) const {
        return _transitions[transition].variables;
    }
}