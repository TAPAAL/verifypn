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

#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"

namespace PetriEngine {

    void InterestingTransitionVisitor::_accept(const PQL::SimpleQuantifierCondition *element)
    {
        Visitor::visit(this, (*element)[0]);
    }

    void InterestingTransitionVisitor::_accept(const PQL::UntilCondition *element)
    {
        Visitor::visit(this, element->getCond1());
        negate();
        Visitor::visit(this, element->getCond1());
        negate();
        Visitor::visit(this, element->getCond2());
    }


    void InterestingTransitionVisitor::_accept(const PQL::AndCondition *element)
    {
        if (!negated) {               // and
            for (auto &c : *element) {
                if (!c->isSatisfied()) {
                    Visitor::visit(this, c);
                    break;
                }
            }
        } else {                    // or
            for (auto &c : *element) Visitor::visit(this, c);
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::OrCondition *element)
    {
        if (!negated) {               // or
            for (auto &c : *element) Visitor::visit(this, c);
        } else {                    // and
            for (auto &c : *element) {
                if (c->isSatisfied()) {
                    Visitor::visit(this, c);
                    break;
                }
            }
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::CompareConjunction *element)
    {

        auto neg = negated != element->isNegated();
        int32_t cand = std::numeric_limits<int32_t>::max();
        bool pre = false;
        for (auto &c : *element) {
            auto val = _stubborn.getParent()[c._place];
            if (c._lower == c._upper) {
                if (neg) {
                    if (val != c._lower) continue;
                    _stubborn.postsetOf(c._place, closure);
                    _stubborn.presetOf(c._place, closure);
                } else {
                    if (val == c._lower) continue;
                    if (val > c._lower) {
                        cand = c._place;
                        pre = false;
                    } else {
                        cand = c._place;
                        pre = true;
                    }
                }
            } else {
                if (!neg) {
                    if (val < c._lower && c._lower != 0) {
                        assert(!neg);
                        cand = c._place;
                        pre = true;
                    }

                    if (val > c._upper && c._upper != std::numeric_limits<uint32_t>::max()) {
                        assert(!neg);
                        cand = c._place;
                        pre = false;
                    }
                } else {
                    if (val >= c._lower && c._lower != 0) {
                        _stubborn.postsetOf(c._place, closure);
                    }

                    if (val <= c._upper && c._upper != std::numeric_limits<uint32_t>::max()) {
                        _stubborn.presetOf(c._place, closure);
                    }
                }
            }
            if (cand != std::numeric_limits<int32_t>::max()) {
                if (pre && _stubborn.seenPre(cand))
                    return;
                else if (!pre && _stubborn.seenPost(cand))
                    return;
            }
        }
        if (cand != std::numeric_limits<int32_t>::max()) {
            if (pre) {
                _stubborn.presetOf(cand, closure);
            } else if (!pre) {
                _stubborn.postsetOf(cand, closure);
            }
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::EqualCondition *element)
    {
        if (!negated) {               // equal
            if (element->getExpr1()->getEval() == element->getExpr2()->getEval()) { return; }
            if (element->getExpr1()->getEval() > element->getExpr2()->getEval()) {
                Visitor::visit(decr, element->getExpr1());
                Visitor::visit(incr, element->getExpr2());
            } else {
                Visitor::visit(incr, element->getExpr1());
                Visitor::visit(decr, element->getExpr2());
            }
        } else {                    // not equal
            if (element->getExpr1()->getEval() != element->getExpr2()->getEval()) { return; }
            Visitor::visit(incr, element->getExpr1());
            Visitor::visit(decr, element->getExpr1());
            Visitor::visit(incr, element->getExpr2());
            Visitor::visit(decr, element->getExpr2());
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::NotEqualCondition *element)
    {
        if (!negated) {               // not equal
            if (element->getExpr1()->getEval() != element->getExpr2()->getEval()) { return; }
            Visitor::visit(incr, element->getExpr1());
            Visitor::visit(decr, element->getExpr1());
            Visitor::visit(incr, element->getExpr2());
            Visitor::visit(decr, element->getExpr2());
        } else {                    // equal
            if (element->getExpr1()->getEval() == element->getExpr2()->getEval()) { return; }
            if (element->getExpr1()->getEval() > element->getExpr2()->getEval()) {
                Visitor::visit(decr, element->getExpr1());
                Visitor::visit(incr, element->getExpr2());
            } else {
                Visitor::visit(incr, element->getExpr1());
                Visitor::visit(decr, element->getExpr2());
            }
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::LessThanCondition *element)
    {
        if (!negated) {               // less than
            if (element->getExpr1()->getEval() < element->getExpr2()->getEval()) { return; }
            Visitor::visit(decr, element->getExpr1());
            Visitor::visit(incr, element->getExpr2());
        } else {                    // greater than or equal
            if (element->getExpr1()->getEval() >= element->getExpr2()->getEval()) { return; }
            Visitor::visit(incr, element->getExpr1());
            Visitor::visit(decr, element->getExpr2());
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::LessThanOrEqualCondition *element)
    {
        if (!negated) {               // less than or equal
            if (element->getExpr1()->getEval() <= element->getExpr2()->getEval()) { return; }
            Visitor::visit(decr, element->getExpr1());
            Visitor::visit(incr, element->getExpr2());
        } else {                    // greater than
            if (element->getExpr1()->getEval() > element->getExpr2()->getEval()) { return; }
            Visitor::visit(incr, element->getExpr1());
            Visitor::visit(decr, element->getExpr2());
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::NotCondition *element)
    {
        negate();
        Visitor::visit(this, (*element)[0]);
        negate();
    }

    void InterestingTransitionVisitor::_accept(const PQL::BooleanCondition *element)
    {
        // Add nothing
    }

    void InterestingTransitionVisitor::_accept(const PQL::DeadlockCondition *element)
    {
        if (!element->isSatisfied()) {
            _stubborn.postPresetOf(_stubborn.leastDependentEnabled(), closure);
        } // else add nothing
    }

    void
    InterestingTransitionVisitor::_accept(const PQL::UnfoldedUpperBoundsCondition *element)
    {
        for (auto &p : element->places())
            if (!p._maxed_out)
                _stubborn.presetOf(p._place);
    }

    void InterestingTransitionVisitor::_accept(const PQL::GCondition *element)
    {
        negate();
        Visitor::visit(this, (*element)[0]);
        negate();
    }

    void InterestingTransitionVisitor::_accept(const PQL::FCondition *element)
    {
        Visitor::visit(this, (*element)[0]);
    }

    void InterestingTransitionVisitor::_accept(const PQL::EFCondition *condition) {
        _accept(static_cast<const PQL::SimpleQuantifierCondition*>(condition));
    }

    void InterestingTransitionVisitor::_accept(const PQL::EGCondition *condition) {
        _accept(static_cast<const PQL::SimpleQuantifierCondition*>(condition));
    }

    void InterestingTransitionVisitor::_accept(const PQL::AGCondition *condition) {
        _accept(static_cast<const PQL::SimpleQuantifierCondition*>(condition));
    }

    void InterestingTransitionVisitor::_accept(const PQL::AFCondition *condition) {
        _accept(static_cast<const PQL::SimpleQuantifierCondition*>(condition));
    }

    void InterestingTransitionVisitor::_accept(const PQL::EXCondition *condition) {
        _accept(static_cast<const PQL::SimpleQuantifierCondition*>(condition));
    }

    void InterestingTransitionVisitor::_accept(const PQL::AXCondition *condition) {
        _accept(static_cast<const PQL::SimpleQuantifierCondition*>(condition));
    }

    void InterestingTransitionVisitor::_accept(const PQL::ACondition *condition) {
        _accept(static_cast<const PQL::SimpleQuantifierCondition*>(condition));
    }

    void InterestingTransitionVisitor::_accept(const PQL::ECondition *condition) {
        _accept(static_cast<const PQL::SimpleQuantifierCondition*>(condition));
    }

    void InterestingTransitionVisitor::_accept(const PQL::XCondition *condition) {
        _accept(static_cast<const PQL::SimpleQuantifierCondition*>(condition));
    }

    void InterestingTransitionVisitor::_accept(const PQL::AUCondition *condition) {
        _accept(static_cast<const PQL::UntilCondition*>(condition));
    }

    void InterestingTransitionVisitor::_accept(const PQL::EUCondition *condition) {
        _accept(static_cast<const PQL::UntilCondition*>(condition));
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::PlusExpr *element)
    {
        for (auto &i : element->places()) _stubborn.presetOf(i.first, closure);
        for (auto &e : element->expressions()) Visitor::visit(this, e);
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::PlusExpr *element)
    {
        for (auto &i : element->places()) _stubborn.postsetOf(i.first, closure);
        for (auto &e : element->expressions()) Visitor::visit(this, e);
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::SubtractExpr *element)
    {
        bool first = true;
        for (auto &e : element->expressions()) {
            if (first)
                Visitor::visit(this, e);
            else
                Visitor::visit(decr, e);
            first = false;
        }
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::SubtractExpr *element)
    {
        bool first = true;
        for (auto &e : element->expressions()) {
            if (first)
                Visitor::visit(this, e);
            else
                Visitor::visit(incr,e);
            first = false;
        }
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::MultiplyExpr *element)
    {
        if ((element->places().size() + element->expressions().size()) == 1) {
            for (auto &i : element->places()) _stubborn.presetOf(i.first, closure);
            for (auto &e : element->expressions()) Visitor::visit(this, e);
        } else {
            for (auto &i : element->places()) {
                _stubborn.presetOf(i.first, closure);
                _stubborn.postsetOf(i.first, closure);
            }
            for (auto &e : element->expressions()) {
                Visitor::visit(this, e);
                Visitor::visit(decr, e);
            }
        }
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::MultiplyExpr *element)
    {
        if ((element->places().size() + element->expressions().size()) == 1) {
            for (auto &i : element->places()) _stubborn.postsetOf(i.first, closure);
            for (auto &e : element->expressions()) Visitor::visit(this, e);
        } else
            Visitor::visit(incr, element);
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::MinusExpr *element)
    {
        // TODO not implemented
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::MinusExpr *element)
    {
        // TODO not implemented
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::LiteralExpr *element)
    {
        // Add nothing
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::LiteralExpr *element)
    {
        // Add nothing
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::UnfoldedIdentifierExpr *element)
    {
        _stubborn.presetOf(element->offset(), closure);
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::UnfoldedIdentifierExpr *element)
    {
        _stubborn.postsetOf(element->offset(), closure);
    }

    void InterestingLTLTransitionVisitor::_accept(const PQL::LessThanCondition *element)
    {
        negate_if_satisfied<PQL::LessThanCondition>(element);
    }

    void InterestingLTLTransitionVisitor::_accept(const PQL::LessThanOrEqualCondition *element)
    {
        negate_if_satisfied<PQL::LessThanOrEqualCondition>(element);
    }

    void InterestingLTLTransitionVisitor::_accept(const PQL::EqualCondition *element)
    {
        negate_if_satisfied<PQL::EqualCondition>(element);
    }

    void InterestingLTLTransitionVisitor::_accept(const PQL::NotEqualCondition *element)
    {
        negate_if_satisfied<PQL::NotEqualCondition>(element);
    }

    void InterestingLTLTransitionVisitor::_accept(const PQL::CompareConjunction *element)
    {
        auto neg = negated != element->isNegated();
        for (auto &c : *element) {
            if (!neg) {
                if (c._lower != 0 && !_stubborn.seenPre(c._place)) {
                    // c < p becomes satisfied by preset of p.
                    _stubborn.presetOf(c._place, closure);
                }
                if (c._upper != std::numeric_limits<uint32_t>::max() && !_stubborn.seenPost(c._place)) {
                    // p < c becomes satisfied by postset of p.
                    _stubborn.postsetOf(c._place, closure);
                }
            }
            else {
                if (c._lower != 0 && !_stubborn.seenPost(c._place)) {
                    // !(p < c) becomes satisfied by preset of p.
                    _stubborn.postsetOf(c._place, closure);
                }
                if (c._upper != std::numeric_limits<uint32_t>::max() && !_stubborn.seenPre(c._place)) {
                    // !(c < p) becomes satisfied by postset of p.
                    _stubborn.presetOf(c._place, closure);
                }
            }
        }
    }

    template<typename Condition>
    void InterestingLTLTransitionVisitor::negate_if_satisfied(const Condition *element)
    {
        auto isSatisfied = element->getSatisfied();
        assert(isSatisfied != PQL::Condition::RUNKNOWN);
        if ((isSatisfied == PQL::Condition::RTRUE) != negated) {
            negate();
            InterestingTransitionVisitor::_accept(element);
            negate();
        } else
            InterestingTransitionVisitor::_accept(element);
    }

    void AutomatonInterestingTransitionVisitor::_accept(const PQL::CompareConjunction *element)
    {
        auto neg = negated != element->isNegated();
        for (auto &c : *element) {
            int32_t cand = std::numeric_limits<int32_t>::max();
            bool pre = false;
            auto val = _stubborn.getParent()[c._place];
            if (c._lower == c._upper) {
                if (neg) {
                    if (val != c._lower) continue;
                    _stubborn.postsetOf(c._place, closure);
                    _stubborn.presetOf(c._place, closure);
                } else {
                    if (val == c._lower) continue;
                    if (val > c._lower) {
                        cand = c._place;
                        pre = false;
                    } else {
                        cand = c._place;
                        pre = true;
                    }
                }
            } else {
                if (!neg) {
                    if (val < c._lower && c._lower != 0) {
                        assert(!neg);
                        cand = c._place;
                        pre = true;
                    }

                    if (val > c._upper && c._upper != std::numeric_limits<uint32_t>::max()) {
                        assert(!neg);
                        cand = c._place;
                        pre = false;
                    }
                } else {
                    if (val >= c._lower && c._lower != 0) {
                        _stubborn.postsetOf(c._place, closure);
                    }

                    if (val <= c._upper && c._upper != std::numeric_limits<uint32_t>::max()) {
                        _stubborn.presetOf(c._place, closure);
                    }
                }
            }
            if (cand != std::numeric_limits<int32_t>::max()) {
                if (pre) {
                    _stubborn.presetOf(cand, closure);
                } else if (!pre) {
                    _stubborn.postsetOf(cand, closure);
                }
                cand = std::numeric_limits<int32_t>::max();
            }
        }

    }
}