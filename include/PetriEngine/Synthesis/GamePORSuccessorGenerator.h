#ifndef GAMEPORSUCCESSORGENERATOR_H_
#define GAMEPORSUCCESSORGENERATOR_H_

#include "PetriEngine/SuccessorGenerator.h"
#include "GameStubbornSet.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Contexts.h"
#include "utils/structures/light_deque.h"

#include <memory>
#include <stack>
namespace PetriEngine {
namespace Synthesis {
    class GamePORSuccessorGenerator : public SuccessorGenerator {
    public:
        GamePORSuccessorGenerator(const PetriNet& net, PQL::Condition* predicate, bool is_safe);
        virtual ~GamePORSuccessorGenerator() = default;

        virtual bool prepare(const Structures::State* state);
        virtual bool next(Structures::State& write);

        virtual bool next_ctrl(Structures::State& write);
        virtual bool next_env(Structures::State& write);

        uint32_t fired() const
        {
            return _last_fired;
        }

    private:
        GameStubbornSet _stubborn;
        PQL::Condition* _predicate = nullptr;
        const bool _is_safety = false;


        uint32_t _last_fired = std::numeric_limits<uint32_t>::max();
        bool _skip = false;
    };
}
}

#endif /* GAMEPORSUCCESSORGENERATOR_H_ */
