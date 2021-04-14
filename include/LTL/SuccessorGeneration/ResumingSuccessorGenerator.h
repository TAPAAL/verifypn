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
    /**
     * type holding sufficient information to resume successor generation for a state from a given point.
     */
    struct successor_info {
        uint32_t pcounter;
        uint32_t tcounter;
        size_t buchi_state;
        size_t last_state;

        friend bool operator==(const successor_info &lhs, const successor_info &rhs)
        {
            return lhs.pcounter == rhs.pcounter &&
                   lhs.tcounter == rhs.tcounter &&
                   lhs.buchi_state == rhs.buchi_state &&
                   lhs.last_state == rhs.last_state;
        }

        friend bool operator!=(const successor_info &lhs, const successor_info &rhs)
        {
            return !(rhs == lhs);
        }

        inline bool has_pcounter() const
        {
            return pcounter != NoPCounter;
        }

        inline bool has_tcounter() const
        {
            return tcounter != NoTCounter;
        }

        inline bool has_buchistate() const
        {
            return buchi_state != NoBuchiState;
        }

        inline bool has_prev_state() const
        {
            return last_state != NoLastState;
        }

        [[nodiscard]] inline bool fresh() const
        {
            return pcounter == NoPCounter && tcounter == NoTCounter;
        }

        static constexpr auto NoPCounter = 0;
        static constexpr auto NoTCounter = std::numeric_limits<uint32_t>::max();
        static constexpr auto NoBuchiState = std::numeric_limits<size_t>::max();
        static constexpr auto NoLastState = std::numeric_limits<size_t>::max();
    };

    class ResumingSuccessorGenerator : public PetriEngine::SuccessorGenerator {
    public:

        ResumingSuccessorGenerator(const PetriEngine::PetriNet *net);

        ResumingSuccessorGenerator(const PetriEngine::PetriNet *net, const std::shared_ptr<PetriEngine::StubbornSet> &);

        ResumingSuccessorGenerator(const PetriEngine::PetriNet *net,
                                   std::vector<std::shared_ptr<PetriEngine::PQL::Condition> > &queries);

        ResumingSuccessorGenerator(const PetriEngine::PetriNet *net,
                                   const std::shared_ptr<PetriEngine::PQL::Condition> &query);

        ~ResumingSuccessorGenerator() override = default;

        void prepare(const PetriEngine::Structures::State *state, const successor_info &sucinfo);


        bool next(PetriEngine::Structures::State &write, successor_info &sucinfo)
        {
            bool has_suc = PetriEngine::SuccessorGenerator::next(write);
            getSuccInfo(sucinfo);
            return has_suc;
        }

        uint32_t fired() const
        {
            return _suc_tcounter - 1;
        }

        const PetriEngine::MarkVal *getParent() const
        {
            return _parent->marking();
        }


        size_t last_transition() const
        {
            return _suc_tcounter == std::numeric_limits<uint32_t>::max() ? std::numeric_limits<uint32_t>::max() :
                   _suc_tcounter - 1;
        }

        using sucinfo = successor_info;

        static constexpr sucinfo _initial_suc_info{
                successor_info::NoPCounter,
                successor_info::NoTCounter,
                successor_info::NoBuchiState,
                successor_info::NoLastState
        };

        static constexpr auto initial_suc_info() { return _initial_suc_info; }

    private:
        void getSuccInfo(successor_info &sucinfo) const;

        //friend class ReducingSuccessorGenerator;
    };
}

#endif /* RESUMINGSUCCESSORGENERATOR_H */

