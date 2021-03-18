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

#ifndef VERIFYPN_SPOOLINGSUCCESSORGENERATOR_H
#define VERIFYPN_SPOOLINGSUCCESSORGENERATOR_H

#include "LTL/Structures/ProductState.h"
#include "PetriEngine/Structures/SuccessorQueue.h"

namespace LTL {
    class SpoolingSuccessorGenerator : public PetriEngine::SuccessorGenerator {
    public:
        SpoolingSuccessorGenerator(const PetriEngine::PetriNet &net, const Condition_ptr &)
                : SuccessorGenerator(net), _transbuf(new uint32_t[net.numberOfTransitions()])
        {
            _markbuf.setMarking(new PetriEngine::MarkVal[net.numberOfPlaces()]);
        }

        struct sucinfo {
            SuccessorQueue successors;
            size_t buchi_state;
            size_t last_state;

            sucinfo(size_t buchiState, size_t lastState) : buchi_state(buchiState), last_state(lastState) {}

            inline bool has_prev_state() const {
                return last_state != NoLastState;
            }

            [[nodiscard]] bool fresh() const { return buchi_state == NoBuchiState && last_state == NoLastState; }

            static constexpr auto NoBuchiState = std::numeric_limits<size_t>::max();
            static constexpr auto NoLastState = std::numeric_limits<size_t>::max();
        };

        [[nodiscard]] static sucinfo initial_suc_info() {
            return sucinfo{sucinfo::NoBuchiState, sucinfo::NoLastState};
        }

        bool prepare(const PetriEngine::Structures::State *state)
        {
            return PetriEngine::SuccessorGenerator::prepare(state);
        }

        bool next(PetriEngine::Structures::State &write)
        {
            return PetriEngine::SuccessorGenerator::next(write);
        }


        void prepare(const Structures::ProductState *state, sucinfo &sucinfo)
        {
            PetriEngine::SuccessorGenerator::prepare(state);
            if (!sucinfo.successors.valid()) {
                uint32_t nsuc = 0;
                // generate list of transitions that generate a successor.
                while (PetriEngine::SuccessorGenerator::next(_markbuf)) {
                    auto t = fired();
                    assert(t <= _net.numberOfTransitions());
                    _transbuf[nsuc++] = t;
                }
                //TODO Perform heuristic before creating SuccessorQueue
                sucinfo.successors = SuccessorQueue(_transbuf.get(), nsuc);
            }
        }

        bool next(Structures::ProductState &state, sucinfo &sucinfo)
        {
            if (sucinfo.successors.empty()) {
                return false;
            }
            auto t = sucinfo.successors.front();
            sucinfo.successors.pop();
            _fire(state, t);
            return true;
        }

    private:
        std::unique_ptr<uint32_t[]> _transbuf;
        PetriEngine::Structures::State _markbuf;
    };
}
#endif //VERIFYPN_SPOOLINGSUCCESSORGENERATOR_H
