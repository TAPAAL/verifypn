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
    struct trans_t {
        uint32_t index;
        int8_t direction;
        trans_t() = default;
        trans_t(uint32_t id, int8_t dir) : index(id), direction(dir) {};
        bool operator<(const trans_t& t) const
        {
            return index < t.index;
        }
    };
    ReducingSuccessorGenerator(const PetriNet& net);
    ReducingSuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries);
    virtual ~ReducingSuccessorGenerator();
    void prepare(const Structures::State* state);
    bool next(Structures::State& write);
    void presetOf(uint32_t place, bool make_closure = false);
    void postsetOf(uint32_t place, bool make_closure = false);
    void postPresetOf(uint32_t t, bool make_closure = false);
    void inhibitorPostsetOf(uint32_t place);
    bool seenPre(uint32_t place) const;
    bool seenPost(uint32_t place) const;
    uint32_t leastDependentEnabled();
    uint32_t fired()
    {
       return _current;
    }
    void setQuery(PQL::Condition* ptr) { _queries.clear(); _queries = {ptr};}
    void reset();

private:
    inline void addToStub(uint32_t t);
    void closure();
    std::unique_ptr<bool[]> _enabled, _stubborn;
    std::unique_ptr<uint8_t[]> _places_seen;
    std::unique_ptr<place_t[]> _places;
    std::unique_ptr<trans_t[]> _transitions;
    light_deque<uint32_t> _unprocessed, _ordering;
    std::unique_ptr<uint32_t[]> _dependency;
    uint32_t _current;
    bool _netContainsInhibitorArcs;
    std::vector<std::vector<uint32_t>> _inhibpost;
    
    std::vector<PQL::Condition* > _queries;
    void constructEnabled();
    void constructPrePost();
    void constructDependency();
    void checkForInhibitor();
};
}

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
