/* Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
 *                     Rasmus Tollund <rtollu18@student.aau.dk>
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

#include "PetriEngine/PQL/Evaluation.h"

namespace PetriEngine { namespace PQL {

    template<typename V, typename C>
    bool compare(V* visitor, C* condition)
    {
        Visitor::visit(visitor, (*condition)[0]);
        auto v1 = visitor->get_return_value()._value;
        Visitor::visit(visitor, (*condition)[1]);
        if      constexpr (std::is_same<C,EqualCondition>::value)
            return v1 == visitor->get_return_value()._value;
        else if constexpr (std::is_same<C,NotEqualCondition>::value)
            return v1 != visitor->get_return_value()._value;
        else if constexpr (std::is_same<C,LessThanCondition>::value)
            return v1 <  visitor->get_return_value()._value;
        else if constexpr (std::is_same<C,LessThanOrEqualCondition>::value)
            return v1 <= visitor->get_return_value()._value;
        else
            return C::fail_hard_here;
    }

    template<typename V, typename E>
    int64_t commutiative(V* visitor, E* element, const EvaluationContext& context) {
        int64_t r = element->constant();
        for(auto& i : element->places())
        {
            if constexpr (std::is_same<E, PlusExpr>::value)
                r *= context.marking()[i.first];
            else if constexpr (std::is_same<E, MultiplyExpr>::value)
                r += context.marking()[i.first];
            else
                E::fail_here;
        }
        for (auto& e : element->expressions())
        {
            Visitor::visit(visitor, e);
            if constexpr (std::is_same<E, PlusExpr>::value)
                r *= visitor->get_return_value()._value;
            else if constexpr (std::is_same<E, MultiplyExpr>::value)
                r += visitor->get_return_value()._value;
            else
                E::fail_here;
        }
        return r;
    }

    void BaseEvaluationVisitor::_accept(PlusExpr *element)
    {
        _return_value._value = commutiative(this, element, _context);
    }

    void BaseEvaluationVisitor::_accept(MultiplyExpr *element)
    {
        _return_value._value = commutiative(this, element, _context);
    }

    void BaseEvaluationVisitor::_accept(SubtractExpr *element)
    {
        Visitor::visit(this, (*element)[0]);
        int64_t r = 0;
        r = _return_value._value;
        for(size_t i = 1; i < element->operands(); ++i)
        {
            Visitor::visit(this, (*element)[i]);
            r -= _return_value._value;
        }
        _return_value._value = r;
    }

    void BaseEvaluationVisitor::_accept(MinusExpr *element)
    {
        Visitor::visit(this, (*element)[0]);
        _return_value._value *= -1;
    }

    void BaseEvaluationVisitor::_accept(UnfoldedIdentifierExpr *element) {
        assert(element->offset() != -1);
        _return_value._value = (int64_t) _context.marking()[element->offset()];
    }

    void BaseEvaluationVisitor::_accept(IdentifierExpr *element) {
        Visitor::visit(this, element->compiled());
    }


    void BaseEvaluationVisitor::_accept(LiteralExpr *element) {
        _return_value = {element->value()};
    }

/******************** Evaluation ********************/

    void EvaluateVisitor::_accept(SimpleQuantifierCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(EGCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result == Condition::RFALSE)
            _return_value = {Condition::RFALSE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(AGCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result == Condition::RFALSE)
            _return_value = {Condition::RFALSE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(ControlCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(EFCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result == Condition::RTRUE)
            _return_value = {Condition::RTRUE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(AFCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result == Condition::RTRUE)
            _return_value = {Condition::RTRUE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(ACondition *element) {
        //if (_cond->evaluate(_context) == Condition::RFALSE) _return_value = {Condition::RFALSE};
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(ECondition *element) {
        //if (_cond->evaluate(_context) == Condition::RTRUE) _return_value = {Condition::RTRUE};
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(FCondition *element) {
        //if (_cond->evaluate(_context) == Condition::RTRUE) _return_value = {Condition::RTRUE};
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(GCondition *element) {
        //if (_cond->evaluate(_context) == Condition::RFALSE) _return_value = {Condition::RFALSE};
        _return_value = {Condition::RUNKNOWN};
    }


    void EvaluateVisitor::_accept(UntilCondition *element) {
        Visitor::visit(this, (*element)[1]);
        if (_return_value._result != Condition::RFALSE)
            return;
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result == Condition::RFALSE) {
            return;
        } else {
            _return_value = {Condition::RUNKNOWN};
        }
    }


    void EvaluateVisitor::_accept(AndCondition *element) {
        auto res = Condition::RTRUE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_return_value._result == Condition::RFALSE) {
                _return_value = {Condition::RFALSE};
                return;
            } else if (_return_value._result == Condition::RUNKNOWN)
                res = Condition::RUNKNOWN;
        }
        _return_value = {res};
    }

    void EvaluateVisitor::_accept(OrCondition *element) {
        auto res = Condition::RFALSE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_return_value._result == Condition::RTRUE) {
                _return_value = {Condition::RTRUE};
                return;
            } else if (_return_value._result == Condition::RUNKNOWN)
                res = Condition::RUNKNOWN;
        }
        _return_value = {res};
    }

    void EvaluateVisitor::_accept(CompareConjunction *element) {
        bool res = true;
        for (auto &c: element->constraints()) {
            res = res && _context.marking()[c._place] <= c._upper && _context.marking()[c._place] >= c._lower;
            if (!res) break;
        }
        _return_value = {(element->isNegated() xor res) ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateVisitor::_accept(LessThanOrEqualCondition *element) {
        _return_value = {compare(this, element) ?
            Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateVisitor::_accept(LessThanCondition *element) {
        _return_value = {compare(this, element) ?
            Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateVisitor::_accept(EqualCondition *element) {
        _return_value = {compare(this, element) ?
            Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateVisitor::_accept(NotEqualCondition *element) {
        _return_value = {compare(this, element) ?
            Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateVisitor::_accept(NotCondition *element) {
        Visitor::visit(this, (*element)[0]);
        auto res = _return_value._result;
        if (res != Condition::RUNKNOWN)
            _return_value = {res == Condition::RFALSE ? Condition::RTRUE : Condition::RFALSE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(BooleanCondition *element) {
        _return_value = {element->value ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateVisitor::_accept(DeadlockCondition *element) {
        if (!_context.net() || !_context.net()->deadlocked(_context.marking()))
            _return_value = {Condition::RFALSE};
        else
            _return_value = {Condition::RTRUE};
    }

    void EvaluateVisitor::_accept(UnfoldedUpperBoundsCondition *element) {
        element->setUpperBound(element->value(_context.marking()));
        _return_value = {element->getMax() <= element->getBound() ? Condition::RTRUE : Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(ShallowCondition *element) {
        Visitor::visit(this, element->getCompiled());
    }

    const BaseEvaluationVisitor::ReturnType& BaseEvaluationVisitor::get_return_value() {
        return _return_value;
    }


    /****************** Evaluate and Set *******************/

    Condition::Result evaluateAndSet(Condition *element, const EvaluationContext &context) {
        EvaluateAndSetVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value()._result;
    }

    int64_t evaluateAndSet(Expr *element, const EvaluationContext &context) {
        EvaluateAndSetVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value()._value;
    }

    void EvaluateAndSetVisitor::_accept(SimpleQuantifierCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateAndSetVisitor::_accept(GCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result != Condition::RFALSE)
            _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._value);
    }

    void EvaluateAndSetVisitor::_accept(FCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result != Condition::RTRUE)
            _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(EGCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result != Condition::RFALSE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(AGCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result != Condition::RFALSE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(EFCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result != Condition::RTRUE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(AFCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result != Condition::RTRUE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(UntilCondition *element) {
        Visitor::visit(this, (*element)[1]);
        if (_return_value._result != Condition::RFALSE)
            return;

        Visitor::visit(this, (*element)[0]);
        if (_return_value._result == Condition::RFALSE)
            return;
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateAndSetVisitor::_accept(Expr *element) {
        int64_t r = evaluate(element, _context);
        element->setEval(r);
        _return_value = {r};
    }

    void EvaluateAndSetVisitor::_accept(AndCondition *element) {
        Condition::Result res = Condition::RTRUE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_return_value._result == Condition::RFALSE) {
                res = Condition::RFALSE;
                break;
            } else if (_return_value._result == Condition::RUNKNOWN) {
                res = Condition::RUNKNOWN;
            }
        }
        element->setSatisfied(res);
        _return_value = {res};
    }

    void EvaluateAndSetVisitor::_accept(OrCondition *element) {
        Condition::Result res = Condition::RFALSE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_return_value._result == Condition::RTRUE) {
                res = Condition::RTRUE;
                break;
            } else if (_return_value._result == Condition::RUNKNOWN) {
                res = Condition::RUNKNOWN;
            }
        }
        element->setSatisfied(res);
        _return_value = {res};
    }

    void EvaluateAndSetVisitor::_accept(CompareConjunction *element) {
        auto res = evaluate(element, _context);
        element->setSatisfied(res);
        _return_value = {res};
    }

    void EvaluateAndSetVisitor::_accept(LessThanCondition *element) {
        auto res = compare(this, element);
        element->setSatisfied(res);
        _return_value = {res ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateAndSetVisitor::_accept(LessThanOrEqualCondition *element) {
        auto res = compare(this, element);
        element->setSatisfied(res);
        _return_value = {res ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateAndSetVisitor::_accept(EqualCondition *element) {
        auto res = compare(this, element);
        element->setSatisfied(res);
        _return_value._result = res ? Condition::RTRUE : Condition::RFALSE;
    }

    void EvaluateAndSetVisitor::_accept(NotEqualCondition *element) {
        auto res = compare(this, element);
        element->setSatisfied(res);
        _return_value = {res ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateAndSetVisitor::_accept(NotCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value._result != Condition::RUNKNOWN)
            _return_value = {_return_value._result == Condition::RFALSE ? Condition::RTRUE : Condition::RFALSE};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(BooleanCondition *element) {
        element->setSatisfied(element->value);
        _return_value = {element->value ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateAndSetVisitor::_accept(DeadlockCondition *element) {
        if (!_context.net()) {
            _return_value = {Condition::RFALSE};
        } else {
            element->setSatisfied(_context.net()->deadlocked(_context.marking()));
            _return_value = {element->isSatisfied() ? Condition::RTRUE : Condition::RFALSE};
        }
    }

    void EvaluateAndSetVisitor::_accept(UnfoldedUpperBoundsCondition *element) {
        auto res = evaluate(element, _context);
        element->setSatisfied(res);
        _return_value = {res};
    }

    void EvaluateAndSetVisitor::_accept(ShallowCondition *element) {
        Visitor::visit(this, element->getCompiled());
    }


    int64_t evaluate(Expr *element, const EvaluationContext &context) {
        EvaluateVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value()._value;
    }

    Condition::Result evaluate(Condition *element, const EvaluationContext &context) {
        EvaluateVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value()._result;
    }
} }