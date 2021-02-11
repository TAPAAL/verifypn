/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#ifndef VERIFYPN_TARJANMODELCHECKER_H
#define VERIFYPN_TARJANMODELCHECKER_H


#include "LTL/Algorithm/ModelChecker.h"
#include "LTL/Structures/ProductStateFactory.h"
#include "PetriEngine/Structures/StateSet.h"
#include "PetriEngine/SuccessorGenerator.h"

#include <stack>
#include <unordered_set>

namespace LTL {

    template <bool SaveTrace = false>
    class TarjanModelChecker : public ModelChecker<PetriEngine::SuccessorGenerator> {
    public:
        TarjanModelChecker(const PetriEngine::PetriNet &net, const Condition_ptr &cond, const bool shortcircuitweak = true)
                : ModelChecker(net, cond, PetriEngine::SuccessorGenerator{net, cond}, shortcircuitweak),
                factory(net, successorGenerator->initial_buchi_state()),
                  seen(net, 0, (int) net.numberOfPlaces() + 1) {
            chash.fill(std::numeric_limits<idx_t>::max());
        }

        bool isSatisfied() override;

        void printStats(ostream &os) override {
            _printStats(os, seen);
        }

    protected:
        using State = LTL::Structures::ProductState;
        using idx_t = size_t;
        // 64 MB hash table
        static constexpr idx_t HashSz = 16777216;

        LTL::Structures::ProductStateFactory factory;

        PetriEngine::Structures::StateSet seen;
        std::unordered_set<idx_t> store;

        std::array<idx_t, HashSz> chash;
        static_assert(sizeof(chash) == (1U << 27U));

        static inline idx_t hash(idx_t id) {
            return id % HashSz;
        }

        struct PlainCEntry {
            idx_t lowlink;
            idx_t stateid;
            idx_t next = std::numeric_limits<idx_t>::max();

            PlainCEntry(idx_t lowlink, idx_t stateid, idx_t next) : lowlink(lowlink), stateid(stateid), next(next) {}
        };

        struct TracableCEntry : PlainCEntry {
            idx_t lowsource = std::numeric_limits<idx_t>::max();
            idx_t sourcetrans;

            TracableCEntry(idx_t lowlink, idx_t stateid, idx_t next) : PlainCEntry(lowlink, stateid, next) {}
        };

        using CEntry = std::conditional_t<SaveTrace,
            TracableCEntry,
            PlainCEntry>;

        struct DEntry {
            idx_t pos;
            PetriEngine::successor_info sucinfo;
        };


        std::vector<CEntry> cstack;
        std::stack<DEntry> dstack;
        std::stack<idx_t> astack;
        bool violation = false;

        void push(State &state);

        void pop();

        void update(idx_t to);

        bool nexttrans(State &state, State& parent, DEntry &delem);

        void popCStack();

    };
extern template class TarjanModelChecker<true>;
extern template class TarjanModelChecker<false>;
}

#endif //VERIFYPN_TARJANMODELCHECKER_H
