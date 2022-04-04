/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <PetriEngine/Stubborn/InterestingTransitionVisitor.h>

#include <memory>
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "PetriEngine/PQL/Contexts.h"

namespace PetriEngine {
    uint32_t StubbornSet::next() {
        while (!_ordering.empty()) {
            _current = _ordering.front();
            _ordering.pop_front();
            if (_stubborn[_current] && _enabled[_current]) {
                return _current;
            }
        }
        return std::numeric_limits<uint32_t>::max();
    }

    bool StubbornSet::checkPreset(uint32_t t) {
        const TransPtr &ptr = transitions()[t];
        uint32_t finv = ptr.inputs;
        uint32_t linv = ptr.outputs;

        for (; finv < linv; ++finv) {
            const Invariant &inv = _net._invariants[finv];
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
        return (_places_seen[place] & PresetSeen) != 0;
    }

    bool StubbornSet::seenPost(uint32_t place) const {
        return (_places_seen[place] & PostsetSeen) != 0;
    }

    void StubbornSet::presetOf(uint32_t place, bool make_closure) {
        if ((_places_seen[place] & PresetSeen) != 0) return;
        _places_seen[place] = _places_seen[place] | PresetSeen;
        for (uint32_t t = _places[place].pre; t < _places[place].post; t++) {
            const auto &tr = _arcs[t];
            addToStub(tr.index);
        }
        if (make_closure) closure();
    }

    void StubbornSet::postsetOf(uint32_t place, bool make_closure) {
        if ((_places_seen[place] & PostsetSeen) != 0) return;
        _places_seen[place] = _places_seen[place] | PostsetSeen;
        for (uint32_t t = _places[place].post; t < _places[place + 1].pre; t++) {
            const auto& tr = _arcs[t];
            if (tr.direction < 0)
                addToStub(tr.index);
        }
        if (make_closure) closure();
    }

    void StubbornSet::inhibitorPostsetOf(uint32_t place) {
        if ((_places_seen[place] & InhibPostsetSeen) != 0) return;
        _places_seen[place] = _places_seen[place] | InhibPostsetSeen;
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

    void StubbornSet::checkForInhibitor() {
        _netContainsInhibitorArcs = false;
        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            const TransPtr &ptr = _net._transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; finv++) { // Post set of places
                if (_net._invariants[finv].inhibitor) {
                    _netContainsInhibitorArcs = true;
                    return;
                }
            }
        }
    }

    void StubbornSet::constructPrePost() {
        std::vector<std::pair<std::vector<trans_t>, std::vector<trans_t>>> tmp_places(_net._nplaces);

        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            const TransPtr &ptr = _net._transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; finv++) { // Post set of places
                if (_net._invariants[finv].inhibitor) {
                    _inhibpost[_net._invariants[finv].place].push_back(t);
                    _netContainsInhibitorArcs = true;
                } else {
                    tmp_places[_net._invariants[finv].place].second.emplace_back(t, _net._invariants[finv].direction);
                }
            }

            finv = linv;
            linv = _net._transitions[t + 1].inputs;
            for (; finv < linv; finv++) { // Pre set of places
                if (_net._invariants[finv].direction > 0)
                    tmp_places[_net._invariants[finv].place].first.emplace_back(t, _net._invariants[finv].direction);
            }
        }

        // flatten
        size_t ntrans = 0;
        for (const auto& p : tmp_places) {
            ntrans += p.first.size() + p.second.size();
        }
        _arcs = std::make_unique<trans_t[]>(ntrans);

        _places = std::make_unique<place_t[]>(_net._nplaces + 1);
        uint32_t offset = 0;
        uint32_t p = 0;
        for (; p < _net._nplaces; ++p) {
            auto &pre = tmp_places[p].first;
            auto &post = tmp_places[p].second;

            // keep things nice for caches
            std::sort(pre.begin(), pre.end());
            std::sort(post.begin(), post.end());

            _places[p].pre = offset;
            offset += pre.size();
            _places[p].post = offset;
            offset += post.size();
            for (size_t tn = 0; tn < pre.size(); ++tn) {
                _arcs[tn + _places[p].pre] = pre[tn];
            }

            for (size_t tn = 0; tn < post.size(); ++tn) {
                _arcs[tn + _places[p].post] = post[tn];
            }

        }
        assert(offset == ntrans);
        _places[p].pre = offset;
        _places[p].post = offset;
    }

    void StubbornSet::constructDependency() {
        memset(_dependency.get(), 0, sizeof(uint32_t) * _net._ntransitions);

        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            auto [finv, linv] = _net.preset(t);

            size_t dep = 0;
            for (; finv < linv; ++finv) {
                uint32_t p = finv->place;
                uint32_t ntrans = (_places[p + 1].pre - _places[p].post);
                dep += ntrans;
            }
            _dependency[t] = dep;
        }
    }


    void StubbornSet::addToStub(uint32_t t) {
        if (!_stubborn[t]) {
            _stubborn[t] = true;
            _unprocessed.push_back(t);
        }
    }

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
        std::fill(_enabled.get(), _enabled.get() + _net.numberOfTransitions(), false);
        std::fill(_stubborn.get(), _stubborn.get() + _net.numberOfTransitions(), false);
        std::fill(_places_seen.get(), _places_seen.get() + _net.numberOfPlaces(), 0);
        _ordering.clear();
        _nenabled = 0;
        //_tid = 0;
        _done = false;
    }
}
