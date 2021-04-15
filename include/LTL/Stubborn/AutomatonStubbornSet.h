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

#include "LTL/Structures/BuchiAutomaton.h"
#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/Stubborn/NegatedStubbornSet.h"
#include "LTL/Structures/GuardInfo.h"
#include "LTL/SuccessorGeneration/SuccessorSpooler.h"

namespace LTL {
    class AutomatonStubbornSet : public PetriEngine::StubbornSet, public SuccessorSpooler {
    public:
        explicit AutomatonStubbornSet(const PetriEngine::PetriNet &net, const Structures::BuchiAutomaton &aut) : PetriEngine::StubbornSet(net),
            _retarding_stubborn_set(net),
            _state_guards(std::move(GuardInfo::from_automaton(aut))),
            _aut(aut)
        {}

        bool prepare(const PetriEngine::Structures::State *marking) override {
            return prepare(dynamic_cast<const LTL::Structures::ProductState*>(marking));
        }

        bool prepare(const LTL::Structures::ProductState *state) override;

        uint32_t next() override;

        void reset() override;


    private:
        bool has_shared_mark(const bool* a, const bool* b, size_t size) {
            for (size_t i = 0; i < size; ++i) {
                if (a[i] && b[i]) return true;
            }
            return false;
        }

    protected:
        void addToStub(uint32_t t) override;

    private:

        PetriEngine::ReachabilityStubbornSet _retarding_stubborn_set;
        const std::vector<GuardInfo> _state_guards;
        const Structures::BuchiAutomaton &_aut;
        bool _has_enabled_stubborn = false;
    };

}

#endif //VERIFYPN_AUTOMATONSTUBBORNSET_H
