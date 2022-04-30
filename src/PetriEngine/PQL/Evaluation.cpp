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
        ExprEvalVisitor eval(visitor->context());
        Visitor::visit(eval, (*condition)[0]);

        auto v1 = eval.value();
        if constexpr (std::is_same<EvaluateAndSetVisitor,V>::value)
            (*condition)[0]->setEval(v1);
        Visitor::visit(eval, (*condition)[1]);
        if constexpr (std::is_same<EvaluateAndSetVisitor,V>::value)
            (*condition)[1]->setEval(eval.value());
        if      constexpr (std::is_same<C,EqualCondition>::value)
            return v1 == eval.value();
        else if constexpr (std::is_same<C,NotEqualCondition>::value)
            return v1 != eval.value();
        else if constexpr (std::is_same<C,LessThanCondition>::value)
            return v1 <  eval.value();
        else if constexpr (std::is_same<C,LessThanOrEqualCondition>::value)
            return v1 <= eval.value();
        else
            return C::fail_hard_here;
    }

    template<typename V, typename E>
    int64_t commutative(V* visitor, const E* element, const EvaluationContext& context) {
        int64_t r = element->constant();
        for(auto& i : element->places())
        {
            if constexpr (std::is_same<E, PlusExpr>::value)
                r += context.marking()[i.first];
            else if constexpr (std::is_same<E, MultiplyExpr>::value)
                r *= context.marking()[i.first];
            else
                E::fail_hard_here;
        }
        for (auto& e : element->expressions())
        {
            Visitor::visit(visitor, e);
            if constexpr (std::is_same<E, PlusExpr>::value)
                r += visitor->value();
            else if constexpr (std::is_same<E, MultiplyExpr>::value)
                r *= visitor->value();
            else
                E::fail_hard_here;
        }
        return r;
    }

    void ExprEvalVisitor::_accept(const PlusExpr *element)
    {
        _value = commutative(this, element, _context);
    }

    void ExprEvalVisitor::_accept(const MultiplyExpr *element)
    {
        _value = commutative(this, element, _context);
    }

    void ExprEvalVisitor::_accept(const SubtractExpr *element)
    {
        Visitor::visit(this, (*element)[0]);
        int64_t r = _value;
        for(size_t i = 1; i < element->operands(); ++i)
        {
            Visitor::visit(this, (*element)[i]);
            r -= _value;
        }
        _value = r;
    }

    void ExprEvalVisitor::_accept(const MinusExpr *element)
    {
        Visitor::visit(this, (*element)[0]);
        _value = -_value;
    }

    void ExprEvalVisitor::_accept(const UnfoldedIdentifierExpr *element) {
        assert(element->offset() != -1);
        _value = (int64_t) _context.marking()[element->offset()];
    }

    void ExprEvalVisitor::_accept(const IdentifierExpr *element) {
        Visitor::visit(this, element->compiled());
    }


    void ExprEvalVisitor::_accept(const LiteralExpr *element) {
        _value = {element->value()};
    }

/******************** Evaluation ********************/

    void EvaluateVisitor::_accept(SimpleQuantifierCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(EGCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value == Condition::RFALSE)
            _return_value = {Condition::RFALSE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(AGCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value == Condition::RFALSE)
            _return_value = {Condition::RFALSE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(ControlCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(EFCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value == Condition::RTRUE)
            _return_value = {Condition::RTRUE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(AFCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value == Condition::RTRUE)
            _return_value = {Condition::RTRUE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(ACondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value == Condition::RFALSE) _return_value = {Condition::RFALSE};
        else _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(ECondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value == Condition::RTRUE) _return_value = {Condition::RTRUE};
        else _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(FCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value == Condition::RTRUE) _return_value = {Condition::RTRUE};
        else _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(GCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value == Condition::RFALSE) _return_value = {Condition::RFALSE};
        else _return_value = {Condition::RUNKNOWN};
    }


    void EvaluateVisitor::_accept(UntilCondition *element) {
        Visitor::visit(this, (*element)[1]);
        if (_return_value != Condition::RFALSE)
        {
            // retain return, either true or unknown.
            return;
        }
        Visitor::visit(this, (*element)[0]);
        if (_return_value == Condition::RFALSE) {
            return;
        } else {
            _return_value = {Condition::RUNKNOWN};
        }
    }


    void EvaluateVisitor::_accept(AndCondition *element) {
        auto res = Condition::RTRUE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_return_value == Condition::RFALSE) {
                _return_value = {Condition::RFALSE};
                return;
            } else if (_return_value == Condition::RUNKNOWN)
                res = Condition::RUNKNOWN;
        }
        _return_value = {res};
    }

    void EvaluateVisitor::_accept(OrCondition *element) {
        auto res = Condition::RFALSE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_return_value == Condition::RTRUE) {
                _return_value = {Condition::RTRUE};
                return;
            } else if (_return_value == Condition::RUNKNOWN)
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
        auto res = _return_value;
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

    void EvaluateVisitor::_accept(UpperBoundsCondition *element) {
        if(element->getCompiled())
            Visitor::visit(this, element->getCompiled());
        else
            _return_value = Condition::RUNKNOWN;
    }

    void EvaluateVisitor::_accept(ShallowCondition *element) {
        Visitor::visit(this, element->getCompiled());
    }


    /****************** Evaluate and Set *******************/

    Condition::Result evaluateAndSet(Condition *element, const EvaluationContext &context) {
        EvaluateAndSetVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value();
    }

    void EvaluateAndSetVisitor::_accept(SimpleQuantifierCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateAndSetVisitor::_accept(GCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value != Condition::RFALSE)
            _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value);
    }

    void EvaluateAndSetVisitor::_accept(FCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value != Condition::RTRUE)
            _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value);
    }

    void EvaluateAndSetVisitor::_accept(EGCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value != Condition::RFALSE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value);
    }

    void EvaluateAndSetVisitor::_accept(AGCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value != Condition::RFALSE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value);
    }

    void EvaluateAndSetVisitor::_accept(EFCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value != Condition::RTRUE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value);
    }

    void EvaluateAndSetVisitor::_accept(AFCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value != Condition::RTRUE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value);
    }

    void EvaluateAndSetVisitor::_accept(UntilCondition *element) {
        Visitor::visit(this, (*element)[1]);
        if (_return_value != Condition::RFALSE)
            return;

        Visitor::visit(this, (*element)[0]);
        if (_return_value == Condition::RFALSE)
            return;
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateAndSetVisitor::_accept(AndCondition *element) {
        Condition::Result res = Condition::RTRUE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_return_value == Condition::RFALSE) {
                res = Condition::RFALSE;
                break;
            } else if (_return_value == Condition::RUNKNOWN) {
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
            if (_return_value == Condition::RTRUE) {
                res = Condition::RTRUE;
                break;
            } else if (_return_value == Condition::RUNKNOWN) {
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
        _return_value = res ? Condition::RTRUE : Condition::RFALSE;
    }

    void EvaluateAndSetVisitor::_accept(NotEqualCondition *element) {
        auto res = compare(this, element);
        element->setSatisfied(res);
        _return_value = {res ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateAndSetVisitor::_accept(NotCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (_return_value != Condition::RUNKNOWN)
            _return_value = {_return_value == Condition::RFALSE ? Condition::RTRUE : Condition::RFALSE};
        element->setSatisfied(_return_value);
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
        ExprEvalVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.value();
    }

    Condition::Result evaluate(Condition *element, const EvaluationContext &context) {
        EvaluateVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value();
    }
} }