#ifndef COLOREDSUCCESSORGENERATOR_CPP
#define COLOREDSUCCESSORGENERATOR_CPP

#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"
#include <set>
#include <memory>

namespace PetriEngine{
    namespace ExplicitColored{
        ColoredSuccessorGenerator::ColoredSuccessorGenerator(const ColoredPetriNet& net)
        : _net(net) {}

        ColoredSuccessorGenerator::~ColoredSuccessorGenerator() = default;

        Binding ColoredSuccessorGenerator::getBinding(const Transition_t tid, const Binding_t bid) const {
            auto map = std::map<Variable_t, Color_t>{};
            const auto& possibleValues = _net._transitions[tid].validVariables.second;
            if (possibleValues != 0){
                auto& variables = _net._transitions[tid].validVariables.first;
                auto interval = possibleValues;
                for (const auto&[varName, varValues] : variables){
                    const auto size = varValues.size();
                    interval /= size;
                    map.emplace(varName, varValues.at((bid / interval) % size));
                }
            }
            return Binding{std::move(map)};
        }

        bool ColoredSuccessorGenerator::check(const ColoredPetriNetMarking& state, const Transition_t tid, const Binding& binding) const{
            return checkInhibitor(state, tid) && checkPresetAndGuard(state, tid, binding);
        }
        CheckingBool ColoredSuccessorGenerator::firstCheckPresetAndGuard(const ColoredPetriNetMarking& state, const Transition_t tid, const Binding& binding) const {
            if (_net._transitions[tid].guardExpression != nullptr && !_net._transitions[tid].guardExpression->eval(binding)){
                if (_net._transitions[tid].guardExpression->getVariables().empty()){
                    return CheckingBool::NEVERTRUE;
                }
                return CheckingBool::FALSE;
            }
            for (auto i = _net._transitionArcs[tid].first; i < _net._transitionArcs[tid].second; i++){
                auto& arc = _net._arcs[i];
                auto arcExpr = arc.arcExpression->eval(binding);
                if (!(state.markings[arc.from] >= arcExpr)){
                    if (state.getPlaceCount(arc.from) < arcExpr.totalCount()){
                        return CheckingBool::NEVERTRUE;
                    }
                    return CheckingBool::FALSE;
                }
            }
            return CheckingBool::TRUE;
        }

        bool ColoredSuccessorGenerator::checkPresetAndGuard(const ColoredPetriNetMarking& state, const Transition_t tid, const Binding& binding) const {
            if (_net._transitions[tid].guardExpression != nullptr && !_net._transitions[tid].guardExpression->eval(binding)){
                return false;
            }
            for (auto i = _net._transitionArcs[tid].first; i < _net._transitionArcs[tid].second; i++){
                auto& arc = _net._arcs[i];
                if (!(state.markings[arc.from] >= arc.arcExpression->eval(binding))){
                    return false;
                }
            }
            return true;
        }

        bool ColoredSuccessorGenerator::checkInhibitor(const ColoredPetriNetMarking& state, const Transition_t tid) const {
            for (size_t i = _net._transitionInhibitors[tid]; i < _net._transitionInhibitors[tid + 1]; i++){
                auto& inhib = _net._inhibitorArcs[i];
                if (inhib.weight <= state.markings[inhib.from].totalCount()){
                    return false;
                }
            }
            return true;
        }

        void ColoredSuccessorGenerator::consumePreset(ColoredPetriNetMarking& state, const Transition_t tid, const Binding& binding) const {
            for (auto i = _net._transitionArcs[tid].first; i < _net._transitionArcs[tid].second; i++){
                auto& arc = _net._arcs[i];
                state.markings[arc.from] -= arc.arcExpression->eval(binding);
            }
        }

        void ColoredSuccessorGenerator::producePostset(ColoredPetriNetMarking& state, const Transition_t tid, const Binding& binding) const {
            for (auto i = _net._transitionArcs[tid].second; i < _net._transitionArcs[tid + 1].first; i++){
                auto& arc = _net._arcs[i];
                state.markings[arc.to] += arc.arcExpression->eval(binding);
            }
        }

        void ColoredSuccessorGenerator::_fire(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const{
            consumePreset(state, tid, binding);
            producePostset(state, tid, binding);
        }
    }
}
#endif /* COLOREDSUCCESSORGENERATOR_CPP */