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
            if (_ingoing.find(tid) != _ingoing.end()){
                return;
            }
            auto variableMap = std::map<Variable_t, std::vector<uint32_t>>{};

            auto ingoing = std::vector<const ColoredPetriNetArc*>{};
            for (auto&& arc : _net._inputArcs) {
                if (arc.to == tid){
                    ingoing.push_back(&arc);
                }
            }

            //Gets possible values of all variables, will overestimate possible values
            for (auto && arc : ingoing){
                updateVariableMap(variableMap, arc->validVariables);
            }

            for (auto&& arc : _net._outputArcs){
                if (arc.from == tid){
                   updateVariableMap(variableMap, arc.validVariables);
                }
            }

            updateVariableMap(variableMap, _net._transitions[tid].validVariables);
            uint32_t totalSize = variableMap.empty() ? 0 : 1;
            for (auto&& var : variableMap){
                totalSize *= var.second.size();
            }
            _variables[tid] = TransitionVariables{std::move(variableMap),totalSize};
            _ingoing[tid] = std::move(ingoing);
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
            return std::all_of(_ingoing[tid].begin(), _ingoing[tid].end(),[&](auto& arc){
                return state.markings[arc->from] >= arc->arcExpression->eval(binding);
            });
        }

        void ColoredSuccessorGenerator::consumePreset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding){
            for (auto&& arc : _ingoing[tid]){
                auto place = arc->from;
                state.markings[place] -= arc->arcExpression->eval(binding);
            }
        }

        void ColoredSuccessorGenerator::producePostset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding){
            for (auto &&arc : _net._outputArcs){
                if (arc.from == tid){
                    state.markings[arc.to] += arc.arcExpression->eval(binding);
                }
            }
        }

        void ColoredSuccessorGenerator::_fire(ColoredPetriNetMarking& state, Transition_t tid, Binding& b){
            consumePreset(state, tid, b);
            producePostset(state, tid, b);
        }
    }
}
#endif /* COLOREDSUCCESSORGENERATOR_CPP */