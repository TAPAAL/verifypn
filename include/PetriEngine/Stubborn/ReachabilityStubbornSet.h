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

#ifndef VERIFYPN_REACHABILITYSTUBBORNSET_H
#define VERIFYPN_REACHABILITYSTUBBORNSET_H


#include "PetriEngine/Stubborn/StubbornSet.h"

namespace PetriEngine {
    class ReachabilityStubbornSet : public StubbornSet {
    public:
        ReachabilityStubbornSet(const PetriNet &net, const vector<PQL::Condition_ptr> &queries)
                : StubbornSet(net, queries) {}

        ReachabilityStubbornSet(const PetriNet &net)
                : StubbornSet(net) {}

        void prepare(const Structures::State *state) override;

    };
}

#endif //VERIFYPN_REACHABILITYSTUBBORNSET_H
