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

#ifndef VERIFYPN_BUCHIAUTOMATON_H
#define VERIFYPN_BUCHIAUTOMATON_H

#include "LTL/LTLToBuchi.h"
#include <spot/twa/twagraph.hh>
#include <unordered_map>

namespace LTL::Structures {
    struct BuchiAutomaton {
        BuchiAutomaton(spot::twa_graph_ptr buchi, std::unordered_map<int, AtomicProposition> apInfo)
                : _buchi(std::move(buchi)), ap_info(std::move(apInfo)) {
            dict = _buchi->get_dict();
        }

        spot::twa_graph_ptr _buchi;
        const std::unordered_map<int, AtomicProposition> ap_info;
        spot::bdd_dict_ptr dict;
    };
}

#endif //VERIFYPN_BUCHIAUTOMATON_H
