/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "PetriEngine/PQL/ColoredPlaceUseVisitor.h"

namespace PetriEngine::PQL {

    void ColoredPlaceUseVisitor::_accept(const NotCondition *element) {
        Visitor::visit(this, (*element)[0]);
    }

    void ColoredPlaceUseVisitor::_accept(const DeadlockCondition *element) {
        // no-op
    }

    void ColoredPlaceUseVisitor::_accept(const CompareConjunction *element) {
        // no-op
    }

    void ColoredPlaceUseVisitor::_accept(const SimpleQuantifierCondition *element) {
        Visitor::visit(this, (*element)[0]);
    }

    void ColoredPlaceUseVisitor::_accept(const LogicalCondition *element) {
        for (auto &cond : element->getOperands())
            Visitor::visit(this, cond);
    }

    void ColoredPlaceUseVisitor::_accept(const CompareCondition *element) {
        Visitor::visit(this, (*element)[0]);
        Visitor::visit(this, (*element)[1]);
    }

    void ColoredPlaceUseVisitor::_accept(const UntilCondition *element) {
        Visitor::visit(this, (*element)[0]);
        Visitor::visit(this, (*element)[1]);
    }

    void ColoredPlaceUseVisitor::_accept(const ShallowCondition *element) {
        // no-op
    }

    void ColoredPlaceUseVisitor::_accept(const BooleanCondition *element) {
        // no-op
    }

    void ColoredPlaceUseVisitor::_accept(const IdentifierExpr *element) {
        auto it = _placeNameToIndexMap.find(element->name());
        if (it == _placeNameToIndexMap.end())
            throw base_error("Unable to resolve identifier \"", element->name(), "\"");
        _inUse[it->second] = true;
    }

    void ColoredPlaceUseVisitor::_accept(const LiteralExpr *element) {
        // no-op
    }

    void ColoredPlaceUseVisitor::_accept(const MinusExpr *element) {
        Visitor::visit(this, (*element)[0]);
    }

    void ColoredPlaceUseVisitor::_accept(const NaryExpr *element) {
        for (auto &cond : element->expressions())
            Visitor::visit(this, cond);
    }
}
