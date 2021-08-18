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

#ifndef VERIFYPN_SAFEAUTSTUBBORNSET_H
#define VERIFYPN_SAFEAUTSTUBBORNSET_H

#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"
#include "LTL/SuccessorGeneration/SuccessorSpooler.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "PetriEngine/PQL/PQL.h"

namespace LTL {
    class SafeAutStubbornSet : public PetriEngine::StubbornSet, public SuccessorSpooler {
    public:
        SafeAutStubbornSet(const PetriEngine::PetriNet &net,
                           const std::vector<PetriEngine::PQL::Condition_ptr> &queries)
                : StubbornSet(net, queries), _unsafe(std::make_unique<bool[]>(net.numberOfTransitions())) {}

        bool prepare(const PetriEngine::Structures::State *marking) override
        {
            assert(false);
            std::cerr << "Error: SafeAutStubbornSet is implemented only for product states\n";
            exit(1);
            return false;
        }

        bool prepare(const LTL::Structures::ProductState *state) override;

        uint32_t next() override {
            return StubbornSet::next();
        }

        void reset() override {
            StubbornSet::reset();
            memset(_unsafe.get(), false, sizeof(bool) * _net.numberOfTransitions());
            _bad = false;
            _has_enabled_stubborn = false;
        }

        void set_buchi_conds(PetriEngine::PQL::Condition_ptr ret_cond,
                             PetriEngine::PQL::Condition_ptr prog_cond,
                             PetriEngine::PQL::Condition_ptr sink_cond) {
            _ret_cond = ret_cond;
            _prog_cond = prog_cond;
            _sink_cond = sink_cond;
        }

    protected:
        void addToStub(uint32_t t) override
        {
            // potential refinement of bad: can manually check whether firing t would violate some progressing formula.
            if (_enabled[t]) {
                _has_enabled_stubborn = true;
                if (_unsafe[t]) {
                    _bad = true;
                    return;
                }
            }
            StubbornSet::addToStub(t);
        }

    private:
        std::unique_ptr<bool[]> _unsafe;
        bool _bad = false;
        bool _has_enabled_stubborn = false;
        PetriEngine::PQL::Condition_ptr _ret_cond;
        PetriEngine::PQL::Condition_ptr _prog_cond;
        PetriEngine::PQL::Condition_ptr _sink_cond;
    };
}

#endif //VERIFYPN_SAFEAUTSTUBBORNSET_H
