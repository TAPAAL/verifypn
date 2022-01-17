/*
 * File:   GameSucessorGenerator.h
 * Author: Peter G. Jensen
 *
 * Created on 13 January 2022, 16.36
 */

#ifndef GAMESUCESSORGENERATOR_H
#define GAMESUCESSORGENERATOR_H

#include "PetriEngine/SuccessorGenerator.h"

#include <assert.h>
#include <stack>
#include <unistd.h>

namespace PetriEngine {
    namespace Synthesis {
        class GameSuccessorGenerator : protected SuccessorGenerator {
        public:
            GameSuccessorGenerator(const PetriNet& net);

            /**
             * To be honest, we should re-index things here for a better/faster
             * successor generation. In particular split controllable and
             * uncontrollable transitions into two different generators.
             */
            virtual bool next_ctrl(Structures::State& write)
            {
                if(_last_mode == ENV)
                    reset();
                _last_mode = CTRL;
                return _next(write, [this](size_t t){ return _net.controllable(t); });
            }

            virtual bool next_env(Structures::State& write)
            {
                if(_last_mode == CTRL)
                    reset();
                _last_mode = ENV;
                return _next(write, [this](size_t t){ return !_net.controllable(t); });
            }

            virtual bool prepare(const Structures::State* state);
            bool prepare(const Structures::State& state) final { return prepare(&state); }

            using SuccessorGenerator::fired;

        protected:
            enum mode_t { NONE, CTRL, ENV };

            mode_t _last_mode = NONE;

            using SuccessorGenerator::_fire;
        };
    }
}


#endif /* GAMESUCESSORGENERATOR_H */

