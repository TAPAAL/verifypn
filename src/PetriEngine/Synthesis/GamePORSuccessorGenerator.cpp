

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
            _skip = !_stubborn.prepare(state);
            if(_skip)
                return true;
            assert(_stubborn.has_ctrl() xor _stubborn.has_env());
            return true;
        }

        bool GamePORSuccessorGenerator::_nxt(Structures::State& write) {
            _last_fired = _stubborn.next();
            if(_last_fired != std::numeric_limits<uint32_t>::max())
            {
                GamePORSuccessorGenerator::_fire(write, _last_fired);
                return true;
            }
            else return false;
        }

        bool GamePORSuccessorGenerator::next_ctrl(Structures::State& write) {

            if(_skip || !_stubborn.has_ctrl())
            {
                auto r = GameSuccessorGenerator::next_ctrl(write);
                _last_fired = GameSuccessorGenerator::fired();
                return r;
            }
            return _nxt(write);
        }

        bool GamePORSuccessorGenerator::next_env(Structures::State& write) {
            if(_skip || !_stubborn.has_env())
            {
                auto r = GameSuccessorGenerator::next_env(write);
                _last_fired = GameSuccessorGenerator::fired();
                return r;
            }
            return _nxt(write);
        }

    }
}
