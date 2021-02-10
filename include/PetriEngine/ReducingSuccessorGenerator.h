#ifndef PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_
#define PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_

#include "SuccessorGenerator.h"
#include "Structures/State.h"
#include "Structures/light_deque.h"
#include "PQL/PQL.h"
#include <memory>
#include "PetriEngine/Stubborn/StubbornSet.h"

namespace PetriEngine {

    class ReducingSuccessorGenerator : public SuccessorGenerator {

    public:
        ReducingSuccessorGenerator(const PetriNet &net, std::shared_ptr<StubbornSet> stubbornSet);

        void reset();

        void setQuery(PQL::Condition *ptr) { _stubSet->setQuery(ptr); }

        void prepare(const Structures::State *state);

        bool next(Structures::State &write);

        auto fired() const { return _current; }

    private:
        std::shared_ptr<StubbornSet> _stubSet;
        uint32_t _current;

        //std::vector<PQL::Condition *> _queries;

    };
}

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
