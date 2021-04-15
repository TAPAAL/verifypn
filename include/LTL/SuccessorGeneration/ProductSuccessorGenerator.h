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
#include "BuchiSuccessorGenerator.h"
#include "LTL/LTLToBuchi.h"
#include "LTL/Stubborn/VisibleLTLStubbornSet.h"
#include "LTL/Simplification/SpotToPQL.h"
#include "LTL/Structures/GuardInfo.h"
#include "LTL/SuccessorGeneration/SpoolingSuccessorGenerator.h"

#include <spot/twa/formula2bdd.hh>
#include <spot/tl/formula.hh>

namespace LTL {

    template<class SuccessorGen>
    class ProductSuccessorGenerator {
    public:

        ProductSuccessorGenerator(const PetriEngine::PetriNet *net,
                                  const Structures::BuchiAutomaton &buchi,
                                  SuccessorGen &&successorGen)
                : _successor_generator(std::move(successorGen)), _net(net),
                  buchi(buchi), aut(buchi)
        {

        }

        [[nodiscard]] size_t initial_buchi_state() const { return buchi.initial_state_number(); };

        auto prepare(const LTL::Structures::ProductState *state)
        {
            buchi.prepare(state->getBuchiState());
            buchi_parent = state->getBuchiState();
            fresh_marking = true;
            return _successor_generator.prepare(state);
        }

        bool next(LTL::Structures::ProductState &state)
        {
            if (fresh_marking) {
                fresh_marking = false;
                if (!_successor_generator.next(state)) {
                    // This is a fresh marking, so if there is no more successors for the state the state is deadlocked.
                    // The semantics for deadlock is to just loop the marking so return true without changing the value of state.
                    std::copy(_successor_generator.getParent(), _successor_generator.getParent() + state.buchi_state_idx + 1,
                              state.marking());
                }
            }
            if (next_buchi_succ(state)) {
                return true;
            }
                // No valid transition in Büchi automaton for current marking;
                // Try next marking(s) and see if we find a successor.
            else {
                while (_successor_generator.next(state)) {
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

        std::vector<LTL::Structures::ProductState> makeInitialState()
        {
            std::vector<LTL::Structures::ProductState> states;
            auto buf = new PetriEngine::MarkVal[_net->numberOfPlaces() + 1];
            std::copy(_net->initial(), _net->initial() + _net->numberOfPlaces(), buf);
            buf[_net->numberOfPlaces()] = initial_buchi_state();
            LTL::Structures::ProductState state;
            state.setMarking(buf, _net->numberOfPlaces());
            //state.setBuchiState(initial_buchi_state());
            buchi.prepare(state.getBuchiState());
            while (next_buchi_succ(state)) {
                states.emplace_back();
                states.back().setMarking(new PetriEngine::MarkVal[_net->numberOfPlaces() + 1], _net->numberOfPlaces());
                std::copy(state.marking(), state.marking() + _net->numberOfPlaces(), states.back().marking());
                states.back().setBuchiState(state.getBuchiState());
            }
            return states;
        }

        [[nodiscard]] bool isInitialState(const LTL::Structures::ProductState &state) const
        {
            return state.markingEqual(_net->initial());
        }

        /**
         * prepare a state for successor generation, starting from specific point in iteration
         * @param state the source state to generate successors from
         * @param sucinfo the point in the iteration to start from, as returned by `next`.
         */
        void prepare(const LTL::Structures::ProductState *state, typename SuccessorGen::sucinfo &sucinfo)
        {
            _successor_generator.prepare(state, sucinfo);
            fresh_marking = sucinfo.fresh();
            buchi.prepare(state->getBuchiState());
            buchi_parent = state->getBuchiState();
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
                if (!_successor_generator.next(state, sucinfo)) {
                    // This is a fresh marking, so if there are no more successors for the state the state is deadlocked.
                    // The semantics for deadlock is to just loop the marking so return true without changing the value of state.
                    std::copy(_successor_generator.getParent(), _successor_generator.getParent() + state.buchi_state_idx + 1,
                              state.marking());
                }
            }
            if (next_buchi_succ(state)) {
                //_successor_generator.getSuccInfo(sucinfo);
                sucinfo.buchi_state = state.getBuchiState();
                return true;
            }
                // No valid transition in Büchi automaton for current marking;
                // Try next marking(s) and see if we find a successor.
            else {
                while (_successor_generator.next(state, sucinfo)) {
                    // reset buchi successors
                    buchi.prepare(buchi_parent);
                    if (next_buchi_succ(state)) {
                        //_successor_generator.getSuccInfo(sucinfo);
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

        size_t last_transition() const { return _successor_generator.last_transition(); }

        size_t fired() const { return _successor_generator.fired(); }

        //template<typename T = std::enable_if_t<std::is_same_v<SuccessorGen, PetriEngine::ReducingSuccessorGenerator>, void>>
        void generateAll(typename SuccessorGen::sucinfo &sucinfo)
        {
            if constexpr (std::is_same_v<SuccessorGen, LTL::SpoolingSuccessorGenerator>) {
                _successor_generator.generate_all(sucinfo);
            }
        }

        size_t numEnabled()
        {
            if constexpr (std::is_same_v<SuccessorGen, LTL::SpoolingSuccessorGenerator>) {
                return _successor_generator.nenabled();
            }
            return -1;
        }

        // FIXME const if possible?
        bool *enabled() const
        {
            if constexpr (std::is_same_v<SuccessorGen, LTL::SpoolingSuccessorGenerator>) {
                return _successor_generator.enabled();
            }
            return nullptr;
        };

        // FIXME const if possible?
        bool *stubborn() const
        {
            if constexpr (std::is_same_v<SuccessorGen, LTL::SpoolingSuccessorGenerator>) {
                return _successor_generator.stubborn();
            }
            return nullptr;
        };

        size_t buchiStates() { return buchi.buchiStates(); }

        void push() {
            if constexpr (std::is_same_v<SuccessorGen, LTL::SpoolingSuccessorGenerator>) {
                _successor_generator.push();
            }
        }

        void pop(const typename SuccessorGen::sucinfo &sucinfo) {
            if constexpr (std::is_same_v<SuccessorGen, LTL::SpoolingSuccessorGenerator>) {
                _successor_generator.pop(sucinfo);
            }
        }

    private:
        SuccessorGen _successor_generator;
        const PetriEngine::PetriNet *_net;

        BuchiSuccessorGenerator buchi;
        const LTL::Structures::BuchiAutomaton &aut;
        bdd cond;
        size_t buchi_parent;
        bool fresh_marking = true;

        std::vector<GuardInfo> stateToGuards;

        /**
         * Evaluate binary decision diagram (BDD) representation of transition guard in given state.
         */
        bool guard_valid(const PetriEngine::Structures::State &state, bdd bdd)
        {
            PetriEngine::PQL::EvaluationContext ctx{state.marking(), _net};
            return buchi.aut.guard_valid(ctx, bdd);
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
