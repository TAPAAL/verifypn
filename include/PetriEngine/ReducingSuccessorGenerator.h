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

        ReducingSuccessorGenerator(const PetriNet &net, std::vector<std::shared_ptr<PQL::Condition> > &queries,
                                   std::shared_ptr<StubbornSet> stubbornSet);

        void reset();

        void setQuery(PQL::Condition *ptr) { _stubSet->setQuery(ptr); }

    private:
        std::shared_ptr<StubbornSet> _stubSet;

        std::vector<PQL::Condition *> _queries;

    };
}

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
