/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "PetriEngine/PQL/ColoredUseVisitor.h"

namespace PetriEngine::PQL {

    void ColoredUseVisitor::_accept(const NotCondition *element) {
        Visitor::visit(this, (*element)[0]);
    }

    void ColoredUseVisitor::_accept(const DeadlockCondition *element) {
        // no-op
    }

    void ColoredUseVisitor::_accept(const CompareConjunction *element) {
        // no-op
    }

    void ColoredUseVisitor::_accept(const SimpleQuantifierCondition *element) {
        Visitor::visit(this, (*element)[0]);
    }

    void ColoredUseVisitor::_accept(const LogicalCondition *element) {
        for (auto &cond : element->getOperands())
            Visitor::visit(this, cond);
    }

    void ColoredUseVisitor::_accept(const CompareCondition *element) {
        Visitor::visit(this, (*element)[0]);
        Visitor::visit(this, (*element)[1]);
    }

    void ColoredUseVisitor::_accept(const UntilCondition *element) {
        Visitor::visit(this, (*element)[0]);
        Visitor::visit(this, (*element)[1]);
    }

    void ColoredUseVisitor::_accept(const FireableCondition *element) {
        auto it = _transitionNameToIndexMap.find(element->getName());
        if (it == _transitionNameToIndexMap.end())
            throw base_error("Unable to resolve identifier \"", element->getName(), "\"");
        _transitionInUse[it->second] = true;
        _anyTransitionInUse = true;
    }

    void ColoredUseVisitor::_accept(const UpperBoundsCondition *element) {
        for(auto& p : element->getPlaces()) {
            auto it = _placeNameToIndexMap.find(p);
            if (it == _placeNameToIndexMap.end())
                throw base_error("Unable to resolve identifier \"", *p, "\"");
            _placeInUse[it->second] = true;
        }
    }

    void ColoredUseVisitor::_accept(const ShallowCondition *element) {
        // no-op
    }

    void ColoredUseVisitor::_accept(const BooleanCondition *element) {
        // no-op
    }

    void ColoredUseVisitor::_accept(const IdentifierExpr *element) {
        auto it = _placeNameToIndexMap.find(element->name());
        if (it == _placeNameToIndexMap.end())
            throw base_error("Unable to resolve identifier \"", element->name(), "\"");
        _placeInUse[it->second] = true;
    }

    void ColoredUseVisitor::_accept(const LiteralExpr *element) {
        // no-op
    }

    void ColoredUseVisitor::_accept(const MinusExpr *element) {
        Visitor::visit(this, (*element)[0]);
    }

    void ColoredUseVisitor::_accept(const NaryExpr *element) {
        for (auto &cond : element->expressions())
            Visitor::visit(this, cond);
    }
}
