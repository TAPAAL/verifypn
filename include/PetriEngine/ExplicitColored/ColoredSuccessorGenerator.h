#ifndef COLOREDSUCCESSORGENERATOR_H
#define COLOREDSUCCESSORGENERATOR_H

#include "ColoredPetriNet.h"
#include "ColoredPetriNetState.h"
#include <limits>

namespace PetriEngine{
    namespace ExplicitColored{
        struct TransitionVariables{
            std::map<Variable_t, std::vector<uint32_t>> possibleValues;
            uint32_t totalBindings;
        };

        class ColoredSuccessorGenerator {
        public:
            ColoredSuccessorGenerator(const ColoredPetriNet& net);
            virtual ~ColoredSuccessorGenerator();

            virtual ColoredPetriNetState next(ColoredPetriNetState& state){
                return _next(state, [](size_t){ return true; });
            }

            const ColoredPetriNet& net() const {
                return _net;
            }

            void reset();

            Binding getBinding(Transition_t tid, uint32_t bid);
            void getVariables(Transition_t tid);
            bool checkPreset(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding);
            void consumePreset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& b);
            void producePostset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& b);
        protected:
            const ColoredPetriNet& _net;
            uint32_t _total_bindings = 0;
            std::map<Transition_t, TransitionVariables> _variables = std::map<Transition_t,TransitionVariables>{};
            std::map<Transition_t, std::vector<const ColoredPetriNetArc*>> _ingoing = std::map<Transition_t, std::vector<const ColoredPetriNetArc*>>{};
            void _fire(ColoredPetriNetMarking& state, Transition_t tid, Binding& b);

            template<typename T>
            ColoredPetriNetState _next(ColoredPetriNetState state, T&& predicate){
                auto newState = state;
                for (auto tid = state.lastTrans; tid < _net._ntransitions; tid++){
                    auto bid = state.lastBinding;
                    std::cout << "getting" << std::endl;
                    getVariables(tid);
                    std::cout << "got" << std::endl;
                    auto totalBindings = _variables[tid].totalBindings;
                    std::cout << "Total bindings" << totalBindings << std::endl;
                    if (totalBindings == 0){
                        auto binding = Binding{};
                        std::cout << "checking" << std::endl;
                        if (checkPreset(state.marking, state.lastTrans, binding)){
                            std::cout << "firing" << std::endl;
                            _fire(newState.marking, tid, binding);
                            newState.lastBinding = 0;
                            newState.lastTrans = 0;
                            state.lastBinding = bid;
                            state.lastTrans = tid;
                            return newState;
                        }
                    }else{
                        while (++bid <= totalBindings){
                        std::cout << bid << std::endl;
                        auto binding = getBinding(tid, bid);
                        std::cout << "checking" << std::endl;
                        if (checkPreset(state.marking, state.lastTrans, binding)){
                            std::cout << "firing" << std::endl;
                            _fire(newState.marking, tid, binding);
                            newState.lastBinding = 0;
                            newState.lastTrans = 0;
                            state.lastBinding = bid;
                            state.lastTrans = tid;
                            return newState;
                        }
                    }
                    }
                }
                std::cout << "returning" << std::endl;
                newState.lastTrans = std::numeric_limits<uint32_t>::max();
                return newState;
            }
        };
    }
};
#endif /* COLOREDSUCCESSORGENERATOR_H */