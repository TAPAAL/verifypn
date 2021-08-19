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

#include "LTL/Stubborn/SafeAutStubbornSet.h"
#include "LTL/Stubborn/VisibleTransitionVisitor.h"

namespace LTL {
    using namespace PetriEngine;

    bool SafeAutStubbornSet::prepare(const LTL::Structures::ProductState *state) {
        reset();
        _parent = state;
        memset(_places_seen.get(), 0, _net.numberOfPlaces());

        constructEnabled();
        if (_ordering.empty()) {
            return false;
        }
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
            return true;
        }

        assert(_prog_cond != nullptr || state->is_accepting());
        _calc_unsafe();


        assert(!_bad);

        _unsafe.swap(_stubborn);
        _has_enabled_stubborn = false;
        _unprocessed.clear();
        memset(_places_seen.get(), 0, _net.numberOfPlaces());

        assert(_unprocessed.empty());

        _calc_stubborn(state);

        return true;
    }

    void SafeAutStubbornSet::_calc_unsafe() {
        PQL::EvaluationContext ctx((*_parent).marking(), &_net);
        if (_prog_cond != nullptr) {
            _prog_cond->evalAndSet(ctx);
            _prog_cond->visit(_interesting);
        }
        _sink_cond->evalAndSet(ctx);
        //_ret_cond->evalAndSet(ctx);
        //(std::make_shared<PetriEngine::PQL::NotCondition>(_ret_cond))->visit(interesting);
        _sink_cond->visit(_interesting);
    }

    void SafeAutStubbornSet::_calc_stubborn(const LTL::Structures::ProductState *state) {
        // in most cases, sink condition is not interesting, just unsafe.
        if (_prog_cond) { _prog_cond->visit(_interesting); }
        else {
            _sink_cond->visit(_interesting);
        }
        closure();
        if (_bad) {
            // abort
            set_all_stubborn();
            return;
        }
        // accepting states need key transition. add first enabled by index.
        if (state->is_accepting() && !_has_enabled_stubborn) {
            addToStub(_ordering.front());
            closure();
            /*            for (int i = 0; i < _net.numberOfPlaces(); ++i) {
                            if (_enabled[i]) {
                                addToStub(i);
                                closure();
                                break;
                            }
                        }*/
            if (_bad) {
                set_all_stubborn();
                return;
            }
        }
    }


}