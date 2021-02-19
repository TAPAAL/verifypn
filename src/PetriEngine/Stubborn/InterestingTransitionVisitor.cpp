/*
 * File:   InterestingTransitionVisitor.cpp.cc
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 03/02/2021
 */

#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"

namespace PetriEngine {

    void InterestingTransitionVisitor::_accept(const PQL::SimpleQuantifierCondition *element) {
        (*element)[0]->visit(*this);
    }

    void InterestingTransitionVisitor::_accept(const PQL::UntilCondition *element) {
        element->getCond1()->visit(*this);
        negate();
        element->getCond1()->visit(*this);
        negate();
        element->getCond2()->visit(*this);
    }


    void InterestingTransitionVisitor::_accept(const PQL::AndCondition *element) {
        if (!negated) {               // and
            for (auto &c : *element) {
                if (!c->isSatisfied()) {
                    c->visit(*this);
                    break;
                }
            }
        } else {                    // or
            for (auto &c : *element) c->visit(*this);
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::OrCondition *element) {
        if (!negated) {               // or
            for (auto &c : *element) c->visit(*this);
        } else {                    // and
            for (auto &c : *element) {
                if (c->isSatisfied()) {
                    c->visit(*this);
                    break;
                }
            }
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::CompareConjunction *element) {

        auto neg = negated != element->isNegated();
        int32_t cand = std::numeric_limits<int32_t>::max();
        bool pre = false;
        for (auto &c : *element) {
            auto val = _stubborn.parent()[c._place];
            if (c._lower == c._upper) {
                if (neg) {
                    if (val != c._lower) continue;
                    _stubborn.postsetOf(c._place, true);
                    _stubborn.presetOf(c._place, true);
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
                        _stubborn.postsetOf(c._place, true);
                    }

                    if (val <= c._upper && c._upper != std::numeric_limits<uint32_t>::max()) {
                        _stubborn.presetOf(c._place, true);
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
                _stubborn.presetOf(cand, true);
            } else if (!pre) {
                _stubborn.postsetOf(cand, true);
            }
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::EqualCondition *element) {
        if (!negated) {               // equal
            if (element->getExpr1()->getEval() == element->getExpr2()->getEval()) { return; }
            if (element->getExpr1()->getEval() > element->getExpr2()->getEval()) {
                element->getExpr1()->visit(decr);
                element->getExpr2()->visit(incr);
            } else {
                element->getExpr1()->visit(incr);
                element->getExpr2()->visit(decr);
            }
        } else {                    // not equal
            if (element->getExpr1()->getEval() != element->getExpr2()->getEval()) { return; }
            element->getExpr1()->visit(incr);
            element->getExpr1()->visit(decr);
            element->getExpr2()->visit(incr);
            element->getExpr2()->visit(decr);
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::NotEqualCondition *element) {
        if (!negated) {               // not equal
            if (element->getExpr1()->getEval() != element->getExpr2()->getEval()) { return; }
            element->getExpr1()->visit(incr);
            element->getExpr1()->visit(decr);
            element->getExpr2()->visit(incr);
            element->getExpr2()->visit(decr);
        } else {                    // equal
            if (element->getExpr1()->getEval() == element->getExpr2()->getEval()) { return; }
            if (element->getExpr1()->getEval() > element->getExpr2()->getEval()) {
                element->getExpr1()->visit(decr);
                element->getExpr2()->visit(incr);
            } else {
                element->getExpr1()->visit(incr);
                element->getExpr2()->visit(decr);
            }
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::LessThanCondition *element) {
        if (!negated) {               // less than
            if (element->getExpr1()->getEval() < element->getExpr2()->getEval()) { return; }
            element->getExpr1()->visit(decr);
            element->getExpr2()->visit(incr);
        } else {                    // greater than or equal
            if (element->getExpr1()->getEval() >= element->getExpr2()->getEval()) { return; }
            element->getExpr1()->visit(incr);
            element->getExpr2()->visit(decr);
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::LessThanOrEqualCondition *element) {
        if (!negated) {               // less than or equal
            if (element->getExpr1()->getEval() <= element->getExpr2()->getEval()) { return; }
            element->getExpr1()->visit(decr);
            element->getExpr2()->visit(incr);
        } else {                    // greater than
            if (element->getExpr1()->getEval() > element->getExpr2()->getEval()) { return; }
            element->getExpr1()->visit(incr);
            element->getExpr2()->visit(decr);
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::GreaterThanCondition *element) {
        if (!negated) {               // greater than
            if (element->getExpr1()->getEval() > element->getExpr2()->getEval()) { return; }
            element->getExpr1()->visit(incr);
            element->getExpr2()->visit(decr);
        } else {                    // less than or equal
            if (element->getExpr1()->getEval() <= element->getExpr2()->getEval()) { return; }
            element->getExpr1()->visit(decr);
            element->getExpr2()->visit(incr);
        }
    }

    void
    InterestingTransitionVisitor::_accept(const PQL::GreaterThanOrEqualCondition *element) {
        if (!negated) {               // greater than or equal
            if (element->getExpr1()->getEval() >= element->getExpr2()->getEval()) { return; }
            element->getExpr1()->visit(incr);
            element->getExpr2()->visit(decr);
        } else {                    // less than
            if (element->getExpr1()->getEval() < element->getExpr2()->getEval()) { return; }
            element->getExpr1()->visit(decr);
            element->getExpr2()->visit(incr);
        }
    }

    void InterestingTransitionVisitor::_accept(const PQL::NotCondition *element) {
        negate();
        (*element)[0]->visit(*this);
        negate();
    }

    void InterestingTransitionVisitor::_accept(const PQL::BooleanCondition *element) {
        // Add nothing
    }

    void InterestingTransitionVisitor::_accept(const PQL::DeadlockCondition *element) {
        if (!element->isSatisfied()) {
            _stubborn.postPresetOf(_stubborn.leastDependentEnabled(), true);
        } // else add nothing
    }

    void
    InterestingTransitionVisitor::_accept(const PQL::UnfoldedUpperBoundsCondition *element) {
        for (auto &p : element->places())
            if (!p._maxed_out)
                _stubborn.presetOf(p._place);
    }

    void InterestingTransitionVisitor::_accept(const PQL::GCondition *element)
    {
        negate();
        (*element)[0]->visit(*this);
        negate();
    }

    void InterestingTransitionVisitor::_accept(const PQL::FCondition *element)
    {
        (*element)[0]->visit(*this);
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::PlusExpr *element) {
        for(auto& i : element->places()) _stubborn.presetOf(i.first, true);
        for(auto& e : element->expressions()) e->visit(*this);
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::PlusExpr *element) {
        for(auto& i : element->places()) _stubborn.postsetOf(i.first, true);
        for(auto& e : element->expressions()) e->visit(*this);
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::SubtractExpr *element) {
        bool first = true;
        for(auto& e : element->expressions())
        {
            if(first)
                e->visit(*this);
            else
                e->visit(*decr);
            first = false;
        }
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::SubtractExpr *element) {
        bool first = true;
        for(auto& e : element->expressions())
        {
            if(first)
                e->visit(*this);
            else
                e->visit(*incr);
            first = false;
        }
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::MultiplyExpr *element) {
        if((element->places().size() + element->expressions().size()) == 1)
        {
            for(auto& i : element->places()) _stubborn.presetOf(i.first, true);
            for(auto& e : element->expressions()) e->visit(*this);
        }
        else
        {
            for(auto& i : element->places())
            {
                _stubborn.presetOf(i.first, true);
                _stubborn.postsetOf(i.first, true);
            }
            for(auto& e : element->expressions())
            {
                e->visit(*this);
                e->visit(*decr);
            }
        }
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::MultiplyExpr *element) {
        if((element->places().size() + element->expressions().size()) == 1)
        {
            for(auto& i : element->places()) _stubborn.postsetOf(i.first, true);
            for(auto& e : element->expressions()) e->visit(*this);
        }
        else
            element->visit(*incr);
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::MinusExpr *element) {
        // TODO not implemented
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::MinusExpr *element) {
        // TODO not implemented
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::LiteralExpr *element) {
        // Add nothing
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::LiteralExpr *element) {
        // Add nothing
    }

    void InterestingTransitionVisitor::IncrVisitor::_accept(const PQL::UnfoldedIdentifierExpr *element) {
        _stubborn.presetOf(element->offset(), true);
    }

    void InterestingTransitionVisitor::DecrVisitor::_accept(const PQL::UnfoldedIdentifierExpr *element) {
        _stubborn.postsetOf(element->offset(), true);
    }

}