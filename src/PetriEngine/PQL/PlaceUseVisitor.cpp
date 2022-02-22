/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   PlaceUseVisitor.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 23, 2020, 8:44 PM
 */

#include "PetriEngine/PQL/PlaceUseVisitor.h"

namespace PetriEngine {
namespace PQL {

    PlaceUseVisitor::PlaceUseVisitor(size_t places)
    : _in_use(places)
    {

    }

    void PlaceUseVisitor::_accept(const NotCondition* element)
    {
        Visitor::visit(this, (*element)[0]);
    }

    void PlaceUseVisitor::_accept(const AndCondition* element)
    {
        for(auto& e : *element)
            Visitor::visit(this, e);
    }

    void PlaceUseVisitor::_accept(const OrCondition* element)
    {
        for(auto& e : *element)
            Visitor::visit(this, e);
    }

    void PlaceUseVisitor::_accept(const LessThanCondition* element)
    {
        for(auto i : {0,1})
            Visitor::visit(this, (*element)[i]);
    }

    void PlaceUseVisitor::_accept(const LessThanOrEqualCondition* element)
    {
        for(auto i : {0,1})
            Visitor::visit(this, (*element)[i]);
    }

    void PlaceUseVisitor::_accept(const EqualCondition* element)
    {
        for(auto i : {0,1})
            Visitor::visit(this, (*element)[i]);
    }

    void PlaceUseVisitor::_accept(const NotEqualCondition* element)
    {
        for(auto i : {0,1})
            Visitor::visit(this, (*element)[i]);
    }

    void PlaceUseVisitor::_accept(const CompareConjunction* element)
    {
        for(auto& e : *element)
            _in_use[e._place] = true;
    }

    void PlaceUseVisitor::visitCommutativeExpr(const CommutativeExpr* element)
    {
        for(auto& p : element->places())
            _in_use[p.first] = true;
        for(auto& e : element->expressions())
            Visitor::visit(this, e);
    }

    void PlaceUseVisitor::_accept(const PlusExpr* element)
    {
        visitCommutativeExpr(element);
    }

    void PlaceUseVisitor::_accept(const SubtractExpr* element)
    {
        for(auto& e : element->expressions())
            Visitor::visit(this, e);
    }

    void PlaceUseVisitor::_accept(const MultiplyExpr* element)
    {
        visitCommutativeExpr(element);
    }

    void PlaceUseVisitor::_accept(const MinusExpr* element)
    {
        Visitor::visit(this, (*element)[0]);
    }

    void PlaceUseVisitor::_accept(const UnfoldedIdentifierExpr* element)
    {
        _in_use[element->offset()] = true;
    }

    void PlaceUseVisitor::_accept(const UnfoldedUpperBoundsCondition* element)
    {
        for(auto& p : element->places())
            _in_use[p._place] = true;
    }

    void PlaceUseVisitor::_accept(const UntilCondition* element)
    {
        Visitor::visit(this, (*element)[0]);
        Visitor::visit(this, (*element)[1]);
    }

    void PlaceUseVisitor::_accept(const ReleaseCondition* element)
    {
        Visitor::visit(this, (*element)[0]);
        Visitor::visit(this, (*element)[1]);
    }


    void PlaceUseVisitor::_accept(const SimpleQuantifierCondition* element)
    {
        Visitor::visit(this, (*element)[0]);
    }


    // shallow elements, neither of these should exist in a compiled expression
    void PlaceUseVisitor::_accept(const LiteralExpr* element) {}
    void PlaceUseVisitor::_accept(const DeadlockCondition*) {}

}
}

