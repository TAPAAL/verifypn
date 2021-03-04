#include "PetriEngine/ReducingSuccessorGenerator.h"

#include <cassert>
#include "PetriEngine/PQL/Contexts.h"

namespace PetriEngine {

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet &net,
                                                           std::shared_ptr<StubbornSet> stubbornSet)
            : SuccessorGenerator(net), _stubSet(stubbornSet) {

    }

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet &net,
                                                           std::vector<std::shared_ptr<PQL::Condition> > &queries,
                                                           std::shared_ptr<StubbornSet> stubbornSet)
            : ReducingSuccessorGenerator(net, stubbornSet) {
        _queries.reserve(queries.size());
        for (auto &q : queries)
            _queries.push_back(q.get());
    }

    void ReducingSuccessorGenerator::reset() {
        SuccessorGenerator::reset();
        _stubSet->reset();
    }


    bool ReducingSuccessorGenerator::next(Structures::State &write) {
        _current = _stubSet->next();
        if (_current == std::numeric_limits<uint32_t>::max()) {
            reset();
            return false;
        }
        memcpy(write.marking(), (*_parent).marking(), _net._nplaces * sizeof(MarkVal));
        consumePreset(write, _current);
        producePostset(write, _current);
        return true;
    }

    void ReducingSuccessorGenerator::prepare(const Structures::State *state) {
        _parent = state;
        _stubSet->prepare(state);
    }


}
