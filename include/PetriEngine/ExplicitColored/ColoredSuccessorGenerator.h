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
            std::vector<std::unique_ptr<ColoredPetriNetArc>> _ingoing;
            uint32_t _t_counter;

            void _fire(ColoredPetriNetMarking& state, uint32_t tid, Binding& b);

            template<typename T>
            std::vector<ColoredPetriNetMarking> _next(ColoredPetriNetMarking& state, T&& predicate){
                auto res = std::vector<ColoredPetriNetMarking>{};
                for (auto i = _t_counter; i < ntransitions; i++){
                    if (auto bindings = checkPreset(i); !bindings.empty()){
                        for (auto&& b : bindings){
                            auto newState = state;
                            _fire(newState, tid, b);
                            res.push_back(newState);
                            _t_counter = i + 1;
                        }
                        return res;
                    }
                }
                return res;
            }
        };
    }
};
#endif /* COLOREDSUCCESSORGENERATOR_H */