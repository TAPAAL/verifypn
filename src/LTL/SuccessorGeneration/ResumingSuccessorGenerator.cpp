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

#include "LTL/SuccessorGeneration/ResumingSuccessorGenerator.h"
#include "PetriEngine/Structures/State.h"
#include "utils/errors.h"

#include <cassert>

namespace LTL {
    using namespace PetriEngine;

    ResumingSuccessorGenerator::ResumingSuccessorGenerator(const PetriNet& net)
    : PetriEngine::SuccessorGenerator(net) { }

    ResumingSuccessorGenerator::ResumingSuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries) : ResumingSuccessorGenerator(net){}

    ResumingSuccessorGenerator::ResumingSuccessorGenerator(const PetriNet& net, const std::shared_ptr<PQL::Condition> &query)
                                           : ResumingSuccessorGenerator(net) {
    }

    ResumingSuccessorGenerator::ResumingSuccessorGenerator(const PetriNet& net, const std::shared_ptr<StubbornSet>&)
        : ResumingSuccessorGenerator(net){}

    void ResumingSuccessorGenerator::prepare(const Structures::State* state, const successor_info_t &sucinfo) {
        SuccessorGenerator::prepare(state);
        _suc_pcounter = sucinfo._pcounter;
        _suc_tcounter = sucinfo._tcounter;
    }

    void ResumingSuccessorGenerator::get_succ_info(successor_info_t &sucinfo) const {
        sucinfo._pcounter = _suc_pcounter;
        sucinfo._tcounter = _suc_tcounter;
    }



}

