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

#include "LTL/Algorithm/ModelChecker.h"

#include <utility>

namespace LTL {
    ModelChecker::ModelChecker(const PetriEngine::PetriNet& net, PetriEngine::PQL::Condition_ptr condition, const bool shortcircuitweak)
        : net(net), formula(condition), shortcircuitweak(shortcircuitweak)
    {

        successorGenerator = std::make_unique<ProductSuccessorGenerator>(net, condition);
        //TODO Create successor generator from net and condition

        //LTL::ProductPrinter::printProduct(*successorGenerator, std::cout, net, condition);
    }
}

