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
#include "LTL/Structures/ProductStateFactory.h"

#include <ptrie/ptrie_stable.h>
#include <unordered_set>

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
     * For most use cases, Tarjan's algorithm (see LTL::TarjanModelChecker) is faster.
     * @tparam W type used for state storage. Use <code>PetriEngine::Structures::TracableStateSet</code> if you want traces,
     *         <code>PetriEngine::Structures::StateSet</code> if you don't care (as it is faster).
     */
    template<typename SucGen, typename W>
class NestedDepthFirstSearch : public ModelChecker<ProductSuccessorGenerator, SucGen> {
    public:
        NestedDepthFirstSearch(const PetriEngine::PetriNet *net, const PetriEngine::PQL::Condition_ptr &query,
                               const Structures::BuchiAutomaton &buchi, SucGen *gen, int kbound)
                : ModelChecker<ProductSuccessorGenerator, SucGen>(net, query, buchi, gen),
                  factory{net, this->successorGenerator->initial_buchi_state()},
                  states(net, kbound) {}

        bool isSatisfied() override;

        void printStats(std::ostream &os) override;

    private:
        using State = LTL::Structures::ProductState;

        Structures::ProductStateFactory factory;
        W states; //StateSet
        std::set<size_t> mark1;
        std::set<size_t> mark2;

        struct StackEntry {
            size_t id;
            typename SucGen::sucinfo sucinfo;
        };

        State *seed;
        bool violation = false;

        //Used for printing the trace
        std::stack<std::pair<size_t, size_t>> nested_transitions;

        void dfs();

        void ndfs(State &state);

        void printTrace(std::stack<std::pair<size_t, size_t>> &transitions, std::ostream &os = std::cout);

        static constexpr bool SaveTrace = std::is_same_v<W, PetriEngine::Structures::TracableStateSet>;
    };

    extern template
    class NestedDepthFirstSearch<LTL::ResumingSuccessorGenerator, LTL::Structures::BitProductStateSet<>>;

    extern template
    class NestedDepthFirstSearch<LTL::ResumingSuccessorGenerator, LTL::Structures::TraceableBitProductStateSet<>>;

    extern template
    class NestedDepthFirstSearch<LTL::SpoolingSuccessorGenerator, LTL::Structures::BitProductStateSet<>>;

    extern template
    class NestedDepthFirstSearch<LTL::SpoolingSuccessorGenerator, LTL::Structures::TraceableBitProductStateSet<>>;
}

#endif //VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H
