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

#ifndef VERIFYPN_AUTOMATONSTUBBORNSET_H
#define VERIFYPN_AUTOMATONSTUBBORNSET_H

#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/Stubborn/NegatedStubbornSet.h"
#include "LTL/Structures/GuardInfo.h"

namespace LTL {
    class AutomatonStubbornSet : public PetriEngine::ReachabilityStubbornSet {
    public:
        explicit AutomatonStubbornSet(const PetriEngine::PetriNet &net) : ReachabilityStubbornSet(net), negated(net) {}
        AutomatonStubbornSet(const PetriEngine::PetriNet &net, const PetriEngine::PQL::Condition_ptr&) : ReachabilityStubbornSet(net), negated(net) {}

        void prepare(const PetriEngine::Structures::State *state,
                     const GuardInfo &info);

        void reset() override;

        void addToStub(uint32_t t) override {
            PetriEngine::StubbornSet::addToStub(t);
        }

    private:
        void prepare_accepting(const PetriEngine::Structures::State *state, const GuardInfo &info);
        void prepare_nonaccepting(const PetriEngine::Structures::State *state, const GuardInfo &info);
        void copyNegated();

        PetriEngine::NegatedStubbornSet negated;
        bool hasStubborn = false;

        friend class VisibilityVisitor;
    };

}

#endif //VERIFYPN_AUTOMATONSTUBBORNSET_H
