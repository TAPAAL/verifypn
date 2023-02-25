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

#define RETURN(x,y) { _return_size = x; _return_depth = y; return;}

using namespace PetriEngine::PQL;

int64_t PetriEngine::PQL::formulaSize(const Condition_constptr& condition) {
    FormulaSizeVisitor formulaSizeVisitor;
    Visitor::visit(formulaSizeVisitor, condition);
    return formulaSizeVisitor.size();
}

int64_t formulaSize(const Expr_ptr &element) {
    FormulaSizeVisitor formulaSizeVisitor;
    Visitor::visit(formulaSizeVisitor, element);
    return formulaSizeVisitor.size();
}


void FormulaSizeVisitor::_accept(const CompareConjunction *condition) {
    int64_t sum = 0;
    for(auto& c : condition->constraints())
    {
        assert(c._place >= 0);
        if(c._lower == c._upper) ++sum;
        else {
            if(c._lower != 0) ++sum;
            if(c._upper != std::numeric_limits<uint32_t>::max()) ++sum;
        }
    }
    if(sum == 1) RETURN(2, 0)
    else RETURN((sum*2) + 1, 0)
}

void FormulaSizeVisitor::_accept(const CompareCondition *condition) {
    auto [x1, y1] = subvisit(condition->getExpr1());
    auto [x2, y2] = subvisit(condition->getExpr2());
    RETURN(x1 + x2 + 1, 0)
}

void FormulaSizeVisitor::_accept(const BooleanCondition *condition) {
    RETURN(0, 0)
}

void FormulaSizeVisitor::_accept(const DeadlockCondition *condition) {
    RETURN(1, 0)
}

void FormulaSizeVisitor::_accept(const UnfoldedUpperBoundsCondition *condition) {
    RETURN(condition->places().size(), 0)
}

void FormulaSizeVisitor::_accept(const NaryExpr *element) {
    size_t sum = 0;
    for(auto& e : element->expressions())
    {
        auto [x, _] = subvisit(e);
        sum += x;
    }
    RETURN(sum + 1, 0)
}

void FormulaSizeVisitor::_accept(const CommutativeExpr *element) {
    size_t sum = element->places().size();
    for(auto& e : element->expressions())
    {
        auto [x, _] = subvisit(e);
        sum += x;
    }
    RETURN(sum + 1, 0)
}

void FormulaSizeVisitor::_accept(const MinusExpr *element) {
    auto [x, y] = subvisit((*element)[0]);
    RETURN(x, 0)
}

void FormulaSizeVisitor::_accept(const LiteralExpr *element) {
    RETURN(1, 0)
}

void FormulaSizeVisitor::_accept(const IdentifierExpr *element) {
    if(element->compiled())
    {
        auto [x, y] = subvisit(element->compiled());
        RETURN(x, y);
    }
    else RETURN(1, 0);
}

void FormulaSizeVisitor::_accept(const UnfoldedIdentifierExpr *condition) {
    RETURN(1, 0)
}

void FormulaSizeVisitor::_accept(const ShallowCondition *condition) {
    auto [x, y] = subvisit(condition->getCompiled());
    RETURN(x, y)
}

void FormulaSizeVisitor::_accept(const NotCondition *condition) {
    auto [x, y] = subvisit(condition->getCond());
    RETURN(x + 1, y + 1)
}

void FormulaSizeVisitor::_accept(const SimpleQuantifierCondition *condition) {
    auto [x, y] = subvisit(condition->getCond());
    RETURN(x + 1, y + 1)
}

void FormulaSizeVisitor::_accept(const UntilCondition *condition) {
    auto [x1, y1] = subvisit(condition->getCond1());
    auto [x2, y2] = subvisit(condition->getCond2());
    RETURN(x1 + x2, std::max(y1, y2) + 1)
}

void FormulaSizeVisitor::_accept(const LogicalCondition *condition) {
    int64_t i = 1;
    int64_t d = 0;
    for(auto& c : *condition)
    {
        auto [x, y] = subvisit(c);
        i += x;
        d = std::max(d, y);
    }
    RETURN(i, d)
}


void FormulaSizeVisitor::_accept(const PathSelectCondition* c)
{
    auto [x, y] = subvisit(c->child());
    RETURN(x + 1, y + 1)
}

void FormulaSizeVisitor::_accept(const PathQuant* c)
{
    auto [x, y] = subvisit(c->child());
    RETURN(x + 1, y + 1)
}

void FormulaSizeVisitor::_accept(const PathSelectExpr* e)
{
    auto [x, y] = subvisit(e->child());
    RETURN(x + 1, y + 1)
}