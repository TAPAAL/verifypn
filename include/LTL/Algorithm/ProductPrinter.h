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

#ifndef VERIFYPN_PRODUCTPRINTER_H
#define VERIFYPN_PRODUCTPRINTER_H

#include "LTL/Structures/ProductStateFactory.h"
#include "LTL/ProductSuccessorGenerator.h"
#include "PetriEngine/Structures/StateSet.h"
#include "PetriEngine/Structures/Queue.h"
#include "PetriEngine/PQL/Contexts.h"

namespace LTL::ProductPrinter {
    void printProduct(ProductSuccessorGenerator &successorGenerator, std::ostream &os, const PetriEngine::PetriNet &net,
                      PetriEngine::PQL::Condition_ptr formula);
}
#endif //VERIFYPN_PRODUCTPRINTER_H
