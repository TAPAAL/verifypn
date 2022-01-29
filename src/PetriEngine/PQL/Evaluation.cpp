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
    int32_t EvaluationVisitor::pre_op(const NaryExpr *element) {
        (*element)[0]->visit(*this);
        return _return_value._value;
    }

    int32_t EvaluationVisitor::pre_op(const CommutativeExpr *element) {
        int32_t res = element->constant();
        for (auto &i: element->places())
            res = apply(element, res, _context.marking()[i.first]);
        if (element->operands() > 0)
            res = apply(element, res, (*element)[0]->evalAndSet(_context));
        return res;
    }

/******************** Evaluation ********************/

    void EvaluationVisitor::_accept(NaryExpr *element) {
        int32_t r = pre_op(element);
        for (size_t i = 1; i < element->operands(); ++i) {
            r = apply(element, r, (*element)[i]->evalAndSet(_context));
        }
        _return_value = {r};
    }

    void EvaluationVisitor::_accept(CommutativeExpr *element) {
        if (element->operands() == 0)
            _return_value = {pre_op(element)};
        else
            element->NaryExpr::visit(*this); // Will implicitly set _return_value
    }

    void EvaluationVisitor::_accept(MinusExpr *element) {
        (*element)[0]->visit(*this);
        _return_value = {-_return_value._value};
    }

    void EvaluationVisitor::_accept(LiteralExpr *element) {
        _return_value = {element->value()};
    }

    void EvaluationVisitor::_accept(UnfoldedIdentifierExpr *element) {
        assert(element->offset() != -1);
        _return_value = {(int)_context.marking()[element->offset()]};
    }

    void EvaluationVisitor::_accept(SimpleQuantifierCondition *element) {
        _return_value = {._result = Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(EGCondition *element) {
        (*element)[0]->visit(*this);
        if (_return_value._result == Condition::RFALSE)
            _return_value = {._result = Condition::RFALSE};
        else
            _return_value = {._result = Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(AGCondition *element) {
        (*element)[0]->visit(*this);
        if (_return_value._result == Condition::RFALSE)
            _return_value = {._result = Condition::RFALSE};
        else
            _return_value = {._result = Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(ControlCondition *element) {
        _return_value = {._result = Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(EFCondition *element) {
        (*element)[0]->visit(*this);
        if (_return_value._result == Condition::RTRUE)
            _return_value = {._result = Condition::RTRUE};
        else
            _return_value = {._result = Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(AFCondition *element) {
        (*element)[0]->visit(*this);
        if (_return_value._result == Condition::RTRUE)
            _return_value = {._result = Condition::RTRUE};
        else
            _return_value = {._result = Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(ACondition *element) {
        //if (_cond->evaluate(_context) == Condition::RFALSE) _return_value = {Condition::RFALSE};
        _return_value = {._result = Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(ECondition *element) {
        //if (_cond->evaluate(_context) == Condition::RTRUE) _return_value = {Condition::RTRUE};
        _return_value = {._result = Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(FCondition *element) {
        //if (_cond->evaluate(_context) == Condition::RTRUE) _return_value = {Condition::RTRUE};
        _return_value = {._result = Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(GCondition *element) {
        //if (_cond->evaluate(_context) == Condition::RFALSE) _return_value = {Condition::RFALSE};
        _return_value = {._result = Condition::RUNKNOWN};
    }

/*        void EvaluationVisitor::_accept(XCondition *element) {
            return _cond->evaluate(_context);
        }*/

    void EvaluationVisitor::_accept(UntilCondition *element) {
        (*element)[1]->visit(*this);
        if (_return_value._result != Condition::RFALSE)
            return;
        (*element)[0]->visit(*this);
        if (_return_value._result == Condition::RFALSE) {
            return;
        } else {
            _return_value = {._result = Condition::RUNKNOWN};
        }
    }


    void EvaluationVisitor::_accept(AndCondition *element) {
        auto res = Condition::RTRUE;
        for (auto &c: element->getOperands()) {
            c->visit(*this);
            if (_return_value._result == Condition::RFALSE) {
                _return_value = {._result = Condition::RFALSE};
                return;
            }
            else if (_return_value._result == Condition::RUNKNOWN)
                res = Condition::RUNKNOWN;
        }
        _return_value = {._result = res};
    }

    void EvaluationVisitor::_accept(OrCondition *element) {
        auto res = Condition::RFALSE;
        for (auto &c: element->getOperands()) {
            c->visit(*this);
            if (_return_value._result == Condition::RTRUE) {
                _return_value = {._result = Condition::RTRUE};
                return;
            }
            else if (_return_value._result == Condition::RUNKNOWN)
                res = Condition::RUNKNOWN;
        }
        _return_value = {._result = res};
    }

    void EvaluationVisitor::_accept(CompareConjunction *element) {
        bool res = true;
        for (auto &c: element->constraints()) {
            res = res && _context.marking()[c._place] <= c._upper && _context.marking()[c._place] >= c._lower;
            if (!res) break;
        }
        _return_value = {._result = (element->isNegated() xor res) ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluationVisitor::_accept(CompareCondition *element) {
        (*element)[0]->visit(*this);
        int v1 = _return_value._value;
        (*element)[1]->visit(*this);
        int v2 = _return_value._value;
        _return_value = {._result = apply(element, v1, v2) ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluationVisitor::_accept(NotCondition *element) {
        (*element)[0]->visit(*this);
        auto res = _return_value._result;
        if (res != Condition::RUNKNOWN)
            _return_value = {._result = res == Condition::RFALSE ? Condition::RTRUE : Condition::RFALSE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(BooleanCondition *element) {
        _return_value = {._result = element->value ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluationVisitor::_accept(DeadlockCondition *element) {
        if (!_context.net() || !_context.net()->deadlocked(_context.marking()))
            _return_value = {Condition::RFALSE};
        else
            _return_value = {Condition::RTRUE};
    }

    void EvaluationVisitor::_accept(UnfoldedUpperBoundsCondition *element) {
        element->setUpperBound(element->value(_context.marking()));
        _return_value = {._result = element->getMax() <= element->getBound() ? Condition::RTRUE : Condition::RUNKNOWN};
    }

    void EvaluationVisitor::_accept(IdentifierExpr *element) {
        element->compiled()->visit(*this);
    }

    void EvaluationVisitor::_accept(ShallowCondition *element) {
        element->getCompiled()->visit(*this);
    }

    EvaluationVisitor::EvaluationReturnType EvaluationVisitor::get_return_value() {
        return _return_value;
    }

    bool EvaluationVisitor::apply(const Condition *element, int lhs, int rhs) {
        _apply_visitor.set_args(lhs, rhs);
        element->visit(_apply_visitor);
        return _apply_visitor.get_return_value();
    }

    int EvaluationVisitor::apply(const Expr *element, int lhs, int rhs) {
        _apply_visitor.set_args(lhs, rhs);
        element->visit(_apply_visitor);
        return _apply_visitor.get_return_value();
    }


    /******************** Apply Visitor ********************/

    void ApplyVisitor::_accept( const PlusExpr *element) {
        _return_value = _lhs + _rhs;
    }

    void ApplyVisitor::_accept( const SubtractExpr *element) {
        _return_value = _lhs - _rhs;
    }

    void ApplyVisitor::_accept( const MultiplyExpr *element) {
        _return_value = _lhs * _rhs;
    }

    void ApplyVisitor::_accept( const EqualCondition *element) {
        _return_value = _lhs == _rhs;
    }

    void ApplyVisitor::_accept( const NotEqualCondition *element) {
        _return_value = _lhs != _rhs;
    }

    void ApplyVisitor::_accept( const LessThanCondition *element) {
        _return_value = _lhs < _rhs;
    }

    void ApplyVisitor::_accept( const LessThanOrEqualCondition *element) {
        _return_value = _lhs <= _rhs;
    }

    int evaluate(Expr *element, const EvaluationContext &context) {
        EvaluationVisitor visitor(context);
        element->visit(visitor);
        return visitor.get_return_value()._value;
    }

    Condition::Result evaluate(Condition *element, const EvaluationContext &context) {
        EvaluationVisitor visitor(context);
        element->visit(visitor);
        return visitor.get_return_value()._result;
    }

    int temp_apply(Expr *element, int lhs, int rhs) {
        ApplyVisitor visitor;
        visitor.set_args(lhs, rhs);
        element->visit(visitor);
        return visitor.get_return_value();
    }

    bool temp_apply(Condition *element, int lhs, int rhs) {
        ApplyVisitor visitor;
        visitor.set_args(lhs, rhs);
        element->visit(visitor);
        return visitor.get_return_value();
    }
}