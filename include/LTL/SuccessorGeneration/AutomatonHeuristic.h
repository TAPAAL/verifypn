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

#ifndef VERIFYPN_AUTOMATONHEURISTIC_H
#define VERIFYPN_AUTOMATONHEURISTIC_H

#include "LTL/Structures/BuchiAutomaton.h"
#include "LTL/SuccessorGeneration/Heuristic.h"
#include "LTL/Structures/GuardInfo.h"

namespace LTL {
    class AutomatonHeuristic : public Heuristic {
    public:
        AutomatonHeuristic(const PetriEngine::PetriNet *net, const Structures::BuchiAutomaton &aut);

        uint32_t eval(const Structures::ProductState &state, uint32_t tid) override;

        bool has_heuristic(const Structures::ProductState &state) override;

        std::ostream &output(std::ostream &os) {
            return os << "AUTOMATON_HEUR";
        }

    protected:
        const PetriEngine::PetriNet *_net;
        const LTL::Structures::BuchiAutomaton &_aut;
        std::vector<guard_info_t> _state_guards;

        std::vector<uint32_t> _bfs_dists;
    };
}

#endif //VERIFYPN_AUTOMATONHEURISTIC_H
