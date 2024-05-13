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

#ifndef VERIFYPN_STUBBORNSET_H
#define VERIFYPN_STUBBORNSET_H

#include "PetriEngine/PetriNet.h"
#include "PetriEngine/Structures/State.h"
#include "utils/structures/light_deque.h"
#include "PetriEngine/PQL/PQL.h"

#include <memory>
#include <vector>

namespace PetriEngine {
    class StubbornSet {
    public:
        StubbornSet(const PetriEngine::PetriNet &net)
                : _net(net), _inhibpost(net._nplaces) {
            _current = 0;
            _enabled = std::make_unique<bool[]>(net._ntransitions);
            _stubborn = std::make_unique<bool[]>(net._ntransitions);
            _dependency = std::make_unique<uint32_t[]>(net._ntransitions);
            _places_seen = std::make_unique<uint8_t[]>(_net.numberOfPlaces());
            StubbornSet::reset();
            constructPrePost();
            constructDependency();
            checkForInhibitor();

        }

        StubbornSet(const PetriEngine::PetriNet &net, const std::vector<PQL::Condition_ptr> &queries)
                : StubbornSet(net) {
            for (auto &q: queries) {
                _queries.push_back(q.get());
            }
        }

        StubbornSet(const PetriEngine::PetriNet &net, const PQL::Condition_ptr &query)
                : StubbornSet(net) {
            _queries.push_back(query.get());
        }

        StubbornSet(const PetriEngine::PetriNet &net, PQL::Condition* &query)
                : StubbornSet(net) {
            _queries.push_back(query);
        }

        virtual bool prepare(const Structures::State *marking) = 0;

        virtual uint32_t next();

        virtual ~StubbornSet() = default;

        virtual void reset();

        [[nodiscard]] const MarkVal *getParent() const {
            return _parent->marking();
        }

        uint32_t _current = 0;

        void presetOf(uint32_t place, bool make_closure = false);

        void postsetOf(uint32_t place, bool make_closure = false);

        void postPresetOf(uint32_t t, bool make_closure = false);

        void inhibitorPostsetOf(uint32_t place);

        bool seenPre(uint32_t place) const;

        bool seenPost(uint32_t place) const;

        uint32_t leastDependentEnabled();

        uint32_t fired() {
            return _current;
        }

        void setQuery(PQL::Condition *ptr) {
            _queries.clear();
            _queries = {ptr};
        }

        void setQueries(std::vector<PQL::Condition*> conds) {
            _queries = conds;
        }

        [[nodiscard]] size_t nenabled() const { return _nenabled; }

        [[nodiscard]] bool *enabled() const { return _enabled.get(); };
        [[nodiscard]] bool *stubborn() const { return _stubborn.get(); };

        const PetriEngine::PetriNet &_net;

        // Bit flags for _places_seen array.
        static constexpr auto PresetSeen = 1;
        static constexpr auto PostsetSeen = 2;
        static constexpr auto InhibPostsetSeen = 4;
    protected:
        const Structures::State *_parent;

        struct place_t {
            uint32_t pre, post;
        };

        struct trans_t {
            uint32_t index;
            int8_t direction;

            trans_t() = default;

            trans_t(uint32_t id, int8_t dir) : index(id), direction(dir) {};

            bool operator<(const trans_t &t) const {
                return index < t.index;
            }
        };

        const std::vector<TransPtr> &transitions() { return _net._transitions; }

        const std::vector<Invariant> &invariants() { return _net._invariants; }

        const std::vector<uint32_t> &placeToPtrs() { return _net._placeToPtrs; }

        bool checkPreset(uint32_t t);

        uint32_t dependency(uint32_t place, bool inhibitor) const {
            uint32_t post_size = _places[place+1].pre - _places[place].post;
            uint32_t pre_size = _places[place].post - _places[place].pre;
            if(!inhibitor)
                return pre_size;
            else
                return post_size;
        }

        virtual void addToStub(uint32_t t);

