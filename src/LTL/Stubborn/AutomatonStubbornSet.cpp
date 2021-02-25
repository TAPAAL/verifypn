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

#include "LTL/Structures/GuardInfo.h"
#include "LTL/Stubborn/AutomatonStubbornSet.h"
#include "LTL/Stubborn/EvalAndSetVisitor.h"
#include "LTL/Stubborn/VisibilityVisitor.h"
#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;

namespace LTL {
    void
    AutomatonStubbornSet::prepare(const PetriEngine::Structures::State *state, const GuardInfo &info)
    {
        reset();
        _parent = state;
        memset(_places_seen.get(), 0, _net.numberOfPlaces());
        constructEnabled();
        if (_ordering.size() == 0) return;
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
            return;
        }
        PQL::EvaluationContext evalCtx(_parent->marking(), &_net);
        EvalAndSetVisitor evalVisitor{evalCtx};
        info.retarding->visit(evalVisitor);
        for (auto &cond : info.progressing) {
            cond->visit(evalVisitor);
        }
        if (!info.is_accepting) {
            prepare_nonaccepting(state, info);
        } else {
            prepare_accepting(state, info);
        }
        if (!_ordering.empty()) {
            for (auto t = 0; t < _net.numberOfTransitions(); ++t) {
                if (_stubborn[t] && _enabled[t]) {
                    hasStubborn = true;
                    break;
                }
            }
            if (!hasStubborn) {
                // if there are enabled transitions, ensure we have some stubborn transition (all)
                std::cerr << "expanding all!" << std::endl;
                memset(_stubborn.get(), true, sizeof(bool) * _net.numberOfTransitions());
            }
        }
        float num_stubborn = 0;
        float num_enabled = 0;
        float num_enabled_stubborn = 0;
        for (int i = 0; i < _net.numberOfTransitions(); ++i) {
            if (_stubborn[i]) ++num_stubborn;
            if (_enabled[i]) ++num_enabled;
            if (_stubborn[i] && _enabled[i]) ++num_enabled_stubborn;
        }
        std::cerr << "Enabled: " << num_enabled << "/" << _net.numberOfTransitions() << " (" << num_enabled/_net.numberOfTransitions()*100.0 << "%),\t "
                  << "Stubborn: " << num_stubborn << "/" << _net.numberOfTransitions() << " (" << num_stubborn/_net.numberOfTransitions()*100.0 << "%),\t "
                  << "Enabled stubborn: " << num_enabled_stubborn << "/" << num_enabled << " (" << num_enabled_stubborn/num_enabled*100.0 << "%)" << std::endl;
    }

    void AutomatonStubbornSet::prepare_accepting(const PetriEngine::Structures::State *state, const GuardInfo &info)
    {
        bool sat = info.retarding->isSatisfied();
        InterestingTransitionVisitor interesting{*this, false};
        std::vector<Condition_ptr> satQueries;
        for (const auto &cond : info.progressing) {
            if (cond->isSatisfied()) {
                satQueries.push_back(cond);
            } else {
                cond->visit(interesting);
            }
        }
        if (!sat) {
            if (!satQueries.empty()) {
                //closure();
            }
            else {
                negated.prepare(state, satQueries, false);
                //closure();
                negated.copyStubborn(_stubborn);
            }
        }
        else {
            if (!satQueries.empty()) {
                negated.prepare(state, satQueries, true);
                negated.extend(info.retarding, false);
                closure();
                negated.copyStubborn(_stubborn);
            }
            else {
                negated.prepare(state, {info.retarding}, false);
                closure();
                negated.copyStubborn(_stubborn);
            }
        }
    }

    void AutomatonStubbornSet::prepare_nonaccepting(const PetriEngine::Structures::State *state, const GuardInfo &info)
    {
        // Treat each outgoing guard as reachability query.
        // If no out guard satisfied, skip closure computation.
        InterestingTransitionVisitor interesting{*this, false};
        std::vector<Condition_ptr> satQueries;
        for (const auto &cond : info.progressing) {
            if (cond->isSatisfied()) {
                satQueries.push_back(cond);
            } else {
                cond->visit(interesting);
            }
        }
        if (info.retarding->isSatisfied()) {
            
            VisibilityVisitor visible{*this};
            info.retarding->visit(visible);
            if (visible.foundVisible()) {
                std::cerr << "Found visible!\n";
                memset(_stubborn.get(), true, _net.numberOfTransitions());
                return;
            }
            else {
                closure();
                return;
            }
            //satQueries.push_back(info.retarding);
        }
        if (!satQueries.empty()) {
            negated.prepare(state, satQueries, false);
            // exists satisfying queries, thus add all interesting transitions
            // closure();
            negated.copyStubborn(_stubborn);
        } else {
            info.retarding->visit(interesting);
        }
    }

    void AutomatonStubbornSet::reset()
    {
        hasStubborn = false;
        nstubborn = 0;
        StubbornSet::reset();
        negated.reset();
    }
}
