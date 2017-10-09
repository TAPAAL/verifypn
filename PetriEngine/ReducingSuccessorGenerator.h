#ifndef PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_
#define PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_

#include "SuccessorGenerator.h"
#include "Structures/State.h"
#include "Structures/light_deque.h"
#include "PQL/PQL.h"
#include <memory>

namespace PetriEngine {

class ReducingSuccessorGenerator : public SuccessorGenerator {

public:
    struct place_t {
        uint32_t pre, post;
    };
    ReducingSuccessorGenerator(const PetriNet& net);
    ReducingSuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries);
    virtual ~ReducingSuccessorGenerator();
    void prepare(const Structures::State* state);
    bool next(Structures::State& write);
    void presetOf(uint32_t place);
    void postsetOf(uint32_t place);
    void postPresetOf(uint32_t t);
    void inhibitorPostsetOf(uint32_t place);
    uint32_t leastDependentEnabled();
    uint32_t fired()
    {
       return _current;
    }
private:
    bool *_enabled, *_stubborn;
    std::unique_ptr<place_t[]> _places;
    std::unique_ptr<uint32_t> _transitions;
    light_deque<uint32_t> _unprocessed, _ordering;
    uint32_t *_dependency;
    uint32_t _current;
    bool _netContainsInhibitorArcs;
    std::vector<std::vector<uint32_t>> _inhibpost;
    
    std::vector<std::shared_ptr<PQL::Condition> > _queries;
    void reset();
    void constructEnabled();
    void constructPrePost();
    void constructDependency();
    void checkForInhibitor();
};
}

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
