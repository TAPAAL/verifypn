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

namespace PetriEngine::PQL {

    int32_t EvaluateVisitor::pre_op(const NaryExpr *element) {
        Visitor::visit(this, (*element)[0]);
        return _return_value._value;
    }

    int32_t EvaluateVisitor::pre_op(const CommutativeExpr *element) {
        int32_t res = element->constant();
        for (auto &i: element->places())
            res = apply(element, res, _context.marking()[i.first]);
        if (element->operands() > 0)
            res = apply(element, res, evaluateAndSet((*element)[0].get(), _context));
        return res;
    }

/******************** Evaluation ********************/

    void EvaluateVisitor::_accept(NaryExpr *element) {
        int32_t r = pre_op(element);
        for (size_t i = 1; i < element->operands(); ++i) {
            r = apply(element, r, evaluateAndSet((*element)[i].get(), _context));
        }
        _return_value = {r};
    }

    void EvaluateVisitor::_accept(CommutativeExpr *element) {
        if (element->operands() == 0)
            _return_value = {pre_op(element)};
        else
            element->NaryExpr::visit(*this); // Will implicitly set _return_value
    }

    void EvaluateVisitor::_accept(MinusExpr *element) {
        Visitor::visit(this, (*element)[0]);
        _return_value = {-_return_value._value};
    }

    void EvaluateVisitor::_accept(LiteralExpr *element) {
        _return_value = {element->value()};
    }

    void EvaluateVisitor::_accept(UnfoldedIdentifierExpr *element) {
        assert(element->offset() != -1);
        _return_value = {(int) _context.marking()[element->offset()]};
    }

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

/*        void EvaluationVisitor::_accept(XCondition *element) {
            return _cond->evaluate(_context);
        }*/

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

    void EvaluateVisitor::_accept(CompareCondition *element) {
        Visitor::visit(this, (*element)[0]);
        int v1 = _return_value._value;
        Visitor::visit(this, (*element)[1]);
        int v2 = _return_value._value;
        _return_value = {apply(element, v1, v2) ? Condition::RTRUE : Condition::RFALSE};
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

    void EvaluateVisitor::_accept(IdentifierExpr *element) {
        Visitor::visit(this, element->compiled());
    }

    void EvaluateVisitor::_accept(ShallowCondition *element) {
        Visitor::visit(this, element->getCompiled());
    }

    BaseEvaluationVisitor::ReturnType BaseEvaluationVisitor::get_return_value() {
        return _return_value;
    }

    bool BaseEvaluationVisitor::apply(const Condition *element, int lhs, int rhs) {
        _apply_visitor.set_args(lhs, rhs);
        Visitor::visit(&_apply_visitor, element);
        return _apply_visitor.get_return_value();
    }

    int BaseEvaluationVisitor::apply(const Expr *element, int lhs, int rhs) {
        _apply_visitor.set_args(lhs, rhs);
        Visitor::visit(&_apply_visitor, element);
        return _apply_visitor.get_return_value();
    }

    /****************** Evaluate and Set *******************/

    Condition::Result evaluateAndSet(Condition *element, const EvaluationContext &context) {
        EvaluateAndSetVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value()._result;
    }

    int evaluateAndSet(Expr *element, const EvaluationContext &context) {
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
        int r = evaluate(element, _context);
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

    void EvaluateAndSetVisitor::_accept(CompareCondition *element) {
        Visitor::visit(this, (*element)[0]);
        int v1 = _return_value._value;
        Visitor::visit(this, (*element)[1]);
        int v2 = _return_value._value;

        bool res = apply(element, v1, v2);
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


    /******************** Apply Visitor ********************/

    void ApplyVisitor::_accept(const PlusExpr *element) {
        _return_value = _lhs + _rhs;
    }

    void ApplyVisitor::_accept(const SubtractExpr *element) {
        _return_value = _lhs - _rhs;
    }

    void ApplyVisitor::_accept(const MultiplyExpr *element) {
        _return_value = _lhs * _rhs;
    }

    void ApplyVisitor::_accept(const EqualCondition *element) {
        _return_value = _lhs == _rhs;
    }

    void ApplyVisitor::_accept(const NotEqualCondition *element) {
        _return_value = _lhs != _rhs;
    }

    void ApplyVisitor::_accept(const LessThanCondition *element) {
        _return_value = _lhs < _rhs;
    }

    void ApplyVisitor::_accept(const LessThanOrEqualCondition *element) {
        _return_value = _lhs <= _rhs;
    }

    int evaluate(Expr *element, const EvaluationContext &context) {
        EvaluateVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value()._value;
    }

    Condition::Result evaluate(Condition *element, const EvaluationContext &context) {
        EvaluateVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value()._result;
    }

    int temp_apply(Expr *element, int lhs, int rhs) {
        ApplyVisitor visitor;
        visitor.set_args(lhs, rhs);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value();
    }

    bool temp_apply(Condition *element, int lhs, int rhs) {
        ApplyVisitor visitor;
        visitor.set_args(lhs, rhs);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value();
    }
}