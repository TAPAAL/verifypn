/* Copyright (C) 2011  Rasmus Grønkjær Tollund <rasmusgtollund@gmail.com>
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

#include "PetriEngine/PQL/FormulaSize.h"

#define RETURN(x) {_return_value = x; return;}

using namespace PetriEngine::PQL;

int PetriEngine::PQL::formulaSize(const Condition_constptr& condition) {
    FormulaSizeVisitor formulaSizeVisitor;
    Visitor::visit(formulaSizeVisitor, condition);
    return formulaSizeVisitor.getReturnValue();
}

int formulaSize(const Expr_ptr &element) {
    FormulaSizeVisitor formulaSizeVisitor;
    Visitor::visit(formulaSizeVisitor, element);
    return formulaSizeVisitor.getReturnValue();
}


void FormulaSizeVisitor::_accept(const CompareConjunction *condition) {
    int sum = 0;
    for(auto& c : condition->constraints())
    {
        assert(c._place >= 0);
        if(c._lower == c._upper) ++sum;
        else {
            if(c._lower != 0) ++sum;
            if(c._upper != std::numeric_limits<uint32_t>::max()) ++sum;
        }
    }
    if(sum == 1) RETURN(2)
    else RETURN((sum*2) + 1)
}

void FormulaSizeVisitor::_accept(const CompareCondition *condition) {
    RETURN(subvisit(condition->getExpr1()) + subvisit(condition->getExpr2()) + 1)
}

void FormulaSizeVisitor::_accept(const BooleanCondition *condition) {
    RETURN(0)
}

void FormulaSizeVisitor::_accept(const DeadlockCondition *condition) {
    RETURN(1)
}

void FormulaSizeVisitor::_accept(const UnfoldedUpperBoundsCondition *condition) {
    RETURN(condition->places().size())
}

void FormulaSizeVisitor::_accept(const NaryExpr *element) {
    size_t sum = 0;
    for(auto& e : element->expressions())
        sum += subvisit(e);
    RETURN(sum + 1)
}

void FormulaSizeVisitor::_accept(const CommutativeExpr *element) {
    size_t sum = element->places().size();
    for(auto& e : element->expressions())
        sum += subvisit(e);
    RETURN(sum + 1)
}

void FormulaSizeVisitor::_accept(const MinusExpr *element) {
    RETURN(subvisit((*element)[0]))
}

void FormulaSizeVisitor::_accept(const LiteralExpr *element) {
    RETURN(1)
}

void FormulaSizeVisitor::_accept(const IdentifierExpr *element) {
    if(element->compiled())
        RETURN(subvisit(element->compiled()));
    RETURN(1);
}

void FormulaSizeVisitor::_accept(const UnfoldedIdentifierExpr *condition) {
    RETURN(1)
}

void FormulaSizeVisitor::_accept(const ShallowCondition *condition) {
    RETURN(subvisit(condition->getCompiled()))
}

void FormulaSizeVisitor::_accept(const NotCondition *condition) {
    RETURN(subvisit(condition->getCond()) + 1)
}

void FormulaSizeVisitor::_accept(const SimpleQuantifierCondition *condition) {
    RETURN(subvisit(condition->getCond()) + 1)
}

void FormulaSizeVisitor::_accept(const UntilCondition *condition) {
    RETURN(subvisit(condition->getCond1()) + subvisit(condition->getCond2()) + 1)
}

void FormulaSizeVisitor::_accept(const LogicalCondition *condition) {
    size_t i = 1;
    for(auto& c : *condition)
        i += subvisit(c);
    RETURN(i)
}


void FormulaSizeVisitor::_accept(const PathSelectCondition* c)
{
    RETURN(subvisit(c->child()) + 1)
}

void FormulaSizeVisitor::_accept(const PathQuant* c)
{
    RETURN(subvisit(c->child()) + 1)
}

void FormulaSizeVisitor::_accept(const PathSelectExpr* e)
{
    RETURN(subvisit(e->child()) + 1)
}