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

#ifndef VERIFYPN_LTLSTUBBORNSET_H
#define VERIFYPN_LTLSTUBBORNSET_H


//#include "PetriEngine/ReducingSuccessorGenerator.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "PetriEngine/PQL/PQL.h"
#include "LTL/Stubborn/VisibleTransitionVisitor.h"

namespace LTL {
    class LTLStubbornSet : public PetriEngine::StubbornSet {
    public:
        LTLStubbornSet(const PetriEngine::PetriNet &net, const std::vector<PetriEngine::PQL::Condition_ptr> &queries)
                : StubbornSet(net, queries), _visible(new bool[net.numberOfTransitions()]) {
            assert(!_netContainsInhibitorArcs);
            VisibleTransitionVisitor visible{_visible};
            for (auto &q : queries) {
                q->visit(visible);
            }
        }

        LTLStubbornSet(const PetriEngine::PetriNet &net, const PetriEngine::PQL::Condition_ptr &query)
                : StubbornSet(net, query), _visible(new bool[net.numberOfPlaces()]) {
            assert(!_netContainsInhibitorArcs);
            VisibleTransitionVisitor visible{_visible};
            query->visit(visible);
        }

        void prepare(const PetriEngine::Structures::State *marking) override;

        uint32_t next();

        void reset();

        void generateAll();
    private:
        std::unique_ptr<bool[]> _visible;
        light_deque<uint32_t> _skipped;
        uint32_t _key;

        void findKeyTransition();

        void ensureRuleV();

        void ensureRulesL();

    };
}

#endif //VERIFYPN_LTLSTUBBORNSET_H
