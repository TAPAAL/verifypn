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

#ifndef VERIFYPN_AUTOMATASTUBBORNTARJAN_H
#define VERIFYPN_AUTOMATASTUBBORNTARJAN_H

#include "LTL/Stubborn/VisibleLTLStubbornSet.h"
#include "LTL/Stubborn/AutomatonStubbornSet.h"
#include "LTL/Stubborn/ReducingSuccessorGenerator.h"
#include "LTL/Algorithm/ModelChecker.h"
#include "PetriEngine/Structures/light_deque.h"
#include "LTL/Structures/BitProductStateSet.h"

namespace LTL {
    template<typename SuccessorGen, typename StateSet = PetriEngine::Structures::StateSet>
    class AutomataStubbornTarjan : public ModelChecker<SuccessorGen> {
        using StubSet = std::conditional_t<std::is_same_v<SuccessorGen, LTL::ReducingSuccessorGenerator>, AutomatonStubbornSet, VisibleLTLStubbornSet>;
    public:
        AutomataStubbornTarjan(const PetriEngine::PetriNet &net, const Condition_ptr &query, const TraceLevel level, const bool shorcircuitweak)
                : ModelChecker<SuccessorGen>
                          (net, query,
                           SuccessorGen{
                                   net, std::make_shared<StubSet>(net, query)},
                                   level, shorcircuitweak),
                  factory(net, this->successorGenerator->initial_buchi_state()),
                  seen(net, 0, net.numberOfPlaces() + 1)
        {
            if (this->successorGenerator->buchiStates() > 65535) {
                std::cout << "CANNOT_COMPUTE\n";
                std::cout << "Too many Buchi states: " << this->successorGenerator->buchiStates() << std::endl;
                exit(EXIT_FAILURE);
            }
            chash.fill(std::numeric_limits<idx_t>::max());
        }

        bool isSatisfied() override;

        void printStats(ostream &os) override
        {
            std::cout << "STATS:\n"
                      << "\tdiscovered states: " << seen.discovered() << std::endl
                      << "\texplored states:   " << this->stats.explored << std::endl
                      << "\texpanded states:   " << this->stats.expanded << std::endl
                      << "\tmax tokens:        " << seen.maxTokens() << std::endl;
        }

    private:

        using State = LTL::Structures::ProductState;
        using idx_t = size_t;

        static constexpr idx_t HashSz = (1U << 27U);

        LTL::Structures::ProductStateFactory factory;

        // TODO take number of Büchi states into account at construction time, perhaps implement pair-ID state set.
        static constexpr auto SaveTrace = std::is_same_v<StateSet, PetriEngine::Structures::TracableStateSet>;
        StateSet seen;
        std::unordered_set<idx_t> store;
        size_t loopstate = std::numeric_limits<size_t>::max();
        size_t looptrans = std::numeric_limits<size_t>::max();

        std::array<idx_t, HashSz> chash;

        static inline idx_t hash(idx_t id)
        {
            // Warning: Smarter hash function may violate cstack search for repeat markings
            // unless the new hash also considers only the marking part of state ID.
            // This should work since the marking ID is lower half of state ID.
            // ^ obsolete warning since bit product state set not in use in here
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

        struct Suc {
            idx_t id;
        };
        struct TraceSuc {
            idx_t id;
            idx_t trans;
        };
        using DSucc = std::conditional_t<SaveTrace, TraceSuc, Suc>;

        struct DEntry {
            idx_t pos;
            idx_t parenttrans;
            light_deque<DSucc> successors{};
            bool expanded = false;

        };

        uint32_t _lasttid;

        std::vector<CEntry> cstack;
        std::stack<DEntry> dstack;
        std::stack<idx_t> astack;
        bool violation = false;

        void push(State &state, size_t stateid);

        void pop();

        void update(idx_t to);

        bool nexttrans(State &state, State &parent, DEntry &delem);

        void popCStack();

        idx_t searchCStack(idx_t stateid) const
        {
            auto p = chash[hash(stateid)];
            while (p != numeric_limits<idx_t>::max() && cstack[p].stateid != stateid) {
                p = cstack[p].next;
            }
            return p;
        }

        std::ostream &printTransition(size_t transition, uint indent, std::ostream &os);

        void printTrace(std::stack<DEntry> &&revdstack, std::ostream &os = std::cout);

    };


    extern template
    class AutomataStubbornTarjan<PetriEngine::ReducingSuccessorGenerator, PetriEngine::Structures::StateSet>;

    extern template
    class AutomataStubbornTarjan<PetriEngine::ReducingSuccessorGenerator, PetriEngine::Structures::TracableStateSet>;

    extern template
    class AutomataStubbornTarjan<PetriEngine::SuccessorGenerator, PetriEngine::Structures::StateSet>;

    extern template
    class AutomataStubbornTarjan<PetriEngine::SuccessorGenerator, PetriEngine::Structures::TracableStateSet>;

    extern template
    class AutomataStubbornTarjan<LTL::ReducingSuccessorGenerator, PetriEngine::Structures::StateSet>;

    extern template
    class AutomataStubbornTarjan<LTL::ReducingSuccessorGenerator, PetriEngine::Structures::TracableStateSet>;
}

#endif //VERIFYPN_AUTOMATASTUBBORNTARJAN_H
