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
    template<typename T, typename Q>
    void visit(T* visitor, const std::shared_ptr<Q>& c)
    {
        visit(visitor, c.get());
    }

    template<typename T, typename Q>
    void visit(T* visitor, const Q* c)
    {
        visit(visitor, const_cast<Q*>(c));
    }

    template<typename T, typename Q>
    void visit(T* visitor, Q* c)
    {
        switch(c->type())
        {
            case type_id<OrCondition>():
                visitor->accept(static_cast<OrCondition*>(c));
                break;
            case type_id<AndCondition>():
                visitor->accept(static_cast<AndCondition*>(c));
                break;
            case type_id<CompareConjunction>():
                visitor->accept(static_cast<CompareConjunction*>(c));
                break;
            case type_id<LessThanCondition>():
                visitor->accept(static_cast<LessThanCondition*>(c));
                break;
            case type_id<LessThanOrEqualCondition>():
                visitor->accept(static_cast<LessThanOrEqualCondition*>(c));
                break;
            case type_id<EqualCondition>():
                visitor->accept(static_cast<EqualCondition*>(c));
                break;
            case type_id<NotEqualCondition>():
                visitor->accept(static_cast<NotEqualCondition*>(c));
                break;
            case type_id<DeadlockCondition>():
                visitor->accept(static_cast<DeadlockCondition*>(c));
                break;
            case type_id<UnfoldedUpperBoundsCondition>():
                visitor->accept(static_cast<UnfoldedUpperBoundsCondition*>(c));
                break;
            case type_id<NotCondition>():
                visitor->accept(static_cast<NotCondition*>(c));
                break;
            case type_id<BooleanCondition>():
                visitor->accept(static_cast<BooleanCondition*>(c));
                break;
            case type_id<ECondition>():
                visitor->accept(static_cast<ECondition*>(c));
                break;
            case type_id<ACondition>():
                visitor->accept(static_cast<ACondition*>(c));
                break;
            case type_id<FCondition>():
                visitor->accept(static_cast<FCondition*>(c));
                break;
            case type_id<GCondition>():
                visitor->accept(static_cast<GCondition*>(c));
                break;
            case type_id<UntilCondition>():
                visitor->accept(static_cast<UntilCondition*>(c));
                break;
            case type_id<XCondition>():
                visitor->accept(static_cast<XCondition*>(c));
                break;
            case type_id<ControlCondition>():
                visitor->accept(static_cast<ControlCondition*>(c));
                break;
            case type_id<StableMarkingCondition>():
                visitor->accept(static_cast<StableMarkingCondition*>(c));
                break;
            case type_id<QuasiLivenessCondition>():
                visitor->accept(static_cast<QuasiLivenessCondition*>(c));
                break;
            case type_id<LivenessCondition>():
                visitor->accept(static_cast<LivenessCondition*>(c));
                break;
            case type_id<KSafeCondition>():
                visitor->accept(static_cast<KSafeCondition*>(c));
                break;
            case type_id<UpperBoundsCondition>():
                visitor->accept(static_cast<UpperBoundsCondition*>(c));
                break;
            case type_id<FireableCondition>():
                visitor->accept(static_cast<FireableCondition*>(c));
                break;
            case type_id<UnfoldedFireableCondition>():
                visitor->accept(static_cast<UnfoldedFireableCondition*>(c));
                break;
            case type_id<EFCondition>():
                visitor->accept(static_cast<EFCondition*>(c));
                break;
            case type_id<AGCondition>():
                visitor->accept(static_cast<AGCondition*>(c));
                break;
            case type_id<AUCondition>():
                visitor->accept(static_cast<AUCondition*>(c));
                break;
            case type_id<EUCondition>():
                visitor->accept(static_cast<EUCondition*>(c));
                break;
            case type_id<EXCondition>():
                visitor->accept(static_cast<EXCondition*>(c));
                break;
            case type_id<AXCondition>():
                visitor->accept(static_cast<AXCondition*>(c));
                break;
            case type_id<AFCondition>():
                visitor->accept(static_cast<AFCondition*>(c));
                break;
            case type_id<EGCondition>():
                visitor->accept(static_cast<EGCondition*>(c));
                break;
            default:
                __builtin_unreachable();
        }
    }


    int32_t EvaluateVisitor::pre_op(const NaryExpr *element) {
        (*element)[0]->visit(*this);
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
        (*element)[0]->visit(*this);
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
        visit(this, (*element)[0]);
        if (_return_value._result == Condition::RFALSE)
            _return_value = {Condition::RFALSE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(AGCondition *element) {
        visit(this, (*element)[0]);
        if (_return_value._result == Condition::RFALSE)
            _return_value = {Condition::RFALSE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(ControlCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(EFCondition *element) {
        visit(this, (*element)[0]);
        if (_return_value._result == Condition::RTRUE)
            _return_value = {Condition::RTRUE};
        else
            _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateVisitor::_accept(AFCondition *element) {
        visit(this, (*element)[0]);
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
        visit(this, (*element)[1]);
        if (_return_value._result != Condition::RFALSE)
            return;
        visit(this, (*element)[0]);
        if (_return_value._result == Condition::RFALSE) {
            return;
        } else {
            _return_value = {Condition::RUNKNOWN};
        }
    }


    void EvaluateVisitor::_accept(AndCondition *element) {
        auto res = Condition::RTRUE;
        for (auto &c: element->getOperands()) {
            visit(this, c);
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
            visit(this, c);
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
        (*element)[0]->visit(*this);
        int v1 = _return_value._value;
        (*element)[1]->visit(*this);
        int v2 = _return_value._value;
        _return_value = {apply(element, v1, v2) ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateVisitor::_accept(NotCondition *element) {
        visit(this, (*element)[0]);
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
        element->compiled()->visit(*this);
    }

    void EvaluateVisitor::_accept(ShallowCondition *element) {
        visit(this, element->getCompiled());
    }

    BaseEvaluationVisitor::ReturnType BaseEvaluationVisitor::get_return_value() {
        return _return_value;
    }

    bool BaseEvaluationVisitor::apply(const Condition *element, int lhs, int rhs) {
        _apply_visitor.set_args(lhs, rhs);
        visit(&_apply_visitor, element);
        return _apply_visitor.get_return_value();
    }

    int BaseEvaluationVisitor::apply(const Expr *element, int lhs, int rhs) {
        _apply_visitor.set_args(lhs, rhs);
        element->visit(_apply_visitor);
        return _apply_visitor.get_return_value();
    }

    /****************** Evaluate and Set *******************/

    Condition::Result evaluateAndSet(Condition *element, const EvaluationContext &context) {
        EvaluateAndSetVisitor visitor(context);
        visit(&visitor, element);
        return visitor.get_return_value()._result;
    }

    int evaluateAndSet(Expr *element, const EvaluationContext &context) {
        EvaluateAndSetVisitor visitor(context);
        element->visit(visitor);
        return visitor.get_return_value()._value;
    }

    void EvaluateAndSetVisitor::_accept(SimpleQuantifierCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void EvaluateAndSetVisitor::_accept(GCondition *element) {
        visit(this, (*element)[0]);
        if (_return_value._result != Condition::RFALSE)
            _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._value);
    }

    void EvaluateAndSetVisitor::_accept(FCondition *element) {
        visit(this, (*element)[0]);
        if (_return_value._result != Condition::RTRUE)
            _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(EGCondition *element) {
        visit(this, (*element)[0]);
        if (_return_value._result != Condition::RFALSE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(AGCondition *element) {
        visit(this, (*element)[0]);
        if (_return_value._result != Condition::RFALSE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(EFCondition *element) {
        visit(this, (*element)[0]);
        if (_return_value._result != Condition::RTRUE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(AFCondition *element) {
        visit(this, (*element)[0]);
        if (_return_value._result != Condition::RTRUE) _return_value = {Condition::RUNKNOWN};
        element->setSatisfied(_return_value._result);
    }

    void EvaluateAndSetVisitor::_accept(UntilCondition *element) {
        visit(this, (*element)[1]);
        if (_return_value._result != Condition::RFALSE)
            return;

        visit(this, (*element)[0]);
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
            visit(this, c);
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
            visit(this, c);
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
        (*element)[0]->visit(*this);
        int v1 = _return_value._value;
        (*element)[1]->visit(*this);
        int v2 = _return_value._value;

        bool res = apply(element, v1, v2);
        element->setSatisfied(res);
        _return_value = {res ? Condition::RTRUE : Condition::RFALSE};
    }

    void EvaluateAndSetVisitor::_accept(NotCondition *element) {
        visit(this, (*element)[0]);
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
        visit(this, element->getCompiled());
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
        element->visit(visitor);
        return visitor.get_return_value()._value;
    }

    Condition::Result evaluate(Condition *element, const EvaluationContext &context) {
        EvaluateVisitor visitor(context);
        visit(&visitor, element);
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
        visit(&visitor, element);
        return visitor.get_return_value();
    }
}