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
#include "LTL/Stubborn/LTLEvalAndSetVisitor.h"
#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;

namespace LTL {
    class NondeterministicConjunctionVisitor : public InterestingLTLTransitionVisitor {
    public:
        NondeterministicConjunctionVisitor(AutomatonStubbornSet &stubbornSet) :
                InterestingLTLTransitionVisitor(stubbornSet, false),
                _net(stubbornSet._net), _stubborn(stubbornSet){}

    protected:
        static constexpr auto PresetBad = 8;
        static constexpr auto PostsetBad = 16;

        void _accept(const PQL::CompareConjunction *element) override
        {
            if (_stubborn._track_changes || element->isNegated() != negated) {
                InterestingLTLTransitionVisitor::_accept(element);
                return;
            }
            _stubborn._track_changes = true;
            memcpy(_stubborn._place_checkpoint.get(), _stubborn._places_seen.get(), _net.numberOfPlaces());
            _accept_conjunction(element);
            _stubborn._track_changes = false;
            assert(_stubborn._pending_stubborn.empty());

        }

        void _accept_conjunction(const PQL::CompareConjunction *element)
        {
            assert(_stubborn._track_changes);
            assert(_stubborn._pending_stubborn.empty());
            assert(element->isNegated() == negated);
            std::vector<std::pair<uint32_t, bool>> cands; // Transition id, Preset
            for (auto &cons : *element) {
                uint32_t tokens = _stubborn.getParent()[cons._place];

                if (cons._lower == cons._upper) {
                    //Compare is equality
                    if (cons._lower == tokens) {
                        continue;
                    } else if (tokens < cons._lower) {
                        // Valid candidate, explore preset
                        cands.emplace_back(cons._place, true);
                    } else {
                        // Valid candidate, explore postset
                        cands.emplace_back(cons._place, false);
                    }
                } else {
                    if (tokens < cons._lower && cons._lower != 0) {
                        // explore preset
                        cands.emplace_back(cons._place, true);
                    }

                    if (tokens > cons._upper && cons._upper != std::numeric_limits<uint32_t>::max()) {
                        // explore postset
                        cands.emplace_back(cons._place, false);
                    }
                }

                // explore cand if available
                // if cand seen previously, return do not add additional (good case)
                // else add cand to candidate list
                auto &[cand, pre] = cands.back();
                if (!cands.empty() && cand == cons._place) {
                    assert(_stubborn._pending_stubborn.empty());
                    if ((pre && _stubborn._places_seen[cand] & PresetBad) ||
                        (!pre && _stubborn._places_seen[cand] & PostsetBad)) {
                        cands.pop_back();
                        continue;
                    }
                    // this constraint was a candidate previously, thus we are happy
                    if ((pre && _stubborn.seenPre(cand)) || (!pre && _stubborn.seenPost(cand)))
                        return;
                }
            }

            // Run through candidate list and find first candidate not violating s-inv and add it to stub, then return.
            for (size_t i = cands.size(); i > 0; --i) {
                auto &[cand, pre] = cands[i - 1];
                if (pre) {
                    //explore pre
                    assert(!(_stubborn._places_seen[cand] & PresetBad));
                    _stubborn.presetOf(cand, false);
                } else {
                    //explore post
                    assert(!(_stubborn._places_seen[cand] & PostsetBad));
                    _stubborn.postsetOf(cand, false);
                }

                if (_stubborn._bad) {
                    _stubborn._places_seen[cand] |= pre ? PresetBad : PostsetBad;
#ifndef NDEBUG
                    //std::cerr << "Bad pre/post and reset" << std::endl;
#endif
                    _stubborn._reset_pending();
                    continue;
                }

                if (_stubborn._closure()) {
                    // success, return this stubborn set
                    _stubborn._apply_pending();
                    return;
                } else {
#ifndef NDEBUG
                    //std::cerr << "Bad closure and reset" << std::endl;
#endif
                    _stubborn._places_seen[cand] |= pre ? PresetBad : PostsetBad;
                    _stubborn._reset_pending();
                }
            }

            // If no candidate not violating s-inv set St=T.
            _stubborn.set_all_stubborn();
        }

        const PetriEngine::PetriNet &_net;
        AutomatonStubbornSet &_stubborn;

    };

    bool AutomatonStubbornSet::prepare(const LTL::Structures::ProductState *state)
    {
        reset();
        _parent = state;
        _gen.prepare(state);
        constructEnabled();
        if (_ordering.empty())
            return false;
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
#ifndef NDEBUG
            std::cerr << "Lone successor " << _net.transitionNames()[_ordering.front()] << std::endl;
#endif
            return true;
        }


        guard_info_t buchi_state = _state_guards[state->get_buchi_state()];

        PQL::EvaluationContext evaluationContext{_parent->marking(), &_net};

        // Check if retarding is satisfied for condition 3.
        _retarding_satisfied = _aut.guard_valid(evaluationContext, buchi_state._retarding._bdd);
        /*
        if (!_aut.guard_valid(evaluationContext, buchi_state.retarding.decision_diagram)) {
            set_all_stubborn();
            __print_debug();
            return true;
        }*/

        // Calculate retarding subborn set to ensure S-INV.
        auto negated_retarding = std::make_unique<NotCondition>(buchi_state._retarding._condition);
        _retarding_stubborn_set.setQuery(negated_retarding.get());
        _retarding_stubborn_set.prepare(state);


