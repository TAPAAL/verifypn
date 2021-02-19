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
#include "PetriEngine/PQL/PQL.h"
#include "LTL/Structures/ProductState.h"
#include "LTL/BuchiSuccessorGenerator.h"
#include "LTL/LTLToBuchi.h"


namespace LTL {
    /**
     * type holding sufficient information to resume successor generation for a state from a given point.
     */
    struct successor_info {
        uint32_t pcounter;
        uint32_t tcounter;
        size_t buchi_state;
        size_t last_state;

        friend bool operator==(const successor_info &lhs, const successor_info &rhs) {
            return lhs.pcounter == rhs.pcounter &&
                   lhs.tcounter == rhs.tcounter &&
                   lhs.buchi_state == rhs.buchi_state &&
                   lhs.last_state == rhs.last_state;
        }

        friend bool operator!=(const successor_info &lhs, const successor_info &rhs) {
            return !(rhs == lhs);
        }

        inline bool has_pcounter() const {
            return pcounter != NoPCounter;
        }

        inline bool has_tcounter() const {
            return tcounter != NoTCounter;
        }

        inline bool has_buchistate() const {
            return buchi_state != NoBuchiState;
        }

        inline bool has_prev_state() const {
            return last_state != NoLastState;
        }

        static constexpr auto NoPCounter = 0;
        static constexpr auto NoTCounter = std::numeric_limits<uint32_t>::max();
        static constexpr auto NoBuchiState = std::numeric_limits<size_t>::max();
        static constexpr auto NoLastState = std::numeric_limits<size_t>::max();
    };

    constexpr successor_info initial_suc_info{
            successor_info::NoPCounter,
            successor_info::NoTCounter,
            successor_info::NoBuchiState,
            successor_info::NoLastState
    };

    class ProductSuccessorGenerator : public PetriEngine::SuccessorGenerator {
    public:

        ProductSuccessorGenerator(const PetriEngine::PetriNet &net,
                                  const PetriEngine::PQL::Condition_ptr &cond)
                : PetriEngine::SuccessorGenerator(net), buchi(makeBuchiAutomaton(cond)) {}

        [[nodiscard]] size_t initial_buchi_state() const { return buchi.initial_state_number(); };

        void prepare(const LTL::Structures::ProductState *state);

        bool next(LTL::Structures::ProductState &state);

        bool isAccepting(const LTL::Structures::ProductState &state);

        void makeInitialState(std::vector<LTL::Structures::ProductState> &states) {
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

        [[nodiscard]] bool isInitialState(const LTL::Structures::ProductState &state) const {
            return state.markingEqual(_net.initial());
        }

        /**
         * prepare a state for successor generation, starting from specific point in iteration
         * @param state the source state to generate successors from
         * @param sucinfo the point in the iteration to start from, as returned by `next`.
         */
        void prepare(const LTL::Structures::ProductState *state, const successor_info &sucinfo);

        /**
         * compute the next successor from the last state that was sent to `prepare`.
         * @param[out] state the state to write
         * @param[out] sucinfo checkpoint information from which iteration can later be resumed.
         * @return `true` if a successor was successfully generated, `false` otherwise.
         * @warning do not use the same State for both prepare and next, this will cause wildly incorrect behaviour!
         */
        bool next(Structures::ProductState &state, successor_info &sucinfo);

        [[nodiscard]] bool is_weak() const {
            return buchi.is_weak();
        }

    private:
        BuchiSuccessorGenerator buchi;
        bdd cond;
        size_t buchi_parent;
        bool fresh_marking = true;

        bool guard_valid(const PetriEngine::Structures::State &state, bdd bdd);

        bool next_buchi_succ(LTL::Structures::ProductState &state);
    };

}

#endif //VERIFYPN_PRODUCTSUCCESSORGENERATOR_H
