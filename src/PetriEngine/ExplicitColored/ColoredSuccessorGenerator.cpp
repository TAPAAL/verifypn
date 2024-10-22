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
            for (auto& vec : vecs){
                for (uint32_t j = 0; j < n; j++){
                    auto copy = vec;
                    copy.push_back(j);
                    output.emplace_back(std::move(copy));
                }
            }
            return output;
        }

        std::vector<Binding> convertVectorToBindings(const std::vector<std::vector<Color_t>>& vecs, const std::set<Variable_t>& vars){
            auto res = std::vector<Binding>{};
            for (auto&& vec : vecs){
                auto map = std::map<Variable_t, Color_t>{};
                size_t i = 0;
                for (const Variable_t var : vars) {
                    map[var] = vec[i++];
                }
                res.push_back(Binding{map});
            }
            return res;
        }

        std::vector<Binding> generateBindings(const std::vector<std::pair<Variable_t, const Variable&>>& variables, size_t index) {
            if (index == 0) {
                std::vector<Binding> bindings;
                for (Color_t c = 0; c < variables[index].second.colorType->colors; c++) {
                    bindings.push_back({{{variables[index].first, c}}});
                }
                return bindings;
            }
            std::vector<Binding> bindings = generateBindings(variables, index - 1);

            for (auto& binding : bindings) {
                binding.setValue(variables[index].first, 0);
            }

            size_t oldBindingCount = bindings.size();

            for (Color_t c = 1; c < variables[index].second.colorType->colors; c++) {
                for (size_t i = 0; i < oldBindingCount; i++) {
                    Binding copy = bindings[i];
                    copy.setValue(variables[index].first, c);
                    bindings.emplace_back(std::move(copy));
                }
            }
            return bindings;
        }

        std::vector<Binding> ColoredSuccessorGenerator::checkPreset(ColoredPetriNetMarking& state, uint32_t tid){
            auto relevantColors = std::vector<std::shared_ptr<ColorType>>{};
            auto bindings = std::vector<Binding>{};
            const auto& transition = _net._transitions[tid];

            for (auto && i : _net._inhibitorArcs) {
                if (i.to == tid){
                    if (state.markings[i.from].totalCount() >= i.weight) {
                        return bindings;
                    }
                    break;
                }
            }

            for (auto&& a : _net._inputArcs) {
                if (a.to == tid){
                    _ingoing.push_back(&a);
                }
            }

            //Get variables we need bindings for
            //Slightly more efficient to split ingoing/outgoing to when outgoing is needed
            auto variables = std::set<Variable_t>{};
            for (auto && a : _ingoing){
                std::set<Variable_t> vars = a->arcExpression->getVariables();
                variables.merge(vars);
            }

            for (auto&& a : _net._outputArcs){
                if (a.from == tid){
                    std::set<Variable_t> vars = a.arcExpression->getVariables();
                    variables.merge(vars);
                }
            }

            if (transition.guardExpression != nullptr) {
                std::set<Variable_t> vars = transition.guardExpression->getVariables();
                variables.merge(vars);
            }

            std::vector<std::pair<Variable_t, const Variable&>> variableList;

            for (Variable_t varId : variables) {
                variableList.emplace_back(varId, _net._variables[varId]);
            }

            if (variableList.empty()) {
                bindings = {{}};
            } else {
                bindings = generateBindings(variableList, variables.size() - 1);
            }

            auto acceptableBindings = std::vector<Binding>{};
            for (const auto& binding : bindings){
                bool fireable = transition.guardExpression == nullptr
                    || _net._transitions[tid].guardExpression->eval(binding);

                if (!fireable){
                    continue;
                }
                for (auto&& a : _ingoing){
                    auto place = a->from;
                    const auto& currentMarking = state.markings[place];
                    auto arcEvaluation = a->arcExpression->eval(binding);
                    if (!(currentMarking >= arcEvaluation)){
                        fireable = false;
                        break;
                    }
                }
                if (fireable){
                    acceptableBindings.push_back(binding);
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
            for (auto &&a : _net._outputArcs){
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