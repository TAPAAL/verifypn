#ifndef VERIFYPN_VALIDVARIABLEGENERATOR_H
#define VERIFYPN_VALIDVARIABLEGENERATOR_H

#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"

namespace PetriEngine::ExplicitColored {
    enum class ValidVariableGeneratorStatus {
        OK,
        TOO_MANY_BINDINGS
    };

    class ValidVariableGenerator {
    public:
        explicit ValidVariableGenerator(ColoredPetriNet& net) : _net(net) {}
        ValidVariableGeneratorStatus generateValidColorsForTransitions() {
            for (size_t tid = 0; tid < _net._transitionArcs.size() - 1; tid++) {
                auto variableMap = std::map<Variable_t, std::vector<uint32_t>>{};
                //Guard valid variables
                _updateVariableMap(variableMap,_net._transitions[tid]);
                //Arc valid variables
                for (auto aid = _net._transitionArcs[tid].first; aid < _net._transitionArcs[tid + 1].first; aid++) {
                    _updateVariableMap(variableMap, _net._arcs[aid]);
                }

                uint64_t totalSize = variableMap.empty() ? 0 : 1;//Gets possible values of all variables, will overestimate possible values
                for (auto&[fst, snd] : variableMap){
                    if (totalSize > std::numeric_limits<uint64_t>::max() / snd.size()) {
                        return ValidVariableGeneratorStatus::TOO_MANY_BINDINGS;
                    }
                    totalSize *= snd.size();
                }
                _net._transitions[tid].validVariables = std::pair(std::move(variableMap),totalSize);
            }
            return ValidVariableGeneratorStatus::OK;
        }
    private:
        ColoredPetriNet &_net;
        void _updateVariableMap(std::map<Variable_t, std::vector<uint32_t>> &map, const ColoredPetriNetArc &arc) {
            std::map<Variable_t, std::vector<uint32_t>> newMap = std::map<Variable_t, std::vector<uint32_t>>{};
            auto vars = arc.expression->getVariables();
            for (auto &&var: vars) {
                auto nValues = _net._variables[var].colorType->colors;
                std::vector<uint32_t> values = std::vector<uint32_t>{};
                for (uint32_t i = 0; i < nValues; i++) {
                    values.push_back(i);
                }
                newMap.emplace(var, values);
            }
            _updateVariableMap(map, newMap);
        }

        void _updateVariableMap(std::map<Variable_t, std::vector<uint32_t>> &map, const ColoredPetriNetTransition &transition) {
            std::map<Variable_t, std::vector<uint32_t>> newMap = std::map<Variable_t, std::vector<uint32_t>>{};
            if (transition.guardExpression == nullptr){
                return;
            }
            auto vars = transition.variables;
            for (auto &&var: vars) {
                auto nValues = _net._variables[var].colorType->colors;
                std::vector<uint32_t> values = std::vector<uint32_t>(nValues);
                for (uint32_t i = 0; i < nValues; i++) {
                    values.push_back(i);
                }
                newMap.emplace(var, values);
            }
            _updateVariableMap(map, newMap);
        }

        static void _updateVariableMap(std::map<Variable_t, std::vector<uint32_t>> &map, const std::map<Variable_t, std::vector<uint32_t>>& newMap) {
            for (auto &&pair: newMap) {
                if (map.find(pair.first) != map.end()) {
                    auto &values = map[pair.first];
                    auto newSet = std::vector<uint32_t>{};
                    std::set_intersection(values.begin(), values.end(),
                                          pair.second.begin(), pair.second.end(), std::back_inserter(newSet));
                    map[pair.first] = std::move(newSet);
                } else {
                    map.insert(pair);
                }
            }
        }
    };
}

#endif //VERIFYPN_VALIDVARIABLEGENERATOR_H
