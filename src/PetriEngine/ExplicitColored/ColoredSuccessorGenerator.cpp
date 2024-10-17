#ifndef COLOREDSUCCESSORGENERATOR_CPP
#define COLOREDSUCCESSORGENERATOR_CPP

#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"
#include <set>
#include <memory>

namespace PetriEngine{
    namespace ExplicitColored{
        ColoredSuccessorGenerator::ColoredSuccessorGenerator(const ColoredPetriNet& net)
        : _net(net), _t_counter(0),_ingoing(std::vector<const ColoredPetriNetArc*>{}) {
        }

        ColoredSuccessorGenerator::~ColoredSuccessorGenerator(){
        }

        void ColoredSuccessorGenerator::reset(){
            _t_counter = 0;
        }

        //Generate every combination that can be made from adding a range onto a vector
        std::vector<std::vector<Color_t>> combineTwo(std::vector<std::vector<Color_t>>& vecs, uint32_t n){
            auto output = std::vector<std::vector<Color_t>>{};
            auto i = 0;
            for (auto& vec : vecs){
                for (uint32_t j = 0; j < n; j++){
                    output[i] = vec;
                    output[i].push_back(j);
                    i++;
                }
            }
            return output;
        }

        std::vector<Binding> convertVectorToBindings(const std::vector<std::vector<Color_t>>& vecs, const std::vector<Variable_t>& vars){
            auto res = std::vector<Binding>{};
            for (auto&& vec : vecs){
                auto map = std::map<Variable_t, Color_t>{};
                for (uint32_t i = 0; i < vars.size(); i++){
                    map[vars[i]] = vec[i];
                }
                res.push_back(Binding{map});
            }
            return res;
        }

        std::vector<Binding> ColoredSuccessorGenerator::checkPreset(ColoredPetriNetMarking& state, uint32_t tid){
            auto relevantColors = std::vector<std::shared_ptr<ColorType>>{};
            auto inhibitors = std::vector<ColoredPetriNetInhibitor&>{};
            auto bindings = std::vector<Binding>{};

            for (auto && i : _net._inhibitorToPlaceArcs){
                if (i.to == tid){
                    if (state.markings[i.from].totalCount() >= i.weight){
                        return bindings;
                    }
                    break;
                }
            }

            for (auto&& a : _net._placeToTransitionArcs){
                if (a.to == tid){
                    _ingoing.push_back(&a);
                }
            }

            //Get variables we need bindings for
            //Slightly more efficient to split ingoing/outgoing to when outgoing is needed
            auto variables = std::vector<uint32_t>{};
            for (auto && a : _ingoing){
                for (auto&& v : a->variables){
                    if (std::find(variables.begin(),variables.end(), v) == variables.end()){
                        variables.push_back(v);
                    };
                }
            }

            for (auto&& a : _net._transitionToPlaceArcs){
                if (a.from == tid){
                    for (auto&& v : a.variables){
                        if (std::find(variables.begin(),variables.end(), v) == variables.end()){
                            variables.push_back(v);
                        };
                    }
                }
            }

            //Generate every possible binding
            auto bindingVector = std::vector<std::vector<Color_t>>{{}};
            for (auto&& v : variables){
               bindingVector = combineTwo(bindingVector, _net._variables[v].colorType->colors);
            }

            bindings = convertVectorToBindings(bindingVector, variables);
            auto acceptableBindings = std::vector<Binding>{};
            for (auto&& b : bindings){
                bool fireable = _net._transitions[tid].guardExpression->eval(b);
                if (!fireable){
                    continue;
                }
                for (auto&& a : _ingoing){
                    auto place = a->from;
                    if (state.markings[place] >= a->arcExpression->eval(b)){
                        fireable = false;
                        break;
                    }
                }
                if (fireable){
                    acceptableBindings.push_back(b);
                }
            }
            return acceptableBindings;
        }

        void ColoredSuccessorGenerator::consumePreset(ColoredPetriNetMarking& state, uint32_t tid, Binding& b){
            for (auto&& a : _ingoing){
                auto p = a->from;
                state.markings[p] -= a->arcExpression->eval(b);
            }
        }

        void ColoredSuccessorGenerator::producePostset(ColoredPetriNetMarking& state, uint32_t tid, Binding& b){
            for (auto &&a : _net._transitionToPlaceArcs){
                if (a.from == tid){
                    state.markings[a.to] += a.arcExpression->eval(b);
                }
            }
        }

        void ColoredSuccessorGenerator::_fire(ColoredPetriNetMarking& state, uint32_t tid, Binding& b){
            consumePreset(state, tid, b);
            producePostset(state, tid, b);
        }
    }
}
#endif /* COLOREDSUCCESSORGENERATOR_CPP */