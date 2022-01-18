

#include "PetriEngine/Synthesis/SimpleSynthesis.h"
#include "PetriEngine/Synthesis/GamePORSuccessorGenerator.h"
#include "PetriEngine/Synthesis/GameStubbornSet.h"

#include <assert.h>
#include <stack>
#include <unistd.h>

namespace PetriEngine {
    namespace Synthesis {

        GamePORSuccessorGenerator::GamePORSuccessorGenerator(const PetriNet& net, PQL::Condition* predicate, bool is_safe)
        : GameSuccessorGenerator(net), _stubborn(net, predicate, is_safe), _predicate(predicate), _is_safety(is_safe) {
        }

        bool GamePORSuccessorGenerator::prepare(const Structures::State* state) {
            GameSuccessorGenerator::prepare(state);
#ifndef NDEBUG
            auto r =
#endif
            _stubborn.prepare(state);
            assert(r);
            return true;
        }

        bool GamePORSuccessorGenerator::_nxt(Structures::State& write, bool ctrl) {
            _last_fired = ctrl ? _stubborn.next_ctrl() : _stubborn.next_env();
            if(_last_fired != std::numeric_limits<uint32_t>::max())
            {
                GamePORSuccessorGenerator::_fire(write, _last_fired);
                return true;
            }
            else return false;
        }

        bool GamePORSuccessorGenerator::next_ctrl(Structures::State& write) {
            return _nxt(write, true);
        }

        bool GamePORSuccessorGenerator::next_env(Structures::State& write) {
            return _nxt(write, false);
        }

    }
}
