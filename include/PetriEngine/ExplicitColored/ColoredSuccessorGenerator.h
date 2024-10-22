#ifndef COLOREDSUCCESSORGENERATOR_H
#define COLOREDSUCCESSORGENERATOR_H

#include "ColoredPetriNet.h"

namespace PetriEngine{
    namespace ExplicitColored{
        class ColoredSuccessorGenerator {
        public:
            ColoredSuccessorGenerator(const ColoredPetriNet& net);
            virtual ~ColoredSuccessorGenerator();

            virtual std::vector<ColoredPetriNetMarking> next(ColoredPetriNetMarking& state){
                return _next(state, [](size_t){ return true; });
            }

            const ColoredPetriNet& net() const {
                return _net;
            }

            void reset();

            std::vector<Binding> checkPreset(ColoredPetriNetMarking& state, uint32_t tid);

            void consumePreset(ColoredPetriNetMarking& state, uint32_t tid, Binding& b);
            void producePostset(ColoredPetriNetMarking& state, uint32_t tid, Binding& b);
        protected:
            const ColoredPetriNet& _net;
            uint32_t _t_counter = 0;
            std::vector<const ColoredPetriNetArc*> _ingoing;

            void _fire(ColoredPetriNetMarking& state, uint32_t tid, Binding& b);

            template<typename T>
            std::vector<ColoredPetriNetMarking> _next(ColoredPetriNetMarking& state, T&& predicate){
                auto res = std::vector<ColoredPetriNetMarking>{};
                for (uint32_t i = _t_counter; i < _net._ntransitions; i++){
                    auto bindings = checkPreset(state, i);
                    if (!bindings.empty()){
                        for (auto&& b : bindings){
                            auto newState = state;
                            _fire(newState, i, b);
                            res.push_back(newState);
                            _t_counter = i + 1;
                        }
                        return res;
                    }
                    _ingoing.clear();
                }
                return res;
            }
        };
    }
};
#endif /* COLOREDSUCCESSORGENERATOR_H */