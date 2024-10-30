#ifndef COLOREDSUCCESSORGENERATOR_H
#define COLOREDSUCCESSORGENERATOR_H

#include "ColoredPetriNet.h"
#include "ColoredPetriNetState.h"
#include <limits>

namespace PetriEngine{
    namespace ExplicitColored{
        struct TransitionVariables{
            std::map<Variable_t, std::vector<uint32_t>> possibleValues = std::map<Variable_t, std::vector<uint32_t>>{};
            uint32_t totalBindings = 0;
        };

        class ColoredSuccessorGenerator {
        public:
            explicit ColoredSuccessorGenerator(const ColoredPetriNet& net);
            virtual ~ColoredSuccessorGenerator();

            virtual ColoredPetriNetState next(ColoredPetriNetState& state){
                return _next(state, [](size_t){ return true; });
            }

            [[nodiscard]] const ColoredPetriNet& net() const {
                return _net;
            }

            Binding getBinding(Transition_t tid, uint32_t bid);
            void getVariables(Transition_t tid);
            bool checkPreset(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding);
            void consumePreset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& b);
            void producePostset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& b);
        protected:
            const ColoredPetriNet& _net;
            std::map<Transition_t, TransitionVariables> _variables = std::map<Transition_t,TransitionVariables>{};
            std::map<Transition_t, std::vector<const ColoredPetriNetArc*>> _ingoing = std::map<Transition_t, std::vector<const ColoredPetriNetArc*>>{};
            void _fire(ColoredPetriNetMarking& state, Transition_t tid, Binding& b);

            template<typename T>
            ColoredPetriNetState _next(ColoredPetriNetState& state, T&& predicate){
                auto newState = ColoredPetriNetState{state}; // This is the issue
                for (auto tid = state.lastTrans; tid <_net._transitions.size(); tid++){
                    auto bid = state.lastBinding;
                    getVariables(tid);
                    auto totalBindings = _variables[tid].totalBindings;
                    if (totalBindings == 0){
                        auto binding = Binding{};
                        if (checkPreset(state.marking, tid, binding)){
                            _fire(newState.marking, tid, binding);
                            newState.lastBinding = 0;
                            newState.lastTrans = 0;
                            state.lastBinding = bid;
                            state.lastTrans = tid;
                            return newState;
                        }
                    }else{
                        while (++bid <= totalBindings){
                        auto binding = getBinding(tid, bid);
                        if (checkPreset(state.marking, tid, binding)){
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
                newState.lastTrans = std::numeric_limits<uint32_t>::max();
                return newState;
            }
        };
    }
};
#endif /* COLOREDSUCCESSORGENERATOR_H */