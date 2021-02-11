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


#include "LTL/ProductSuccessorGenerator.h"

/*
namespace LTL {
    template<typename SuccessorGen>
    void ProductSuccessorGenerator<SuccessorGen>::prepare(const LTL::Structures::ProductState *state) {
        successorGenerator.prepare(state);
        buchi.prepare(state->getBuchiState());
        buchi_parent = state->getBuchiState();
        fresh_marking = true;
    }

    template<typename SuccessorGen>
    void ProductSuccessorGenerator<SuccessorGen>::prepare(const LTL::Structures::ProductState *state,
                                                          const PetriEngine::successor_info &sucinfo) {
        successorGenerator.prepare(state, sucinfo);
        buchi.prepare(state->getBuchiState());
        buchi_parent = state->getBuchiState();
        fresh_marking = sucinfo.pcounter == 0 && sucinfo.tcounter == std::numeric_limits<uint32_t>::max();
        if (!fresh_marking) {
            assert(sucinfo.buchi_state != std::numeric_limits<size_t>::max());
            // spool Büchi successors until last state found.
            // TODO is there perhaps a good way to avoid this, perhaps using raw edge vector?
            // Caveat: it seems like there usually are not that many successors, so this is probably cheap regardless
            size_t tmp;
            while (buchi.next(tmp, cond)) {
                if (tmp == sucinfo.buchi_state) {
                    break;
                }
            }
        }
    }

    template<typename SuccessorGen>
    bool ProductSuccessorGenerator<SuccessorGen>::next(LTL::Structures::ProductState &state) {
        if (fresh_marking) {
            fresh_marking = false;
            if (!successorGenerator.next(state)) {
                // This is a fresh marking, so if there is no more successors for the state the state is deadlocked.
                // The semantics for deadlock is to just loop the marking so return true without changing the value of state.
                std::copy(successorGenerator.parent(), successorGenerator.parent() + state.buchi_state_idx + 1,
                          state.marking());
            }
        }
        if (next_buchi_succ(state)) {
            return true;
        }
            // No valid transition in Büchi automaton for current marking;
            // Try next marking(s) and see if we find a successor.
        else {
            while (successorGenerator.next(state)) {
                // reset buchi successors
                buchi.prepare(buchi_parent);
                if (next_buchi_succ(state)) {
                    return true;
                }
            }
            return false;
        }
    }

    template<typename SuccessorGen>
    bool ProductSuccessorGenerator<SuccessorGen>::isAccepting(const LTL::Structures::ProductState &state) {
        return buchi.is_accepting(state.getBuchiState());
    }

    template<typename SuccessorGen>
    bool ProductSuccessorGenerator<SuccessorGen>::next_buchi_succ(LTL::Structures::ProductState &state) {
        size_t tmp;
        while (buchi.next(tmp, cond)) {
            if (guard_valid(state, cond)) {
                state.setBuchiState(tmp);
                return true;
            }
        }
        return false;
    }

    template<typename SuccessorGen>
    bool ProductSuccessorGenerator<SuccessorGen>::guard_valid(const PetriEngine::Structures::State &state, bdd bdd) {
        EvaluationContext ctx{state.marking(), &_net};
        // IDs 0 and 1 are false and true atoms, respectively
        while (bdd.id() > 1*/
/*!(bdd == bddtrue || bdd == bddfalse)*//*
) {
            // find variable to test, and test it
            size_t var = bdd_var(bdd);
            Condition::Result res = buchi.getExpression(var)->evaluate(ctx);
            switch (res) {
                case Condition::RUNKNOWN:
                    std::cerr << "Unexpected unknown answer from evaluating query!\n";
                    assert(false);
                    exit(1);
                    break;
                case Condition::RFALSE:
                    bdd = bdd_low(bdd);
                    break;
                case Condition::RTRUE:
                    bdd = bdd_high(bdd);
                    break;
            }
        }
        return bdd == bddtrue;
    }

    template<typename SuccessorGen>
    bool ProductSuccessorGenerator<SuccessorGen>::next(Structures::ProductState &state,
                                                       PetriEngine::successor_info &sucinfo) {
        if (fresh_marking) {
            fresh_marking = false;
            if (!successorGenerator.next(state)) {
                // This is a fresh marking, so if there are no more successors for the state the state is deadlocked.
                // The semantics for deadlock is to just loop the marking so return true without changing the value of state.
                std::copy(successorGenerator.parent(), successorGenerator.parent() + state.buchi_state_idx + 1,
                          state.marking());
            }
        }
        if (next_buchi_succ(state)) {
            successorGenerator.getSuccInfo(sucinfo);
            sucinfo.buchi_state = state.getBuchiState();
            return true;
        }
            // No valid transition in Büchi automaton for current marking;
            // Try next marking(s) and see if we find a successor.
        else {
            while (successorGenerator.next(state)) {
                // reset buchi successors
                buchi.prepare(buchi_parent);
                if (next_buchi_succ(state)) {
                    successorGenerator.getSuccInfo(sucinfo);
                    sucinfo.buchi_state = state.getBuchiState();
                    return true;
                }
            }
            return false;
        }
    }

}
*/

namespace LTL {
    template
    class ProductSuccessorGenerator<PetriEngine::SuccessorGenerator>;

    template
    class ProductSuccessorGenerator<PetriEngine::ReducingSuccessorGenerator>;
}