#ifndef COLOREDSUCCESSORGENERATOR_CPP
#define COLOREDSUCCESSORGENERATOR_CPP

#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"
#include <set>
#include <memory>

namespace PetriEngine{
    namespace ExplicitColored{
        ColoredSuccessorGenerator::ColoredSuccessorGenerator(const ColoredPetriNet& net)
        : _net(net) {
        }

        ColoredSuccessorGenerator::~ColoredSuccessorGenerator(){
        }

        void updateVariableMap(std::map<Variable_t, std::vector<uint32_t>>& map, const std::map<Variable_t, std::vector<uint32_t>>& newMap){
            for (auto&& pair : newMap){
                if (map.find(pair.first) != map.end()){
                    auto& values = map[pair.first];
                    auto newSet = std::vector<uint32_t>{};
                    std::set_intersection(values.begin(), values.end(),
                        pair.second.begin(), pair.second.end(), newSet.begin());
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
            uint32_t totalSize = variableMap.size() != 0 ? 1 : 0;
            for (auto&& var : variableMap){
                totalSize *= var.second.size();
            }
            std::cout << variableMap.size() << std::endl;
            std::cout << ingoing.size() << std::endl;
            _variables[tid] = TransitionVariables{std::move(variableMap),totalSize};
            _ingoing[tid] = std::move(ingoing);
        }

        Binding ColoredSuccessorGenerator::getBinding(Transition_t tid, uint32_t bid){
            auto map = std::map<Variable_t, Color_t>{};
            std::cout << "Getting binding" << std::endl;
            if (_variables.find(tid) != _variables.end() && _variables[tid].totalBindings != 0){
                std::cout << "Getting binding2" << std::endl;
                auto variables = _variables[tid];
                std::cout << variables.possibleValues.size() << " " << variables.totalBindings << std::endl;
                uint32_t interval = variables.totalBindings;
                for (size_t i = 0; i < variables.possibleValues.size(); i++){
                    auto size = variables.possibleValues[i].size();
                    interval /= size;
                    map.emplace(i, variables.possibleValues[i][(bid / interval) % size]);
                }
            }
            std::cout << "Got binding" << std::endl;
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
//            for (auto&& arc : _ingoing[tid]){
//                if (!(state.markings[arc->from] >= arc->arcExpression->eval(binding))){
//                    return false;
//                }
//            }
//            return true;
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