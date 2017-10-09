#include "ReducingSuccessorGenerator.h"

#include <assert.h>
#include "PQL/Contexts.h"

namespace PetriEngine {

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet& net) : SuccessorGenerator(net), _inhibpost(net._nplaces){
        _current = 0;
        _enabled = new bool[net._ntransitions];
        _stubborn = new bool[net._ntransitions];
        _dependency = new uint32_t[net._ntransitions];

        reset();
        constructPrePost();
        constructDependency();
        checkForInhibitor();      
    }

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries) : ReducingSuccessorGenerator(net) {
        _queries = queries;
    }

    ReducingSuccessorGenerator::~ReducingSuccessorGenerator() {
        delete [] _enabled;
        delete [] _stubborn;
        delete [] _dependency;
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
        std::vector<std::pair<std::vector<uint32_t>, std::vector < uint32_t>>> tmp_places(_net._nplaces);
                
        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            const TransPtr& ptr = _net._transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; finv++) { // Post set of places
                if (_net._invariants[finv].inhibitor) {
                    _inhibpost[_net._invariants[finv].place].push_back(t);
                    _netContainsInhibitorArcs=true;
                } else {
                    tmp_places[_net._invariants[finv].place].second.push_back(t);
                }
            }

            finv = linv;
            linv = _net._transitions[t + 1].inputs;
            for (; finv < linv; finv++) { // Pre set of places
                tmp_places[_net._invariants[finv].place].first.push_back(t);
            }
        }

        // flatten
        size_t ntrans = 0;
        for (auto p : tmp_places) {
            ntrans += p.first.size() + p.second.size();
        }
        _transitions.reset(new uint32_t[ntrans]);

        _places.reset(new place_t[_net._nplaces + 1]);
        uint32_t offset = 0;
        uint32_t p = 0;
        for (; p < _net._nplaces; ++p) {
            std::vector<uint32_t>& pre = tmp_places[p].first;
            std::vector<uint32_t>& post = tmp_places[p].second;

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
        memset(_dependency, 0, sizeof(uint32_t) * _net._ntransitions);

        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            uint32_t finv = _net._transitions[t].inputs;
            uint32_t linv = _net._transitions[t].outputs;

            for (; finv < linv; finv++) {
                const Invariant& inv = _net._invariants[finv];
                uint32_t p = inv.place;
                uint32_t ntrans = (_places.get()[p + 1].pre - _places.get()[p].post);

                for (uint32_t tIndex = 0; tIndex < ntrans; tIndex++) {
                    _dependency[t]++;
                }
            }
        }
    }

    void ReducingSuccessorGenerator::constructEnabled() {
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
    
    void ReducingSuccessorGenerator::presetOf(uint32_t place) {
        for (uint32_t t = _places.get()[place].pre; t < _places.get()[place].post; t++) {
            uint32_t newstub = _transitions.get()[t];
            if(!_stubborn[newstub]){
                _stubborn[newstub] = true;
                _unprocessed.push_back(newstub);
            }
        }
    }
    
    void ReducingSuccessorGenerator::postsetOf(uint32_t place) {
        for (uint32_t t = _places.get()[place].post; t < _places.get()[place + 1].pre; t++) {
            uint32_t newstub = _transitions.get()[t];
            if(!_stubborn[newstub]){
                _stubborn[newstub] = true;
                _unprocessed.push_back(newstub);
            }
        }
    }
    
    void ReducingSuccessorGenerator::inhibitorPostsetOf(uint32_t place){
        for(uint32_t& newstub : _inhibpost[place]){
            if(!_stubborn[newstub]){
                _stubborn[newstub] = true;
                _unprocessed.push_back(newstub);
            }
        }
    }
    
    void ReducingSuccessorGenerator::postPresetOf(uint32_t t) {
        const TransPtr& ptr = _net._transitions[t];
        uint32_t finv = ptr.inputs;
        uint32_t linv = ptr.outputs;
        for (; finv < linv; finv++) { // pre-set of t
            if(_net._invariants[finv].inhibitor){ 
                presetOf(_net._invariants[finv].place);
            } else {
                postsetOf(_net._invariants[finv].place); 
            }
        }        
    }
    

    void ReducingSuccessorGenerator::prepare(const Structures::State* state) {
        _parent = state;
        constructEnabled();
        _queries.front()->evalAndSet(PQL::EvaluationContext((*_parent).marking(), &_net));
        for (auto &q : _queries) {
            q->findInteresting(*this, false);
        }
        
        while (!_unprocessed.empty()) {
            uint32_t tr = _unprocessed.front();
            _unprocessed.pop_front();
            const TransPtr& ptr = _net._transitions[tr];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            uint32_t next_finv = _net._transitions[tr+1].inputs;
            if (_enabled[tr]) {
                for (; finv < linv; finv++) {
                    postsetOf(_net._invariants[finv].place);
                }
                if(_netContainsInhibitorArcs){
                    for (; linv < next_finv; linv++) {                    
                        inhibitorPostsetOf(_net._invariants[finv].place);
                    }
                }
            } else {
                for (; finv < linv; ++finv) {
                    const Invariant& inv = _net._invariants[finv];
                    if ((*_parent).marking()[inv.place] < inv.tokens) {
                        presetOf(inv.place);
                        break;
                    } else if ((*_parent).marking()[inv.place] >= inv.tokens && inv.inhibitor) {
                        postsetOf(inv.place);
                        break;
                    }
                }
            }
        }
    }

    bool ReducingSuccessorGenerator::next(Structures::State& write) {
        while (!_ordering.empty()) {
            _current = _ordering.front();
            _ordering.pop_front();
            if (_stubborn[_current]) {
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
        memset(_enabled, false, sizeof(bool) * _net._ntransitions);
        memset(_stubborn, false, sizeof(bool) * _net._ntransitions);
    }
}
