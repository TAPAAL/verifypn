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

#ifndef VERIFYPN_RESUMINGSTUBBORNTARJAN_H
#define VERIFYPN_RESUMINGSTUBBORNTARJAN_H


#include "LTL/Stubborn/VisibleLTLStubbornSet.h"
#include "LTL/Algorithm/ModelChecker.h"
#include "PetriEngine/Structures/light_deque.h"
#include "LTL/Structures/BitProductStateSet.h"
#include "LTL/Structures/EnabledTransitionSet.h"

namespace LTL {
    class ResumingStubbornTarjan : public ModelChecker<PetriEngine::ReducingSuccessorGenerator> {
    public:
        ResumingStubbornTarjan(const PetriEngine::PetriNet &net, const Condition_ptr &query)
                : ModelChecker<PetriEngine::ReducingSuccessorGenerator>
                          (net, query,
                           PetriEngine::ReducingSuccessorGenerator{
                                   net, std::make_shared<VisibleLTLStubbornSet>(net, query)}),
                  factory(net, this->successorGenerator->initial_buchi_state()),
                  seen(net, 0), _enabled(net.numberOfTransitions()),
                  buf1(new bool[net.numberOfTransitions()]), buf2(new bool[net.numberOfTransitions()])
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
                      << "\tmax tokens:        " << seen.max_tokens() << std::endl;
        }

    private:

        using State = LTL::Structures::ProductState;
        using idx_t = size_t;
        using EnabledSet = LTL::Structures::EnabledTransitionSet;

        static constexpr idx_t HashSz = 16777216;

        LTL::Structures::ProductStateFactory factory;

        // TODO take number of Büchi states into account at construction time, perhaps implement pair-ID state set.
        LTL::Structures::BitProductStateSet<> seen;
        EnabledSet _enabled;
        std::unordered_set<idx_t> store;

        std::array<idx_t, HashSz> chash;
        static_assert(sizeof(chash) == (1U << 27U));

        static inline idx_t hash(idx_t id)
        {
            // Warning: Smarter hash function may violate cstack search for repeat markings
            // unless the new hash also considers only the marking part of state ID.
            // This should work since the marking ID is lower half of state ID.
            return id % HashSz;
        }

        struct CEntry {
            idx_t lowlink;
            idx_t stateid;
            idx_t next = std::numeric_limits<idx_t>::max();

            CEntry(idx_t lowlink, idx_t stateid, idx_t next) : lowlink(lowlink), stateid(stateid), next(next) {}

            //bool hasEnabled() const { return enabled != std::numeric_limits<idx_t>::max(); }
        };

        struct DEntry {
            idx_t pos;
            PetriEngine::ReducingSuccessorGenerator::sucinfo sucinfo;
        };

        std::vector<CEntry> cstack;
        std::stack<DEntry> dstack;
        std::stack<idx_t> astack;
        std::stack<idx_t> extstack; // tarjan extension; stack of states that were fully expanded in stubborn set
        bool violation = false;

        std::unique_ptr<bool[]> buf1;
        std::unique_ptr<bool[]> buf2;


        void push(State &state, size_t stateid);

        void pop();

        void update(idx_t to);

        bool nexttrans(State &state, State &parent, DEntry &delem);

        void popCStack();

        void expandAll(DEntry &delem);

        idx_t searchCStack(idx_t stateid) const
        {
            auto p = chash[hash(stateid)];
            while (p != numeric_limits<idx_t>::max() && cstack[p].stateid != stateid) {
                p = cstack[p].next;
            }
            return p;
        }
    };
}

#endif //VERIFYPN_RESUMINGSTUBBORNTARJAN_H
