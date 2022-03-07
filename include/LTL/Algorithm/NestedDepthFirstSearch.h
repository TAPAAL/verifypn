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

#ifndef VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H
#define VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H

#include "ModelChecker.h"
#include "PetriEngine/Structures/StateSet.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/Structures/Queue.h"
#include "utils/structures/light_deque.h"
#include "LTL/Structures/ProductStateFactory.h"

#include <ptrie/ptrie_map.h>

namespace LTL {

    /**
     * Implement the nested DFS algorithm for LTL model checking. Roughly based on versions given in
     * <p>
     *   Jaco Geldenhuys & Antti Valmari,<br>
     *   More efficient on-the-fly LTL verification with Tarjan's algorithm,<br>
     *   https://doi.org/10.1016/j.tcs.2005.07.004
     * </p>
     * and
     * <p>
     *   Gerard J. Holzmann, Doron Peled, and Mihalis Yannakakis<br>
     *   On Nested Depth First Search<br>
     *   https://spinroot.com/gerard/pdf/inprint/spin96.pdf
     * </p>
     */
    class NestedDepthFirstSearch : public ModelChecker {
    public:
        NestedDepthFirstSearch(const PetriEngine::PetriNet& net, const PetriEngine::PQL::Condition_ptr &query,
                               const Structures::BuchiAutomaton &buchi, uint32_t kbound)
                : ModelChecker(net, query, buchi), _states(net, kbound) {}

        virtual bool check();

        void print_stats(std::ostream &os) const override;

    private:
        using State = LTL::Structures::ProductState;
        std::pair<bool,size_t> mark(State& state, uint8_t);

        LTL::Structures::BitProductStateSet<ptrie::map<Structures::stateid_t, uint8_t>> _states;

        ptrie::map<size_t, uint8_t> _markers;
        static constexpr uint8_t MARKER1 = 1;
        static constexpr uint8_t MARKER2 = 2;
        size_t _mark_count[3] = {0,0,0};

        template<typename T>
        struct stack_entry_t {
            size_t _id;
            typename T::successor_info_t _sucinfo;
        };

        template<typename T>
        void dfs(ProductSuccessorGenerator<T>& successor_generator);

        template<typename T>
        void ndfs(ProductSuccessorGenerator<T>& successor_generator, const State &state, light_deque<stack_entry_t<T>>& nested_todo);

        template<typename T>
        void build_trace(light_deque<stack_entry_t<T>>& todo, light_deque<stack_entry_t<T>>& nested_todo);
    };

}

#endif //VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H
