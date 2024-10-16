#ifndef COLOREDSUCCESSORGENERATOR_CPP
#define COLOREDSUCCESSORGENERATOR_CPP

#include "ExplicitColored/ColoredSuccessorGenerator.h"
#include <set>
namespace PetriEngine{
    namespace ExplicitColored{
        ColoredSuccessorGenerator::ColoredSuccessorGenerator(const ColoredPetriNet& net)
        : _net(net), _t_counter(0),_ingoing(std::vector<std::unique_ptr<ColoredPetriNetArc>>{}) {
        }

        ColoredSuccessorGenerator::~ColoredSuccessorGenerator(){
        }

        void ColoredSuccessorGenerator::reset(){
            _t_counter = 0;
        }

        std::vector<Binding> ColoredSuccessorGenerator::checkPreset(ColoredPetriNetMarking& state, uint32_t tid){
            auto& const arcs = _net._placeToTransitionArcs;
            auto relevantColors = std::vector<std::shared_ptr<ColorType>>{};
            auto ingoing = std::vector<std::unique_ptr<ColoredPetriNetArc>>();
            auto bindings = std::vector<Binding>{};
            for (auto &&a : arcs){
                if (a.to == tid){
                    ingoing.push_back(std::make_unique<ColoredPetriNetArc>(a));
                }
            }
            _ingoing = ingoing;
            //Get variables we need bindings for
            auto variables = std::vector<std::shared_ptr<Variable>>{};
            for (auto&& elm : ingoing[0]->arcExpression->variables){
                if (std::find(variables.begin(),variables.end(), elm) == variables.end()){
                    variables.push_back(elm);
                };
            }
            for (auto&& elm : _net._transitionToPlaceArcs[tid].arcExpression->variables){
                 if (std::find(variables.begin(),variables.end(), elm) == variables.end()){
                    variables.push_back(elm);
                };
            }
            //Generate every possible binding
            auto bindingVector = std::vector<std::vector<Color_t>>{{}};
            for (auto&& v : variables){
               bindingVector = combineTwo(bindingVector, v->colorType->colors);
            }

            auto bindings = convertVectorToBindings(bindingVector, variables);
            auto acceptableBindings = std::vector<Binding>{};
            for (auto&& b : bindings){
                bool fireable = _net._transitions[tid].guardExpression->eval(b);
                if (!fireable){
                    continue;
                }
                for (auto&& a : ingoing){
                    auto place = a->from;
                    if (a->inhib > 0){
                        if (state.markings[place].totalCount() >= a->inhib){
                            fireable = false;
                            break;
                        }
                    }
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

        //Generate every combination that can be made from adding a range onto a vector
        std::vector<std::vector<Color_t>> combineTwo(std::vector<std::vector<Color_t>>& vecs, uint32_t n){
            auto output = std::vector<std::vector<Color_t>>{};
            auto i = 0;
            for (auto& vec : vecs){
                for (auto j = 0; j < n ; n++){
                    output[i] = vec;
                    output[i].push_back(j);
                    i++;
                }
            }
            return output;
        }

        std::vector<Binding> convertVectorToBindings(std::vector<std::vector<Color_t>>& vecs, std::vector<std::shared_ptr<Variable>> vars){
            auto res = std::vector<Binding>{};
            for (auto&& vec : vecs){
                auto map = std::map<std::string, Color_t>{};
                for (auto i = 0; i < vars.size(); i++){
                    map[vars[i]->id] = vec[i];
                }
                res.push_back(Binding{map});
            }
            return res;
        }

        void ColoredSuccessorGenerator::consumePreset(ColoredPetriNetMarking& state, uint32_t tid, Binding& b){
            for (auto&& a : _ingoing){
                if (a->inhib == 0){
                    auto p = a->from;
                    state.markings[p] -= a->arcExpression->eval(b);
                }
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