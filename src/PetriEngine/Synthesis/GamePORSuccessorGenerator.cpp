

#include "PetriEngine/Synthesis/SimpleSynthesis.h"
#include "PetriEngine/Synthesis/GamePORSuccessorGenerator.h"
#include "PetriEngine/Synthesis/GameStubbornSet.h"

#include <assert.h>
#include <stack>
#include <unistd.h>

namespace PetriEngine {
    namespace Synthesis {

        GamePORSuccessorGenerator::GamePORSuccessorGenerator(const PetriNet& net, PQL::Condition* predicate, bool is_safe)
        : SuccessorGenerator(net), _predicate(predicate), _is_safety(is_safe), _stubborn(_net, _predicate) {
        }

        bool GamePORSuccessorGenerator::prepare(const Structures::State* state) {
            SuccessorGenerator::prepare(state);
            _skip = !_stubborn.prepare(state);
            if(_skip)
                return true;
            return true;
        }

        bool GamePORSuccessorGenerator::next(Structures::State& write)
        {
            throw base_error("ERROR: next should never be called for games.");
        }

        bool GamePORSuccessorGenerator::next_ctrl(Structures::State& write) {
            if(_skip)
                return SuccessorGenerator::next_ctrl(write);
            return false;
        }

        bool GamePORSuccessorGenerator::next_env(Structures::State& write) {
            if(_skip)
                return SuccessorGenerator::next_env(write);
            return false;
        }

    }
}