        _nenabled = _ordering.size();

        // If a progressing formula satisfies the guard St=T is the only way to ensure NLG.
        for (auto &q : buchi_state._progressing) {
            if (_aut.guard_valid(evaluationContext, q._bdd)) {
                set_all_stubborn();
                __print_debug();
                return true;
            }
        }

        //Interesting on each progressing formula gives NLG.
        for (auto &q : buchi_state._progressing) {
            LTLEvalAndSetVisitor evalAndSetVisitor{evaluationContext};
            PetriEngine::PQL::Visitor::visit(evalAndSetVisitor, q._condition);

            NondeterministicConjunctionVisitor interesting{*this};
            PetriEngine::PQL::Visitor::visit(interesting, q._condition);
            if (_done) return true;
            else {
                assert(!_track_changes);
                assert(_pending_stubborn.empty());
                // Closure to ensure COM.
                _closure();
                if (_bad) {
                    set_all_stubborn();
                    return true;
                }
            }
        }

        assert(_unprocessed.empty());
        assert(_pending_stubborn.empty());
        assert(!_bad);
        assert(!_done);



        /*//Check that S-INV is satisfied
        if (has_shared_mark(_stubborn.get(), _retarding_stubborn_set.stubborn(), _net.numberOfTransitions())) {
            memset(_stubborn.get(), true, sizeof(bool) * _net.numberOfTransitions());
            //return true;
        }*/

        // Ensure we have a key transition in accepting buchi states.
        if (!_has_enabled_stubborn && buchi_state._is_accepting) {
            for (uint32_t i = 0; i < _net.numberOfTransitions(); ++i) {
                if (!_stubborn[i] && _enabled[i]) {
                    addToStub(i);
                    _closure();
                    if (_bad) {
                        set_all_stubborn();
                        return true;
                    }
                    break;
                }
            }
        }

        assert(!has_shared_mark(_stubborn.get(), _retarding_stubborn_set.stubborn(), _net.numberOfTransitions()));

        __print_debug();

        return true;
    }


    uint32_t AutomatonStubbornSet::next()
    {
        while (!_ordering.empty()) {
            _current = _ordering.front();
            _ordering.pop_front();
            if (_stubborn[_current] && _enabled[_current]) {
                return _current;
            }
        }
        reset();
        return std::numeric_limits<uint32_t>::max();
    }

    void AutomatonStubbornSet::__print_debug()
    {
#ifndef NDEBUG
        return;
        std::cout << "Enabled: ";
        for (int i = 0; i < _net.numberOfTransitions(); ++i) {
            if (_enabled[i]) {
                std::cout << _net.transitionNames()[i] << ' ';
            }
        }
        std::cout << "\nStubborn: ";
        for (int i = 0; i < _net.numberOfTransitions(); ++i) {
            if (_stubborn[i]) {
                std::cout << _net.transitionNames()[i] << ' ';
            }
        }
        std::cout << std::endl;
#endif
    }

    void AutomatonStubbornSet::reset()
    {
        StubbornSet::reset();
        memset(_place_checkpoint.get(), 0, _net.numberOfPlaces());
        _retarding_stubborn_set.reset();
        _has_enabled_stubborn = false;
        _bad = false;
        _done = false;
        _track_changes = false;
        _pending_stubborn.clear();
        _unprocessed.clear();
    }

    void AutomatonStubbornSet::addToStub(uint32_t t)
    {
        if (_retarding_stubborn_set.stubborn()[t] || !_cond3_valid(t)) {
            _bad = true;
            return;
        }
        if (_track_changes) {
            if (_enabled[t])
                _has_enabled_stubborn = true;
            if (_pending_stubborn.insert(t).second) {
                _unprocessed.push_back(t);
            }
        } else {
            StubbornSet::addToStub(t);
        }
    }

    bool AutomatonStubbornSet::_cond3_valid(uint32_t t)
    {
        EvaluationContext ctx{_markbuf.marking(), &_net};
        if (_retarding_satisfied || !_enabled[t]) return true;
        else {
            assert(_gen.checkPreset(t));
            assert(dynamic_cast<const LTL::Structures::ProductState *>(_parent) != nullptr);
            memcpy(_markbuf.marking(), (*_parent).marking(), _net.numberOfPlaces() * sizeof(MarkVal));
            _gen.consumePreset(_markbuf, t);
            _gen.producePostset(_markbuf, t);
            return _aut.guard_valid(ctx,
                                    _state_guards[static_cast<const LTL::Structures::ProductState *>(_parent)->get_buchi_state()]._retarding._bdd);
        }
    }

    void AutomatonStubbornSet::_reset_pending()
    {
        _bad = false;
        _pending_stubborn.clear();
        _unprocessed.clear();
        memcpy(_places_seen.get(), _place_checkpoint.get(), _net.numberOfPlaces());
    }

    void AutomatonStubbornSet::_apply_pending()
    {
        assert(!_bad);
        for (auto t : _pending_stubborn) {
            _stubborn[t] = true;
        }
        _pending_stubborn.clear();
        assert(_unprocessed.empty());
    }

    void AutomatonStubbornSet::set_all_stubborn()
    {
        memset(_stubborn.get(), true, sizeof(bool) * _net.numberOfTransitions());
        _done = true;
    }

    bool AutomatonStubbornSet::_closure()
    {
        StubbornSet::closure([&]() { return !_bad; });
        return !_bad;
    }
}
