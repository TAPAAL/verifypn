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

#ifndef VERIFYPN_STUBBORNTARJANMODELCHECKER_H
#define VERIFYPN_STUBBORNTARJANMODELCHECKER_H

#include "LTL/Stubborn/LTLStubbornSet.h"
#include "LTL/Algorithm/ModelChecker.h"
#include "PetriEngine/Structures/light_deque.h"

namespace LTL {
    class StubbornTarjanModelChecker : public ModelChecker<PetriEngine::ReducingSuccessorGenerator> {
    public:
        StubbornTarjanModelChecker(const PetriEngine::PetriNet &net, const Condition_ptr &query)
                : ModelChecker<PetriEngine::ReducingSuccessorGenerator>
                          (net, query,
                           PetriEngine::ReducingSuccessorGenerator{
                                   net, std::make_shared<LTLStubbornSet>(net, query)}),
                  factory(net, successorGenerator->initial_buchi_state()),
                  seen(net, 0, (int) net.numberOfPlaces() + 1) {
            chash.fill(std::numeric_limits<idx_t>::max());
        }

        bool isSatisfied() override;

        void printStats(ostream &os) override {
            _printStats(os, seen);
        }
    private:

        using State = LTL::Structures::ProductState;
        using idx_t = size_t;
        // 64 MB hash table
        static constexpr idx_t HashSz = (1U << 28U);

        LTL::Structures::ProductStateFactory factory;

        PetriEngine::Structures::StateSet seen;
        std::unordered_set<idx_t> store;

        std::array<idx_t, HashSz> chash;

        static inline idx_t hash(idx_t id) {
            return id % HashSz;
        }

        struct CEntry {
            idx_t lowlink;
            idx_t stateid;
            idx_t next = std::numeric_limits<idx_t>::max();

            CEntry(idx_t lowlink, idx_t stateid, idx_t next) : lowlink(lowlink), stateid(stateid), next(next) {}
        };

        struct DEntry {
            idx_t pos;
            light_deque<idx_t> successors{64};
            bool expanded=false;
        };


        std::vector<CEntry> cstack;
        std::stack<DEntry> dstack;
        std::stack<idx_t> astack;
        bool violation = false;

        void push(State &state);

        void pop();

        void update(idx_t to);

        bool nexttrans(State &state, State &parent, DEntry &delem);

        void popCStack();

        idx_t searchCStack(idx_t stateid) const {
            auto p = chash[hash(stateid)];
            while (p != numeric_limits<idx_t>::max() && cstack[p].stateid != stateid) {
                p = cstack[p].next;
            }
            return p;
        }
    };
}

#endif //VERIFYPN_STUBBORNTARJANMODELCHECKER_H
