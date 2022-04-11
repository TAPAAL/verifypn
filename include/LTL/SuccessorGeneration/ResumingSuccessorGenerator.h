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

#ifndef RESUMINGSUCCESSORGENERATOR_H
#define RESUMINGSUCCESSORGENERATOR_H

#include "PetriEngine/SuccessorGenerator.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/Structures/State.h"
#include <memory>
#include "PetriEngine/Stubborn/StubbornSet.h"

namespace LTL {

    class ResumingSuccessorGenerator : public PetriEngine::SuccessorGenerator {
    public:

        struct successor_info_t {
            uint32_t _pcounter;
            uint32_t _tcounter;
            size_t _buchi_state;
            size_t _last_state;

            friend bool operator==(const successor_info_t &lhs, const successor_info_t &rhs) {
                return  lhs._pcounter ==    rhs._pcounter &&
                        lhs._tcounter ==    rhs._tcounter &&
                        lhs._buchi_state == rhs._buchi_state &&
                        lhs._last_state ==  rhs._last_state;
            }

            friend bool operator!=(const successor_info_t &lhs, const successor_info_t &rhs) {
                return !(rhs == lhs);
            }

            bool has_pcounter() const {
                return _pcounter != NoPCounter;
            }

            bool has_tcounter() const {
                return _tcounter != NoTCounter;
            }

            bool has_buchistate() const {
                return _buchi_state != NoBuchiState;
            }

            bool has_prev_state() const {
                return _last_state != NoLastState;
            }

            size_t state() const {
                return _last_state;
            }

            size_t transition() const {
                return _tcounter - 1;
            }

            [[nodiscard]] bool fresh() const {
                return _pcounter == NoPCounter && _tcounter == NoTCounter;
            }

            static constexpr auto NoPCounter = 0;
            static constexpr auto NoTCounter = std::numeric_limits<uint32_t>::max();
            static constexpr auto NoBuchiState = std::numeric_limits<size_t>::max();
            static constexpr auto NoLastState = std::numeric_limits<size_t>::max();
        };
    public:

        ResumingSuccessorGenerator(const PetriEngine::PetriNet& net);

        ResumingSuccessorGenerator(const PetriEngine::PetriNet& net, const std::shared_ptr<PetriEngine::StubbornSet> &);

        ResumingSuccessorGenerator(const PetriEngine::PetriNet& net,
                std::vector<std::shared_ptr<PetriEngine::PQL::Condition> > &queries);

        ResumingSuccessorGenerator(const PetriEngine::PetriNet& net,
                const std::shared_ptr<PetriEngine::PQL::Condition> &query);

        ~ResumingSuccessorGenerator() override = default;

        void prepare(const PetriEngine::Structures::State *state, const successor_info_t &sucinfo);

        bool next(PetriEngine::Structures::State &write, successor_info_t &sucinfo) {
            bool has_suc = PetriEngine::SuccessorGenerator::next(write);
            get_succ_info(sucinfo);
            return has_suc;
        }

        uint32_t fired() const {
            return _suc_tcounter - 1;
        }

        const PetriEngine::MarkVal *get_parent() const {
            return _parent->marking();
        }

        size_t last_transition() const {
            return _suc_tcounter == std::numeric_limits<uint32_t>::max() ? std::numeric_limits<uint32_t>::max() :
                    _suc_tcounter - 1;
        }

        static constexpr successor_info_t _initial_suc_info{
            successor_info_t::NoPCounter,
            successor_info_t::NoTCounter,
            successor_info_t::NoBuchiState,
            successor_info_t::NoLastState};

        static constexpr auto initial_suc_info() {
            return _initial_suc_info;
        }

    private:
        void get_succ_info(successor_info_t &sucinfo) const;

        //friend class ReducingSuccessorGenerator;
    };
}

#endif /* RESUMINGSUCCESSORGENERATOR_H */

