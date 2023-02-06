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

#ifndef VERIFYPN_VISIBLELTLSTUBBORNSET_H
#define VERIFYPN_VISIBLELTLSTUBBORNSET_H


//#include "PetriEngine/ReducingSuccessorGenerator.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "PetriEngine/PQL/PQL.h"
#include "LTL/Stubborn/VisibleTransitionVisitor.h"
#include "LTL/SuccessorGeneration/SuccessorSpooler.h"

// TODO LTL Stubborn sets should be subclassing just PetriEngine::StubbornSet, then a class
// LTL::StubbornSuccessorSpooler : LTL::SuccessorSpooler has a stubborn set via simple wrapping.
// This way we can avoid the super ugly multiple inheritance caused by both base classes
// each having prepare, next, reset methods.q
namespace LTL {
    class VisibleLTLStubbornSet : public PetriEngine::StubbornSet, public SuccessorSpooler {
    public:
        VisibleLTLStubbornSet(const PetriEngine::PetriNet &net,
                              const std::vector<PetriEngine::PQL::Condition_ptr> &queries)
                : StubbornSet(net, queries), _visible(new bool[net.numberOfTransitions()])
        {
            assert(!_netContainsInhibitorArcs);
            memset(_visible.get(), 0, sizeof(bool) * net.numberOfPlaces());
            VisibleTransitionVisitor visible{_visible};
            for (auto &q : queries) {
                PetriEngine::PQL::Visitor::visit(visible, q);
            }
        }

        VisibleLTLStubbornSet(const PetriEngine::PetriNet &net, const PetriEngine::PQL::Condition_ptr &query)
                : StubbornSet(net, query), _visible(new bool[net.numberOfTransitions()])
        {
            assert(!_netContainsInhibitorArcs);
            auto places = std::make_unique<bool[]>(net.numberOfPlaces());
            memset(places.get(), 0, sizeof(bool) * net.numberOfPlaces());
            memset(_visible.get(), 0, sizeof(bool) * net.numberOfTransitions());
            VisibleTransitionVisitor visible{places};
            PetriEngine::PQL::Visitor::visit(visible, query);

            memset(_places_seen.get(), 0, _net.numberOfPlaces());
            for (uint32_t p = 0; p < net.numberOfPlaces(); ++p) {
                if (places[p]) {
                    visTrans(p);
                }
            }
        }

        void visTrans(uint32_t place)
        {
            if (_places_seen[place] > 0) return;
            _places_seen[place] = 1;
            for (uint32_t t = _places[place].pre; t < _places[place].post; ++t) {
                const auto& tr = _arcs[t];
                _visible[tr.index] = true;
            }
            for (uint32_t t = _places.get()[place].post; t < _places.get()[place + 1].pre; t++) {
                const auto& tr = _arcs[t];
                if (tr.direction < 0)
                    _visible[tr.index] = true;
            }
        }

        bool prepare(const PetriEngine::Structures::State *marking) override;

        bool prepare(const LTL::Structures::ProductState *marking) override
        {
            return prepare((const PetriEngine::Structures::State *) marking);
        }

        uint32_t next() override;

        void reset();

        bool generateAll(const LTL::Structures::ProductState *parent);

    protected:
        void addToStub(uint32_t t) override;

    private:
        std::unique_ptr<bool[]> _visible;
        light_deque<uint32_t> _skipped;
        uint32_t _key;
        bool _has_enabled_stubborn;

        void findKeyTransition();

        void ensureRuleV();

        void ensureRulesL();

    };
}

#endif //VERIFYPN_VISIBLELTLSTUBBORNSET_H
