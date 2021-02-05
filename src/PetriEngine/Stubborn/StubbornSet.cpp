/*
 * File:   StubbornSet.cpp.cc
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 03/02/2021
 */

#include <PetriEngine/Stubborn/InterestingTransitionVisitor.h>
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "PetriEngine/PQL/Contexts.h"

namespace PetriEngine {
    size_t StubbornSet::next() {
        while (_tid < _net.numberOfTransitions()) {
            if (_stubborn[_tid]) {
                return _tid;
            }
            ++_tid;
        }
        return std::numeric_limits<size_t>::max();
    }

    bool StubbornSet::checkPreset(uint32_t t) {
        const TransPtr& ptr = transitions()[t];
        uint32_t finv = ptr.inputs;
        uint32_t linv = ptr.outputs;

        for (; finv < linv; ++finv) {
            const Invariant& inv = _net._invariants[finv];
            if (_parent->marking()[inv.place] < inv.tokens) {
                if (!inv.inhibitor) {
                    return false;
                }
            } else {
                if (inv.inhibitor) {
                    return false;
                }
            }
        }
        return true;
    }

    bool StubbornSet::seenPre(uint32_t place) const {
        return (_places_seen.get()[place] & 1) != 0;
    }

    bool StubbornSet::seenPost(uint32_t place) const {
        return (_places_seen.get()[place] & 2) != 0;
    }

    void StubbornSet::presetOf(uint32_t place, bool make_closure) {
        if ((_places_seen.get()[place] & 1) != 0) return;
        _places_seen.get()[place] = _places_seen.get()[place] | 1;
        for (uint32_t t = _places.get()[place].pre; t < _places.get()[place].post; t++) {
            auto &tr = _transitions.get()[t];
            addToStub(tr.index);
        }
        if (make_closure) closure();
    }

    void StubbornSet::postsetOf(uint32_t place, bool make_closure) {
        if ((_places_seen.get()[place] & 2) != 0) return;
        _places_seen.get()[place] = _places_seen.get()[place] | 2;
        for (uint32_t t = _places.get()[place].post; t < _places.get()[place + 1].pre; t++) {
            auto tr = _transitions.get()[t];
            if (tr.direction < 0)
                addToStub(tr.index);
        }
        if (make_closure) closure();
    }

    void StubbornSet::inhibitorPostsetOf(uint32_t place) {
        if ((_places_seen.get()[place] & 4) != 0) return;
        _places_seen.get()[place] = _places_seen.get()[place] | 4;
        for (uint32_t &newstub : _inhibpost[place])
            addToStub(newstub);
    }

    void StubbornSet::postPresetOf(uint32_t t, bool make_closure) {
        const TransPtr &ptr = transitions()[t];
        uint32_t finv = ptr.inputs;
        uint32_t linv = ptr.outputs;
        for (; finv < linv; finv++) { // pre-set of t
            if (invariants()[finv].inhibitor) {
                presetOf(invariants()[finv].place, make_closure);
            } else {
                postsetOf(invariants()[finv].place, make_closure);
            }
        }
    }

    void StubbornSet::constructEnabled() {
        _ordering.clear();
        for (uint32_t p = 0; p < _net.numberOfPlaces(); ++p) {
            // orphans are currently under "place 0" as a special case
            if (p == 0 || _parent->marking()[p] > 0) {
                uint32_t t = placeToPtrs()[p];
                uint32_t last = placeToPtrs()[p + 1];

                for (; t != last; ++t) {
                    if (!checkPreset(t)) continue;
                    _enabled[t] = true;
                    _ordering.push_back(t);
                }
            }
        }
    }

    void StubbornSet::checkForInhibitor(){
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

    void StubbornSet::constructPrePost() {
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

    void StubbornSet::constructDependency() {
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


    void StubbornSet::addToStub(uint32_t t) {
        if (!_stubborn[t]) {
            _stubborn[t] = true;
            _unprocessed.push_back(t);
        }
    }

    void StubbornSet::closure() {
        while (!_unprocessed.empty()) {
            uint32_t tr = _unprocessed.front();
            _unprocessed.pop_front();
            const TransPtr &ptr = transitions()[tr];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            if (_enabled[tr]) {
                for (; finv < linv; finv++) {
                    if (invariants()[finv].direction < 0) {
                        auto place = invariants()[finv].place;
                        for (uint32_t t = _places.get()[place].post; t < _places.get()[place + 1].pre; t++)
                            addToStub(_transitions.get()[t].index);
                    }
                }
                if (_netContainsInhibitorArcs) {
                    uint32_t next_finv = transitions()[tr + 1].inputs;
                    for (; linv < next_finv; linv++) {
                        if (invariants()[linv].direction > 0)
                            inhibitorPostsetOf(invariants()[linv].place);
                    }
                }
            } else {
                bool ok = false;
                bool inhib = false;
                uint32_t cand = std::numeric_limits<uint32_t>::max();

                // Lets try to see if we havent already added sufficient pre/post
                // for this transition.
                for (; finv < linv; ++finv) {
                    const Invariant &inv = invariants()[finv];
                    if ((*_parent).marking()[inv.place] < inv.tokens && !inv.inhibitor) {
                        inhib = false;
                        ok = (_places_seen.get()[inv.place] & 1) != 0;
                        cand = inv.place;
                    } else if ((*_parent).marking()[inv.place] >= inv.tokens && inv.inhibitor) {
                        inhib = true;
                        ok = (_places_seen.get()[inv.place] & 2) != 0;
                        cand = inv.place;
                    }
                    if (ok) break;

                }

                // OK, we didnt have sufficient, we just pick whatever is left
                // in cand.
                assert(cand != std::numeric_limits<uint32_t>::max());
                if (!ok && cand != std::numeric_limits<uint32_t>::max()) {
                    if (!inhib) presetOf(cand);
                    else postsetOf(cand);
                }
            }
        }
    }

    /*bool StubbornSet::next(Structures::State &write) {
        while (!_ordering.empty()) {
            _current = _ordering.front();
            _ordering.pop_front();
            if (_stubborn[_current]) {
                assert(_enabled[_current]);
                memcpy(write.marking(), (*_parent).marking(), _net._nplaces * sizeof(MarkVal));
                consumePreset(write, _current);
                producePostset(write, _current);
                return true;
            }
        }
        reset();
        return false;
    }*/

    uint32_t StubbornSet::leastDependentEnabled() {
        uint32_t tLeast = -1;
        bool foundLeast = false;
        for (uint32_t t = 0; t < _net.numberOfTransitions(); t++) {
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

    void StubbornSet::reset() {
        memset(_enabled.get(), false, sizeof(bool) * _net.numberOfTransitions());
        memset(_stubborn.get(), false, sizeof(bool) * _net.numberOfTransitions());
    }
}
