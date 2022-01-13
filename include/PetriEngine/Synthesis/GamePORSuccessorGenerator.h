#ifndef GAMEPORSUCCESSORGENERATOR_H_
#define GAMEPORSUCCESSORGENERATOR_H_

#include "GameStubbornSet.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Contexts.h"
#include "utils/structures/light_deque.h"
#include "GameSuccessorGenerator.h"

#include <memory>
#include <stack>
namespace PetriEngine {
namespace Synthesis {
    class GamePORSuccessorGenerator : public GameSuccessorGenerator {
    public:
        GamePORSuccessorGenerator(const PetriNet& net, PQL::Condition* predicate, bool is_safe);
        virtual ~GamePORSuccessorGenerator() = default;

        virtual bool prepare(const Structures::State* state);
        virtual bool next_ctrl(Structures::State& write);
        virtual bool next_env(Structures::State& write);
        virtual uint32_t fired() const { return _last_fired; }
    private:

        bool _nxt(Structures::State& write, bool ctrl);

        GameStubbornSet _stubborn;
        PQL::Condition* _predicate = nullptr;
        const bool _is_safety = false;
        uint32_t _last_fired;
    };
}
}

#endif /* GAMEPORSUCCESSORGENERATOR_H_ */
