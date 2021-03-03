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

#ifndef VERIFYPN_NEGATEDSTUBBORNSET_H
#define VERIFYPN_NEGATEDSTUBBORNSET_H

#include "PetriEngine/Stubborn/StubbornSet.h"
#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"

namespace PetriEngine {
    /**
     * Exactly Stubborn Set, but bool vector _stubborn is flipped s.t.
     * 1 means not stubborn and 0 means stubborn. Intended for use in
     * automaton-based LTL-preserving stubborn set method.
     */
    class NegatedStubbornSet : public ReachabilityStubbornSet {
    public:
        NegatedStubbornSet(const PetriNet &net)
                : ReachabilityStubbornSet(net) {}

        void reset() override;

        void prepare(const Structures::State *state, const std::vector<PQL::Condition_ptr> &_queries,
                     bool do_closure);

        void copyStubborn(std::unique_ptr<bool[]> &stub) {
            for (size_t i = 0; i < _net.numberOfTransitions(); ++i) {
                stub[i] |= _stubborn[i];
            }
        }

        [[nodiscard]] inline bool isStubborn(size_t t) const { return _stubborn[t]; }

        void extend(const PQL::Condition_ptr &query, bool do_closure);
            protected:
        void addToStub(uint32_t) override;
    };

}
#endif //VERIFYPN_NEGATEDSTUBBORNSET_H
