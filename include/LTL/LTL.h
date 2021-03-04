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

#ifndef VERIFYPN_LTLALGORITHM_H
#define VERIFYPN_LTLALGORITHM_H

#include "LTL/LTLValidator.h"
#include "LTL/Algorithm/TarjanModelChecker.h"
#include "LTL/Algorithm/NestedDepthFirstSearch.h"
#include "LTL/Algorithm/RandomNDFS.h"
#include "LTL/Simplification/SpotToPQL.h"
#include "LTL/LTLToBuchi.h"

namespace LTL {


    std::pair<Condition_ptr, bool> to_ltl(const Condition_ptr &formula);

}

#endif