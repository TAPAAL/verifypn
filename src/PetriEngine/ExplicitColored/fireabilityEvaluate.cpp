#include "PetriEngine/ExplicitColored/Visitors/FireabilityEvaluate.h"

namespace PetriEngine::PQL {
    template<typename V, typename C>
    Condition::Result compare(V* visitor, C* condition) {
        ExprEvalVisitor eval(visitor->context());
        Visitor::visit(eval, (*condition)[0]);
        const auto v1 = eval.value();
        if constexpr (std::is_same_v<C,EqualCondition>) {
            return Condition::RUNKNOWN;
        }
        else if constexpr (std::is_same_v<C,NotEqualCondition>) {
            return Condition::RUNKNOWN;
        }
        else if constexpr (std::is_same_v<C,LessThanCondition>) {
            if (v1 < eval.value()) {
                return Condition::RTRUE;
            }
            return Condition::RUNKNOWN;
        }
        else if constexpr (std::is_same_v<C,LessThanOrEqualCondition>) {
            if (v1 <= eval.value()) {
                return Condition::RTRUE;
            }
            return Condition::RUNKNOWN;
        }
        else {
            return C::fail_hard_here;
        }
    }

    // Condition::Result compare(V* visitor, C* condition) {
    //     ExprEvalVisitor eval(visitor->context());
    //     Visitor::visit(eval, (*condition)[0]);
    //     const auto v1 = eval.value();
    //     if constexpr (std::is_same_v<C,EqualCondition>) {
    //         return Condition::RUNKNOWN;
    //     }
    //     else if constexpr (std::is_same_v<C,NotEqualCondition>) {
    //         return Condition::RUNKNOWN;
    //     }
    //     else if constexpr (std::is_same_v<C,LessThanCondition>) {
    //         if (v1 < eval.value()) {
    //             return Condition::RTRUE;
    //         }
    //         return Condition::RUNKNOWN;
    //     }
    //     else if constexpr (std::is_same_v<C,LessThanOrEqualCondition>) {
    //         if (v1 <= eval.value()) {
    //             return Condition::RTRUE;
    //         }
    //         return Condition::RUNKNOWN;
    //     }
    //     else {
    //         return C::fail_hard_here;
    //     }
    // }
    void FireabilityEvaluateVisitor::_accept(SimpleQuantifierCondition *element) {
        _returnValue = {Condition::RUNKNOWN};
    }

    //All fireability queries need to be converted into EF
    void FireabilityEvaluateVisitor::_accept(AGCondition *element) {
        _returnValue = {Condition::RUNKNOWN};
    }

    void FireabilityEvaluateVisitor::_accept(ControlCondition *element) {
        _returnValue = {Condition::RUNKNOWN};
    }

    void FireabilityEvaluateVisitor::_accept(EFCondition *element) {
        Visitor::visit(this, (*element)[0]);
    }

    //All fireability queries need to be converted into EF
    void FireabilityEvaluateVisitor::_accept(AllPaths *element) {
        _returnValue = {Condition::RUNKNOWN};
    }

    void FireabilityEvaluateVisitor::_accept(ExistPath *element) {
        Visitor::visit(this, element->child());
    }

    void FireabilityEvaluateVisitor::_accept(AndCondition *element) {
        auto res = Condition::RTRUE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_returnValue == Condition::RFALSE) {
                return;
            }
            if (_returnValue == Condition::RUNKNOWN)
                res = Condition::RUNKNOWN;
        }
        _returnValue = {res};
    }

    void FireabilityEvaluateVisitor::_accept(OrCondition *element) {
        auto res = Condition::RFALSE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_returnValue == Condition::RTRUE) {
                return;
            }
            if (_returnValue == Condition::RUNKNOWN)
                res = Condition::RUNKNOWN;
        }
        _returnValue = {res};
    }

    // Transition fireability is converted into a compare conjunction, where each input place is an element
    void FireabilityEvaluateVisitor::_accept(CompareConjunction *element) {
        //Res is false if it is impossible to reach a marking where the constraints are satisfied
        bool res = true;
        for (auto &c : element->constraints()) {
            auto tokenCount = _context.marking()[c._place];
            res = _context.marking()[c._place] >= c._lower;
            if (!res) break;
        }
        if (!res) {
            _returnValue = element->isNegated() ? Condition::RTRUE : Condition::RFALSE;
        }else {
            _returnValue = Condition::RUNKNOWN;
        }
    }

    void FireabilityEvaluateVisitor::_accept(LessThanOrEqualCondition *element) {
        _returnValue = {compare(this, element)};
    }

    void FireabilityEvaluateVisitor::_accept(LessThanCondition *element) {
        _returnValue = {compare(this, element)};
    }

    void FireabilityEvaluateVisitor::_accept(EqualCondition *element) {
        _returnValue = {compare(this, element)};
    }

    void FireabilityEvaluateVisitor::_accept(NotEqualCondition *element) {
        _returnValue = {compare(this, element)};
    }

    void FireabilityEvaluateVisitor::_accept(NotCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (dynamic_cast<EFCondition*>((*element)[0].get())) {
            if (_returnValue != Condition::RUNKNOWN)
                _returnValue = {_returnValue == Condition::RFALSE ? Condition::RTRUE : Condition::RFALSE};
            else
                _returnValue = {Condition::RUNKNOWN};
        }else {

        }

    }

    void FireabilityEvaluateVisitor::_accept(BooleanCondition *element) {
        _returnValue = {element->value ? Condition::RTRUE : Condition::RFALSE};
    }

    void FireabilityEvaluateVisitor::_accept(DeadlockCondition *element) {
        _returnValue = {Condition::RUNKNOWN};
    }

    void FireabilityEvaluateVisitor::_accept(ShallowCondition *element) {
        Visitor::visit(this, element->getCompiled());
    }

    Condition::Result fireabilityEvaluate(Condition *element, const EvaluationContext &context) {
        FireabilityEvaluateVisitor visitor(context);
        Visitor::visit(&visitor, element);
        return visitor.get_return_value();
    }
}