        template <typename T = std::nullptr_t>
        void closure(T&& callback = nullptr) {
            while (!_unprocessed.empty()) {
                if constexpr (!std::is_null_pointer_v<T>) {
                    if (!callback()) return;
                }
                uint32_t tr = _unprocessed.front();
                _unprocessed.pop_front();
                auto [finv, linv] = _net.preset(tr);
                if (_enabled[tr]) {
                    for (; finv < linv; ++finv) {
                        if (finv->direction < 0) {
                            auto place = finv->place;
                            for (uint32_t t = _places[place].post; t < _places[place + 1].pre; t++)
                                addToStub(_arcs[t].index);
                        }
                    }
                    if (_netContainsInhibitorArcs) {
                        auto [linv, ninv] = _net.postset(tr);
                        for (; linv < ninv; ++linv) {
                            if (linv->direction > 0)
                                inhibitorPostsetOf(linv->place);
                        }
                    }
                } else {
                    bool ok = false;
                    bool inhib = false;
                    uint32_t cand = std::numeric_limits<uint32_t>::max();
                    uint32_t deps = std::numeric_limits<uint32_t>::max();

                    // Lets try to see if we havent already added sufficient pre/post
                    // for this transition.
                    for (; finv < linv; ++finv) {
                        if ((*_parent).marking()[finv->place] < finv->tokens && !finv->inhibitor) {
                            ok = (_places_seen[finv->place] & PresetSeen) != 0;
                            if(ok)
                                cand = finv->place;
                            if(!ok)
                            {
                                if(deps >= dependency(finv->place, false))
                                {
                                    inhib = false;
                                    cand = finv->place;
                                    deps = dependency(finv->place, false);
                                }
                            }
                        } else if ((*_parent)[finv->place] >= finv->tokens && finv->inhibitor) {
                            ok = (_places_seen[finv->place] & PostsetSeen) != 0;
                            if(ok)
                                cand = finv->place;
                            if(!ok)
                            {
                                if(deps >= dependency(finv->place, true))
                                {
                                    inhib = true;
                                    cand = finv->place;
                                    deps = dependency(finv->place, true);
                                }
                            }
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

        std::unique_ptr<bool[]> _enabled, _stubborn;
        size_t _nenabled;
        std::unique_ptr<uint8_t[]> _places_seen;
        std::unique_ptr<place_t[]> _places;
        std::unique_ptr<trans_t[]> _arcs;
        light_deque<uint32_t> _unprocessed, _ordering;
        std::unique_ptr<uint32_t[]> _dependency;
        bool _netContainsInhibitorArcs, _done;
        std::vector<std::vector<uint32_t>> _inhibpost;

        std::vector<PQL::Condition *> _queries;

        template <typename T = std::nullptr_t>
        void constructEnabled(T&& callback = nullptr){
            _ordering.clear();
            memset(_enabled.get(), 0, _net.numberOfTransitions());
            memset(_stubborn.get(), 0, _net.numberOfTransitions());
            for (uint32_t p = 0; p < _net.numberOfPlaces(); ++p) {
                // orphans are currently under "place 0" as a special case
                if (p == 0 || _parent->marking()[p] > 0) {
                    uint32_t t = placeToPtrs()[p];
                    uint32_t last = placeToPtrs()[p + 1];

                    for (; t != last; ++t) {
                        if (!checkPreset(t)) {
                            continue;
                        }
                        if constexpr (!std::is_null_pointer_v<T>)
                            if(!callback(t))
                                return;
                        _enabled[t] = true;
                        _ordering.push_back(t);
                        ++_nenabled;
                    }
                }
            }
        }

        void constructPrePost();

        void constructDependency();

        void checkForInhibitor();

        void set_all_stubborn() {
            std::fill(_stubborn.get(), _stubborn.get() + _net.numberOfTransitions(), true);
            _done = true;
        }
    };
}

#endif //VERIFYPN_STUBBORNSET_H
