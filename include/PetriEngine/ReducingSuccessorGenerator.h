#ifndef PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_
#define PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_

#include "SuccessorGenerator.h"
#include "Structures/State.h"
#include "utils/structures/light_deque.h"
#include "PQL/PQL.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include <memory>

namespace LTL {
    class guard_info_t;
}
namespace PetriEngine {

    class ReducingSuccessorGenerator : public SuccessorGenerator {

    public:
        ReducingSuccessorGenerator(const PetriNet &net, std::shared_ptr<StubbornSet> stubbornSet);


        ReducingSuccessorGenerator(const PetriNet &net, std::vector<std::shared_ptr<PQL::Condition> > &queries,
                                   std::shared_ptr<StubbornSet> stubbornSet);

        void reset();

        void setQuery(PQL::Condition *ptr) { _stubSet->setQuery(ptr); }

        bool prepare(const Structures::State *state) override;

        bool next(Structures::State &write);

        auto fired() const { return _current; }
    private:
        std::shared_ptr<StubbornSet> _stubSet;
        uint32_t _current;

        std::vector<PQL::Condition *> _queries;
    };
}

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
