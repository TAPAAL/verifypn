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

        void ColoredSuccessorGenerator::getVariables(Transition_t tid){
            if (_variables.find(tid) != _variables.end()){
                return;
            }
            auto variableMap = std::map<Variable_t, std::vector<uint32_t>>{};

            //Gets possible values of all variables, will overestimate possible values
            for (auto i = _net._transitionArcs[tid].first; i < _net._transitionArcs[tid + 1].first; i++){
                updateVariableMap(variableMap, _net._invariants[i]->validVariables);
            }

            updateVariableMap(variableMap, _net._transitions[tid].validVariables);
            uint32_t totalSize = variableMap.empty() ? 0 : 1;//Gets possible values of all variables, will overestimate possible values
            for (auto&& var : variableMap){
                totalSize *= var.second.size();
            }
            _variables[tid] = TransitionVariables{std::move(variableMap),totalSize};
        }

        Binding ColoredSuccessorGenerator::getBinding(Transition_t tid, uint32_t bid){
            auto map = std::map<Variable_t, Color_t>{};
            if (_variables.find(tid) != _variables.end() && _variables[tid].totalBindings != 0){
                const auto& variables = _variables[tid];
                uint32_t interval = variables.totalBindings;
                for (const auto& pair : variables.possibleValues){
                    auto size = pair.second.size();
                    interval /= size;
                    map.emplace(pair.first, pair.second.at((bid / interval) % size));
                }
            }
            return Binding{map};
        }

        bool ColoredSuccessorGenerator::checkPreset(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding){
            for (auto&& inhib : _net._inhibitorArcs){
                if (inhib.to == tid){
                    if (inhib.weight <= state.markings[inhib.from].totalCount()){
                        return false;
                    }
                    break;
                }
            }
            if (_net._transitions[tid].guardExpression != nullptr && !_net._transitions[tid].guardExpression->eval(binding)){
                return false;
            }
            for (auto i = _net._transitionArcs[tid].first; i < _net._transitionArcs[tid].second; i++){
                auto& arc = _net._invariants[i];
                if (!(state.markings[arc->from] >= arc->arcExpression->eval(binding))){
                    return false;
                }
            }
            return true;
        }

        void ColoredSuccessorGenerator::consumePreset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding){
            for (auto i = _net._transitionArcs[tid].first; i < _net._transitionArcs[tid].second; i++){
                auto& arc = _net._invariants[i];
                auto place = arc->from;
                state.markings[place] -= arc->arcExpression->eval(binding);
            }
        }

        void ColoredSuccessorGenerator::producePostset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding){
            for (auto i = _net._transitionArcs[tid].second; i < _net._transitionArcs[tid + 1].first; i++){
                auto& arc = _net._invariants[i];
                state.markings[arc->to] += arc->arcExpression->eval(binding);
            }
        }

        void ColoredSuccessorGenerator::_fire(ColoredPetriNetMarking& state, Transition_t tid, const Binding& b){
            consumePreset(state, tid, b);
            producePostset(state, tid, b);
        }
    }
}
#endif /* COLOREDSUCCESSORGENERATOR_CPP */