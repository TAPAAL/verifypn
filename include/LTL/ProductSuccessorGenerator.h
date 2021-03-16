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

#ifndef VERIFYPN_PRODUCTSUCCESSORGENERATOR_H
#define VERIFYPN_PRODUCTSUCCESSORGENERATOR_H

#include "PetriEngine/SuccessorGenerator.h"
#include "PetriEngine/ReducingSuccessorGenerator.h"
#include "PetriEngine/PQL/PQL.h"
#include "LTL/Structures/ProductState.h"
#include "LTL/BuchiSuccessorGenerator.h"
#include "LTL/LTLToBuchi.h"
#include "LTL/Stubborn/LTLStubbornSet.h"


namespace LTL {

    template<class SuccessorGen>
    class ProductSuccessorGenerator {
    public:

        ProductSuccessorGenerator(const PetriEngine::PetriNet &net,
                                  const PetriEngine::PQL::Condition_ptr &cond,
                                  const SuccessorGen &successorGen)
                : successorGenerator(successorGen), _net(net), buchi(makeBuchiAutomaton(cond)) {}

        [[nodiscard]] size_t initial_buchi_state() const { return buchi.initial_state_number(); };

        bool prepare(const LTL::Structures::ProductState *state)
        {
            buchi.prepare(state->getBuchiState());
            buchi_parent = state->getBuchiState();
            fresh_marking = true;
            return successorGenerator.prepare(state);
        }

        bool next(LTL::Structures::ProductState &state)
        {
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

        bool isAccepting(const LTL::Structures::ProductState &state)
        {
            return buchi.is_accepting(state.getBuchiState());
        }

        void makeInitialState(std::vector<LTL::Structures::ProductState> &states)
        {
            auto buf = new PetriEngine::MarkVal[_net.numberOfPlaces() + 1];
            std::copy(_net.initial(), _net.initial() + _net.numberOfPlaces(), buf);
            buf[_net.numberOfPlaces()] = 0;
            LTL::Structures::ProductState state;
            state.setMarking(buf, _net.numberOfPlaces());
            state.setBuchiState(initial_buchi_state());
            buchi.prepare(state.getBuchiState());
            while (next_buchi_succ(state)) {
                states.emplace_back();
                states.back().setMarking(new PetriEngine::MarkVal[_net.numberOfPlaces() + 1], _net.numberOfPlaces());
                std::copy(state.marking(), state.marking() + _net.numberOfPlaces(), states.back().marking());
                states.back().setBuchiState(state.getBuchiState());
            }
        }

        [[nodiscard]] bool isInitialState(const LTL::Structures::ProductState &state) const
        {
            return state.markingEqual(_net.initial());
        }

        /**
         * prepare a state for successor generation, starting from specific point in iteration
         * @param state the source state to generate successors from
         * @param sucinfo the point in the iteration to start from, as returned by `next`.
         */
        void prepare(const LTL::Structures::ProductState *state, const typename SuccessorGen::sucinfo &sucinfo)
        {
            successorGenerator.prepare(state, sucinfo);
            buchi.prepare(state->getBuchiState());
            buchi_parent = state->getBuchiState();
            fresh_marking = sucinfo.fresh(); //
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

        /**
         * compute the next successor from the last state that was sent to `prepare`.
         * @param[out] state the state to write
         * @param[out] sucinfo checkpoint information from which iteration can later be resumed.
         * @return `true` if a successor was successfully generated, `false` otherwise.
         * @warning do not use the same State for both prepare and next, this will cause wildly incorrect behaviour!
         */
        bool next(Structures::ProductState &state, typename SuccessorGen::sucinfo &sucinfo)
        {
            if (fresh_marking) {
                fresh_marking = false;
                if (!successorGenerator.next(state, sucinfo)) {
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
                while (successorGenerator.next(state, sucinfo)) {
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


        [[nodiscard]] bool is_weak() const
        {
            return buchi.is_weak();
        }

        size_t last_transition() const { return successorGenerator.last_transition(); }

        size_t fired() const { return successorGenerator.fired(); }

        //template<typename T = std::enable_if_t<std::is_same_v<SuccessorGen, PetriEngine::ReducingSuccessorGenerator>, void>>
        void generateAll()
        {
            if constexpr (std::is_same_v<SuccessorGen, PetriEngine::ReducingSuccessorGenerator>)
                successorGenerator.generateAll();
        }

        size_t numEnabled()
        {
            if constexpr (std::is_same_v<SuccessorGen, PetriEngine::ReducingSuccessorGenerator>)
                return successorGenerator.nenabled();
            return -1;
        }

        bool *enabled() const
        {
            if constexpr (std::is_same_v<SuccessorGen, PetriEngine::ReducingSuccessorGenerator>) {
                return successorGenerator.enabled();
            }
            return nullptr;
        };

        bool *stubborn() const
        {
            if constexpr (std::is_same_v<SuccessorGen, PetriEngine::ReducingSuccessorGenerator>) {
                return successorGenerator.stubborn();
            }
            return nullptr;
        };

        size_t buchiStates() { return buchi.buchiStates(); }


    private:
        SuccessorGen successorGenerator;
        const PetriEngine::PetriNet &_net;

        BuchiSuccessorGenerator buchi;
        bdd cond;
        size_t buchi_parent;
        bool fresh_marking = true;

        /**
         * Evaluate binary decision diagram (BDD) representation of transition guard in given state.
         */
        bool guard_valid(const PetriEngine::Structures::State &state, bdd bdd)
        {
            EvaluationContext ctx{state.marking(), &_net};
            // IDs 0 and 1 are false and true atoms, respectively
            // More details in buddy manual ( http://buddy.sourceforge.net/manual/main.html )
            while (bdd.id() > 1) {
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

        bool next_buchi_succ(LTL::Structures::ProductState &state)
        {
            size_t tmp;
            while (buchi.next(tmp, cond)) {
                if (guard_valid(state, cond)) {
                    state.setBuchiState(tmp);
                    return true;
                }
            }
            return false;
        }
    };

}


#endif //VERIFYPN_PRODUCTSUCCESSORGENERATOR_H
