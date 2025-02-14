#ifndef COLOREDSUCCESSORGENERATOR_CPP
#define COLOREDSUCCESSORGENERATOR_CPP

#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"
#include <memory>


namespace PetriEngine::ExplicitColored{
    ColoredSuccessorGenerator::ColoredSuccessorGenerator(const ColoredPetriNet& net)
    : _net(net) {}

    void updateVariableMap(std::map<Variable_t, std::vector<uint32_t>>& map, const std::map<Variable_t, std::vector<uint32_t>>& newMap){
        for (auto&& pair : newMap){
            if (map.find(pair.first) != map.end()){
                auto& values = map[pair.first];
                auto newSet = std::vector<uint32_t>{};
                std::set_intersection(values.begin(), values.end(),
                    pair.second.begin(), pair.second.end(), std::back_inserter(newSet));
                map[pair.first] = std::move(newSet);
            }else{
                map.insert(pair);
            }
        }
    }

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

    bool ColoredSuccessorGenerator::checkPresetAndGuard(const ColoredPetriNetMarking& state, const Transition_t tid, const Binding& binding) const {
        if (_net._transitions[tid].guardExpression != nullptr && !_net._transitions[tid].guardExpression->eval(binding)){
            return false;
        }
        for (auto i = _net._transitionArcs[tid].first; i < _net._transitionArcs[tid].second; i++){
            auto& arc = _net._arcs[i];
            if (!arc.expression->isSubSet(state.markings[arc.from], binding)) {
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
            arc.expression->consume(state.markings[arc.from], binding);
        }
    }

    void ColoredSuccessorGenerator::producePostset(ColoredPetriNetMarking& state, const Transition_t tid, const Binding& binding) const {
        for (auto i = _net._transitionArcs[tid].second; i < _net._transitionArcs[tid + 1].first; i++){
            auto& arc = _net._arcs[i];
            arc.expression->produce(state.markings[arc.to], binding);
        }
    }

    void ColoredSuccessorGenerator::_fire(ColoredPetriNetMarking& state, const Transition_t tid, const Binding& binding) const{
        consumePreset(state, tid, binding);
        producePostset(state, tid, binding);
    }

    bool ColoredSuccessorGenerator::hasMinimalCardinality(const ColoredPetriNetMarking &marking, const Transition_t transition) const {
        for (auto i = _net._transitionArcs[transition].first; i < _net._transitionArcs[transition].second; i++) {
            auto& arc = _net._arcs[i];
            if (marking.markings[arc.from].totalCount() < arc.expression->getMinimalMarkingCount()) {
                return false;
            }
        }
        return true;
    }
}

#endif /* COLOREDSUCCESSORGENERATOR_CPP */