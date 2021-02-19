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
#include "PetriEngine/Structures/light_deque.h"
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

        virtual void prepare(const Structures::State *marking) = 0;

        uint32_t next();

        virtual ~StubbornSet() = default;

        virtual void reset();

        [[nodiscard]] const MarkVal *parent() const {
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

        [[nodiscard]] size_t nenabled() const { return _nenabled; }

        [[nodiscard]] const bool *enabled() const { return _enabled.get(); };
        [[nodiscard]] const bool *stubborn() const { return _stubborn.get(); };

    protected:
        const PetriEngine::PetriNet &_net;
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

        void addToStub(uint32_t t);

        void closure();

        std::unique_ptr<bool[]> _enabled, _stubborn;
        size_t _nenabled;

    protected:
        std::unique_ptr<uint8_t[]> _places_seen;
        std::unique_ptr<place_t[]> _places;
        std::unique_ptr<trans_t[]> _transitions;
        light_deque<uint32_t> _unprocessed, _ordering;
        std::unique_ptr<uint32_t[]> _dependency;
        bool _netContainsInhibitorArcs;
        std::vector<std::vector<uint32_t>> _inhibpost;

        std::vector<PQL::Condition *> _queries;

        void constructEnabled();

        void constructPrePost();

        void constructDependency();

        void checkForInhibitor();
    };
}

#endif //VERIFYPN_STUBBORNSET_H
