
#include "PetriEngine/Synthesis/GameSuccessorGenerator.h"

namespace PetriEngine {
    namespace Synthesis {
        GameSuccessorGenerator::GameSuccessorGenerator(const PetriNet& net)
            : SuccessorGenerator(net) {
        }

        bool GameSuccessorGenerator::prepare(const Structures::State* state) {
            _last_mode = NONE;
            return SuccessorGenerator::prepare(state);
        }


    }
}