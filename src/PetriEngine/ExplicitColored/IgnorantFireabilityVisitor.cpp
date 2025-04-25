#include "PetriEngine/ExplicitColored/Visitors/IgnorantFireabilityVisitor.h"

namespace PetriEngine::PQL {
    template<typename V, typename C>
    Condition::Result compare(V* visitor, C* condition) {
        std::cout << "Cardinality comparison not supported for fireability reduction" << std::endl;
        return Condition::RUNKNOWN;
    }

    void FireabilityEvaluateVisitor::_accept(SimpleQuantifierCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    //All fireability queries need to be converted into EF
    void FireabilityEvaluateVisitor::_accept(AGCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void FireabilityEvaluateVisitor::_accept(ControlCondition *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void FireabilityEvaluateVisitor::_accept(EFCondition *element) {
        Visitor::visit(this, (*element)[0]);
    }

    //All fireability queries need to be converted into EF
    void FireabilityEvaluateVisitor::_accept(AllPaths *element) {
        _return_value = {Condition::RUNKNOWN};
    }

    void FireabilityEvaluateVisitor::_accept(ExistPath *element) {
        Visitor::visit(this, element->child());
    }

    void FireabilityEvaluateVisitor::_accept(AndCondition *element) {
        auto res = Condition::RTRUE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_return_value == Condition::RFALSE) {
                return;
            }
            if (_return_value == Condition::RUNKNOWN)
                res = Condition::RUNKNOWN;
        }
        _return_value = {res};
    }

    void FireabilityEvaluateVisitor::_accept(OrCondition *element) {
        auto res = Condition::RFALSE;
        for (auto &c: element->getOperands()) {
            Visitor::visit(this, c);
            if (_return_value == Condition::RTRUE) {
                return;
            }
            if (_return_value == Condition::RUNKNOWN)
                res = Condition::RUNKNOWN;
        }
        _return_value = {res};
    }

    // Transition fireability is converted into a compare conjunction, where each input place is an element
    void FireabilityEvaluateVisitor::_accept(CompareConjunction *element) {
        //Res is false if it is impossible to reach a marking where the constraints are satisfied
        bool res = true;
        for (auto &c : element->constraints()) {
            res = _context.marking()[c._place] >= c._lower;
            if (!res) break;
        }
        if (!res) {
            _return_value = element->isNegated() ? Condition::RTRUE : Condition::RFALSE;
        }else {
            _return_value = Condition::RUNKNOWN;
        }
    }

    void FireabilityEvaluateVisitor::_accept(LessThanOrEqualCondition *element) {
        _return_value = {compare(this, element)};
    }

    void FireabilityEvaluateVisitor::_accept(LessThanCondition *element) {
        _return_value = {compare(this, element)};
    }

    void FireabilityEvaluateVisitor::_accept(EqualCondition *element) {
        _return_value = {compare(this, element)};
    }

    void FireabilityEvaluateVisitor::_accept(NotEqualCondition *element) {
        _return_value = {compare(this, element)};
    }

    void FireabilityEvaluateVisitor::_accept(NotCondition *element) {
        Visitor::visit(this, (*element)[0]);
        if (dynamic_cast<EFCondition*>((*element)[0].get())) {
            if (_return_value != Condition::RUNKNOWN)
                _return_value = {_return_value == Condition::RFALSE ? Condition::RTRUE : Condition::RFALSE};
            else
                _return_value = {Condition::RUNKNOWN};
        }else {

        }

    }

    void FireabilityEvaluateVisitor::_accept(BooleanCondition *element) {
        _return_value = {element->value ? Condition::RTRUE : Condition::RFALSE};
    }

    void FireabilityEvaluateVisitor::_accept(DeadlockCondition *element) {
        _return_value = {Condition::RUNKNOWN};
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
