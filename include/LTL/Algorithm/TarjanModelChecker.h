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
#include "utils/structures/light_deque.h"

#include <ptrie/ptrie.h>

#include <limits>

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
     */
    class TarjanModelChecker : public ModelChecker {
    public:
        TarjanModelChecker(const PetriEngine::PetriNet& net, const PetriEngine::PQL::Condition_ptr &cond,
                           const Structures::BuchiAutomaton &buchi,
                           uint32_t kbound)
                : ModelChecker(net, cond, buchi), _k_bound(kbound)
        {
            if (buchi.buchi().num_states() > 1048576) {
                throw base_error("Cannot handle Büchi automata larger than 2^20 states");
            }
            _chash.fill(std::numeric_limits<idx_t>::max());
        }

        bool check() override;

        void print_stats(std::ostream &os) const override;

        virtual void set_partial_order(LTLPartialOrder);

        LTLPartialOrder used_partial_order() const {
            return _order;
        }
    private:

        template<typename SuccGen>
        bool select_trace_compute(SuccGen& successorGenerator);

        template<bool TRACE, typename SuccGen>
        bool compute(SuccGen& successorGenerator);

        using State = LTL::Structures::ProductState;
        using idx_t = size_t;
        // 64 MB hash table
        static constexpr idx_t _hash_sz = 16777216;

        ptrie::set<idx_t,17,32,8> _store;

        // rudimentary hash table of state IDs. chash[hash(state)] is the top index in cstack
        // corresponding to state. Collisions are resolved using linked list via CEntry::next.
        std::array<idx_t, _hash_sz> _chash;
        static_assert(sizeof(_chash) == (1U << 27U));

        static inline idx_t hash(idx_t buchi_state, idx_t marking_id)
        {
            return (buchi_state xor marking_id) % _hash_sz;
        }

        struct plain_centry_t {
            idx_t _lowlink;
            idx_t _stateid;
            idx_t _next = std::numeric_limits<idx_t>::max();
            bool _dstack = true;
            plain_centry_t(idx_t lowlink, idx_t stateid, idx_t next) : _lowlink(lowlink), _stateid(stateid), _next(next) {}
            static constexpr bool save_trace() { return false; }
        };

        struct tracable_centry_t : plain_centry_t {
            idx_t _lowsource = std::numeric_limits<idx_t>::max();
            idx_t _sourcetrans;
            tracable_centry_t(idx_t lowlink, idx_t stateid, idx_t next) : plain_centry_t(lowlink, stateid, next) {}
            static constexpr bool save_trace() { return true; }
        };

        template<typename T>
        struct dentry_t {
            idx_t _pos; // position in cstack.
            typename T::successor_info_t _sucinfo;
            explicit dentry_t(idx_t pos) : _pos(pos), _sucinfo(T::initial_suc_info()) {}
        };


        // cstack positions of accepting states in current search path, for quick access.
        light_deque<idx_t> _astack;

        bool _invariant_loop = true;
        size_t _loop_state = std::numeric_limits<size_t>::max();
        size_t _loop_trans = std::numeric_limits<size_t>::max();
        size_t _discoverd = std::numeric_limits<size_t>::max();
        size_t _max_tokens = std::numeric_limits<size_t>::max();
        uint32_t _k_bound;
        LTLPartialOrder _order = LTLPartialOrder::None;

        // TODO, instead of this template hell, we should really just have a templated state that we shuffle around.
        template<typename StateSet, typename T, typename D, typename S>
        void push(StateSet& s, light_deque<T>& cstack, light_deque<D>& dstack, S& successor_generator, State &state, size_t stateid);

        template<typename S, typename T, typename D, typename SuccGen>
        void pop(S& seen, light_deque<T>& cstack, light_deque<D>& dstack, SuccGen& successorGenerator);

        template<typename T, typename D, typename SuccGen>
        void update(light_deque<T>& cstack, light_deque<D>& d, SuccGen& successorGenerator, idx_t to);

        template<typename S, typename T, typename SuccGen, typename D>
        bool next_trans(S& seen, light_deque<T>& cstack, SuccGen& successorGenerator, State &state, State &parent, D &delem);

        template<typename StateSet, typename T>
        void popCStack(StateSet& s, light_deque<T>& cstack);

        template<typename S, typename D, typename C>
        void build_trace(S& seen, light_deque<D> &&dstack, light_deque<C>& cstack);
    };
}

#endif //VERIFYPN_TARJANMODELCHECKER_H
