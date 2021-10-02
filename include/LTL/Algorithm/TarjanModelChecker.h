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
#include "LTL/Structures/BitProductStateSet.h"
#include "LTL/SuccessorGeneration/ResumingSuccessorGenerator.h"
#include "LTL/SuccessorGeneration/SpoolingSuccessorGenerator.h"

#include <limits>
#include <stack>
#include <unordered_set>

namespace LTL {

    /**
     * Implements on-the-fly version of Tarjan's algorithm suitable for LTL model checking.
     * The idea is to detect some reachable strongly connected component containing an accepting state,
     * which constitutes a counter-example.
     * For more details see
     * <p>
     *   Jaco Geldenhuys & Antti Valmari
     *   More efficient on-the-fly LTL verification with Tarjan's algorithm
     *   https://doi.org/10.1016/j.tcs.2005.07.004
     * </p>
     * @tparam SaveTrace whether to save and print counter-examples when possible.
     */
    template<template <typename, typename...> typename ProductSucGen, typename SuccessorGen, bool SaveTrace = false, typename... Spooler>
    class TarjanModelChecker : public ModelChecker<ProductSucGen, SuccessorGen, Spooler...> {
    public:
        TarjanModelChecker(const PetriEngine::PetriNet *net, const PetriEngine::PQL::Condition_ptr &cond,
                           const Structures::BuchiAutomaton &buchi,
                           SuccessorGen *successorGen,
                           int kbound,
                           std::unique_ptr<Spooler> &&...spooler)
                : ModelChecker<ProductSucGen, SuccessorGen, Spooler...>(net, cond, buchi, successorGen, nullptr, std::move(spooler)...),
                  _seen(net, kbound)
        {
            if (buchi._buchi->num_states() > 65535) {
                std::cerr << "Fatal error: cannot handle Büchi automata larger than 2^16 states\n";
                exit(1);
            }
            _chash.fill(std::numeric_limits<idx_t>::max());
        }

        bool isSatisfied() override;

        void printStats(std::ostream &os) override
        {
            this->_printStats(os, _seen);
        }

    private:
        using State = LTL::Structures::ProductState;
        using idx_t = size_t;
        // 64 MB hash table
        static constexpr idx_t _hash_sz = 16777216;


        using StateSet = std::conditional_t<SaveTrace, LTL::Structures::TraceableBitProductStateSet<>, LTL::Structures::BitProductStateSet<>>;
        static constexpr bool _is_spooling = std::is_same_v<SuccessorGen, SpoolingSuccessorGenerator>;

        StateSet _seen;
        std::unordered_set<idx_t> _store;

        // rudimentary hash table of state IDs. chash[hash(state)] is the top index in cstack
        // corresponding to state. Collisions are resolved using linked list via CEntry::next.
        std::array<idx_t, _hash_sz> _chash;
        static_assert(sizeof(_chash) == (1U << 27U));

        static inline idx_t hash(idx_t id)
        {
            return id % _hash_sz;
        }

        struct PlainCEntry {
            idx_t _lowlink;
            idx_t _stateid;
            idx_t _next = std::numeric_limits<idx_t>::max();
            bool _dstack = true;

            PlainCEntry(idx_t lowlink, idx_t stateid, idx_t next) : _lowlink(lowlink), _stateid(stateid), _next(next) {}
        };

        struct TracableCEntry : PlainCEntry {
            idx_t _lowsource = std::numeric_limits<idx_t>::max();
            idx_t _sourcetrans;

            TracableCEntry(idx_t lowlink, idx_t stateid, idx_t next) : PlainCEntry(lowlink, stateid, next) {}
        };

        using CEntry = std::conditional_t<SaveTrace,
                TracableCEntry,
                PlainCEntry>;

        struct DEntry {
            idx_t _pos; // position in cstack.

            typename SuccessorGen::successor_info_t _sucinfo;

            explicit DEntry(idx_t pos) : _pos(pos), _sucinfo(SuccessorGen::initial_suc_info()) {}
        };

        // master list of state information.
        std::vector<CEntry> _cstack;
        // depth-first search stack, contains current search path.
        std::stack<DEntry> _dstack;
        // cstack positions of accepting states in current search path, for quick access.
        std::stack<idx_t> _astack;

        bool _violation = false;
        bool _invariant_loop = true;
        size_t _loop_state = std::numeric_limits<size_t>::max();
        size_t _loop_trans = std::numeric_limits<size_t>::max();

        void push(State &state, size_t stateid);

        void pop();

        void update(idx_t to);

        bool nexttrans(State &state, State &parent, DEntry &delem);

        void popCStack();

        void printTrace(std::stack<DEntry> &&dstack, std::ostream &os = std::cout);
    };

    extern template
    class TarjanModelChecker<ProductSuccessorGenerator, LTL::ResumingSuccessorGenerator, true>;

    extern template
    class TarjanModelChecker<ProductSuccessorGenerator, LTL::ResumingSuccessorGenerator, false>;

    extern template
    class TarjanModelChecker<ProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, true>;

    extern template
    class TarjanModelChecker<ProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, false>;

    extern template
    class TarjanModelChecker<ReachStubProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, true, VisibleLTLStubbornSet>;

    extern template
    class TarjanModelChecker<ReachStubProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, false, VisibleLTLStubbornSet>;

    extern template
    class TarjanModelChecker<ReachStubProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, true, EnabledSpooler>;

    extern template
    class TarjanModelChecker<ReachStubProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, false, EnabledSpooler>;
}

#endif //VERIFYPN_TARJANMODELCHECKER_H
