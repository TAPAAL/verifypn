#include "PetriEngine/ReducingSuccessorGenerator.h"

#include <cassert>
#include "PetriEngine/PQL/Contexts.h"

namespace PetriEngine {

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet& net) : SuccessorGenerator(net), _inhibpost(net._nplaces){
        _current = 0;
        _enabled = std::make_unique<bool[]>(net._ntransitions);
        _stubborn = std::make_unique<bool[]>(net._ntransitions);
        _dependency = std::make_unique<uint32_t[]>(net._ntransitions);
        _places_seen = std::make_unique<uint8_t[]>(_net.numberOfPlaces());
        reset();
        constructPrePost();
        constructDependency();
        checkForInhibitor();      
    }

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries) : ReducingSuccessorGenerator(net) {
        _queries.reserve(queries.size());
        for(auto& q : queries)
            _queries.push_back(q.get());
    }

    ReducingSuccessorGenerator::~ReducingSuccessorGenerator() {
    }
    
    void ReducingSuccessorGenerator::checkForInhibitor(){
        _netContainsInhibitorArcs=false;
        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            const TransPtr& ptr = _net._transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; finv++) { // Post set of places
                if (_net._invariants[finv].inhibitor) {
                    _netContainsInhibitorArcs=true;
                    return;
                }
            }
        }
    }

    void ReducingSuccessorGenerator::constructPrePost() {
        std::vector<std::pair<std::vector<trans_t>, std::vector < trans_t>>> tmp_places(_net._nplaces);
                
        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            const TransPtr& ptr = _net._transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; finv++) { // Post set of places
                if (_net._invariants[finv].inhibitor) {
                    _inhibpost[_net._invariants[finv].place].push_back(t);
                    _netContainsInhibitorArcs=true;
                } else {
                    tmp_places[_net._invariants[finv].place].second.emplace_back(t, _net._invariants[finv].direction);
                }
            }

            finv = linv;
            linv = _net._transitions[t + 1].inputs;
            for (; finv < linv; finv++) { // Pre set of places
                if(_net._invariants[finv].direction > 0)
                    tmp_places[_net._invariants[finv].place].first.emplace_back(t, _net._invariants[finv].direction);
            }
        }

        // flatten
        size_t ntrans = 0;
        for (auto p : tmp_places) {
            ntrans += p.first.size() + p.second.size();
        }
        _transitions.reset(new trans_t[ntrans]);

        _places.reset(new place_t[_net._nplaces + 1]);
        uint32_t offset = 0;
        uint32_t p = 0;
        for (; p < _net._nplaces; ++p) {
            auto& pre = tmp_places[p].first;
            auto& post = tmp_places[p].second;

            // keep things nice for caches
            std::sort(pre.begin(), pre.end());
            std::sort(post.begin(), post.end());

            _places.get()[p].pre = offset;
            offset += pre.size();
            _places.get()[p].post = offset;
            offset += post.size();
            for (size_t tn = 0; tn < pre.size(); ++tn) {
                _transitions.get()[tn + _places.get()[p].pre] = pre[tn];
            }

            for (size_t tn = 0; tn < post.size(); ++tn) {
                _transitions.get()[tn + _places.get()[p].post] = post[tn];
            }

        }
        assert(offset == ntrans);
        _places.get()[p].pre = offset;
        _places.get()[p].post = offset;
    }

    void ReducingSuccessorGenerator::constructDependency() {
        memset(_dependency.get(), 0, sizeof(uint32_t) * _net._ntransitions);

        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            uint32_t finv = _net._transitions[t].inputs;
            uint32_t linv = _net._transitions[t].outputs;

            for (; finv < linv; finv++) {
                const Invariant& inv = _net._invariants[finv];
                uint32_t p = inv.place;
                uint32_t ntrans = (_places.get()[p + 1].pre - _places.get()[p].post);

                for (uint32_t tIndex = 0; tIndex < ntrans; tIndex++) {
                    ++_dependency[t];
                }
            }
        }
    }

    void ReducingSuccessorGenerator::constructEnabled() {
        _ordering.clear();
        for (uint32_t p = 0; p < _net._nplaces; ++p) {
            // orphans are currently under "place 0" as a special case
            if (p == 0 || (*_parent).marking()[p] > 0) { 
                uint32_t t = _net._placeToPtrs[p];
                uint32_t last = _net._placeToPtrs[p + 1];

                for (; t != last; ++t) {
                    if (!checkPreset(t)) continue;
                    _enabled[t] = true;
                    _ordering.push_back(t);
                }
            }
        }
    }

    bool ReducingSuccessorGenerator::seenPre(uint32_t place) const
    {
        return (_places_seen.get()[place] & 1) != 0;
    }
    
    bool ReducingSuccessorGenerator::seenPost(uint32_t place) const
    {
        return (_places_seen.get()[place] & 2) != 0;
    }
    
    void ReducingSuccessorGenerator::presetOf(uint32_t place, bool make_closure) {
        if((_places_seen.get()[place] & 1) != 0) return;
        _places_seen.get()[place] = _places_seen.get()[place] | 1;
        for (uint32_t t = _places.get()[place].pre; t < _places.get()[place].post; t++)
        {
            auto& tr = _transitions.get()[t];
            addToStub(tr.index);
        }
        if(make_closure) closure();            
    }
    
    void ReducingSuccessorGenerator::postsetOf(uint32_t place, bool make_closure) {       
        if((_places_seen.get()[place] & 2) != 0) return;
        _places_seen.get()[place] = _places_seen.get()[place] | 2;
        for (uint32_t t = _places.get()[place].post; t < _places.get()[place + 1].pre; t++) {
            auto tr = _transitions.get()[t];
            if(tr.direction < 0)
                addToStub(tr.index);
        }
        if(make_closure) closure();
    }
    
    void ReducingSuccessorGenerator::addToStub(uint32_t t)
    {
        if(!_stubborn[t])
        {
            _stubborn[t] = true;
            _unprocessed.push_back(t);
        }
    }
    
    void ReducingSuccessorGenerator::inhibitorPostsetOf(uint32_t place){
        if((_places_seen.get()[place] & 4) != 0) return;
        _places_seen.get()[place] = _places_seen.get()[place] | 4;
        for(uint32_t& newstub : _inhibpost[place])
            addToStub(newstub);
    }
    
    void ReducingSuccessorGenerator::postPresetOf(uint32_t t, bool make_closure) {
        const TransPtr& ptr = _net._transitions[t];
        uint32_t finv = ptr.inputs;
        uint32_t linv = ptr.outputs;
        for (; finv < linv; finv++) { // pre-set of t
            if(_net._invariants[finv].inhibitor){ 
                presetOf(_net._invariants[finv].place, make_closure);
            } else {
                postsetOf(_net._invariants[finv].place, make_closure); 
            }
        }        
    }
    

    void ReducingSuccessorGenerator::prepare(const Structures::State* state) {
        _parent = state;
        memset(_places_seen.get(), 0, _net.numberOfPlaces());
        constructEnabled();
        if(_ordering.size() == 0) return;
        if(_ordering.size() == 1)
        {
            _stubborn[_ordering.front()] = true;
            return;
        }
        for (auto &q : _queries) {
            q->evalAndSet(PQL::EvaluationContext((*_parent).marking(), &_net));
            q->findInteresting(*this, false);
        }
        
        closure();
    }
    
    void ReducingSuccessorGenerator::closure()
    {
        while (!_unprocessed.empty()) {
            uint32_t tr = _unprocessed.front();
            _unprocessed.pop_front();
            const TransPtr& ptr = _net._transitions[tr];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            if(_enabled[tr]){
                for (; finv < linv; finv++) {
                    if(_net._invariants[finv].direction < 0)
                    {
                        auto place = _net._invariants[finv].place;
                        for (uint32_t t = _places.get()[place].post; t < _places.get()[place + 1].pre; t++) 
                            addToStub(_transitions.get()[t].index);
                    }
                }
                if(_netContainsInhibitorArcs){
                    uint32_t next_finv = _net._transitions[tr+1].inputs;
                    for (; linv < next_finv; linv++)
                    {
                        if(_net._invariants[linv].direction > 0)
                            inhibitorPostsetOf(_net._invariants[linv].place);
                    }
                }
            } else {
                bool ok = false;
                bool inhib = false;
                uint32_t cand = std::numeric_limits<uint32_t>::max();
               
                // Lets try to see if we havent already added sufficient pre/post 
                // for this transition.
                for (; finv < linv; ++finv) {
                    const Invariant& inv = _net._invariants[finv];
                    if ((*_parent).marking()[inv.place] < inv.tokens && !inv.inhibitor) {
                        inhib = false;
                        ok = (_places_seen.get()[inv.place] & 1) != 0;
                        cand = inv.place;
                    } else if ((*_parent).marking()[inv.place] >= inv.tokens && inv.inhibitor) {
                        inhib = true;
                        ok = (_places_seen.get()[inv.place] & 2) != 0;
                        cand = inv.place;
                    }
                    if(ok) break;

                }
                
                // OK, we didnt have sufficient, we just pick whatever is left
                // in cand.
                assert(cand != std::numeric_limits<uint32_t>::max());
                if(!ok && cand != std::numeric_limits<uint32_t>::max())
                {
                    if(!inhib) presetOf(cand);
                    else       postsetOf(cand);
                }
            }
        }        
    }

    bool ReducingSuccessorGenerator::next(Structures::State& write) {
        while (!_ordering.empty()) {
            _current = _ordering.front();
            _ordering.pop_front();
            if (_stubborn[_current]) {
                assert(_enabled[_current]);
                memcpy(write.marking(), (*_parent).marking(), _net._nplaces*sizeof(MarkVal));
                consumePreset(write, _current);
                producePostset(write, _current);
                return true;
            }
        }
        reset();
        return false;
    }
    
    uint32_t ReducingSuccessorGenerator::leastDependentEnabled() {
        uint32_t tLeast = -1;
        bool foundLeast = false;
        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            if (_enabled[t]) {
                if (!foundLeast) {
                    tLeast = t;
                    foundLeast = true;
                } else {
                    if (_dependency[t] < _dependency[tLeast]) {
                        tLeast = t;
                    }
                }
            }
        }
        return tLeast;
    }

    void ReducingSuccessorGenerator::reset() {
        SuccessorGenerator::reset();
        memset(_enabled.get(), false, sizeof(bool) * _net._ntransitions);
        memset(_stubborn.get(), false, sizeof(bool) * _net._ntransitions);
    }
}
