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

#include <PetriEngine/PQL/QueryPrinter.h>
#include "PetriEngine/PQL/ToTAPAALQuery.h"

using namespace PetriEngine::PQL;

void ToTAPAALQueryVisitor::_accept(const SimpleQuantifierCondition* condition) {
    _os << condition->op() << " ";
    condition->getCond()->visit(*this);
}

void ToTAPAALQueryVisitor::_accept(const UntilCondition* condition) {
    _os << condition->op() << " (";
    condition->getCond1()->visit(*this);
    _os << " U ";
    condition->getCond2()->visit(*this);
    _os << ")";
}

void ToTAPAALQueryVisitor::_accept(const LogicalCondition* condition) {
    _os << "(";
    (*condition)[0]->visit(*this);
    for(size_t i = 1; i < condition->operands(); ++i)
    {
        _os << " " << condition->op() << " ";
        (*condition)[i]->visit(*this);
    }
    _os << ")";
}

void ToTAPAALQueryVisitor::_accept(const CompareConjunction* condition) {
    _os << "(";
    if(condition->isNegated()) _os << "!";
    bool first = true;
    for(auto& c : condition->constraints())
    {
        if(!first) _os << " and ";
        if(c._lower != 0)
            _os << "(" << c._lower << " <= " <<  _context.netName << "." << c._name << ")";
        if(c._lower != 0 && c._upper != std::numeric_limits<uint32_t>::max())
            _os << " and ";
        if(c._lower != 0)
            _os << "(" << c._upper << " >= " <<  _context.netName << "." << c._name << ")";
        first = false;
    }
    _os << ")";
}

void ToTAPAALQueryVisitor::_accept(const CompareCondition* condition) {
    //If <id> <op> <literal>
    QueryPrinter printer;
    if (condition->getExpr1()->type() == Expr::IdentifierExpr && condition->getExpr2()->type() == Expr::LiteralExpr) {
        _os << " ( " << _context.netName << ".";
        condition->getExpr1()->visit(printer);
        _os << " " << condition->opTAPAAL() << " ";
        condition->getExpr2()->visit(printer);
        _os << " ) ";
        //If <literal> <op> <id>
    } else if (condition->getExpr2()->type() == Expr::IdentifierExpr && condition->getExpr1()->type() == Expr::LiteralExpr) {
        _os << " ( ";
        condition->getExpr1()->visit(printer);
        _os << " " << condition->sopTAPAAL() << " " <<  _context.netName << ".";
        condition->getExpr2()->visit(printer);
        _os << " ) ";
    } else {
        _context.failed = true;
        _os << " false ";
    }
}

void ToTAPAALQueryVisitor::_accept(const NotEqualCondition* condition) {
    _os << " !( ";
    condition->visit(*this);
    _os << " ) ";
}

void ToTAPAALQueryVisitor::_accept(const NotCondition* condition) {
    _os << " !( ";
    condition->getCond()->visit(*this);
    _os << " ) ";
}

void ToTAPAALQueryVisitor::_accept(const BooleanCondition* condition) {
    if (condition->value)
        _os << "true";
    else
        _os << "false";
}

void ToTAPAALQueryVisitor::_accept(const DeadlockCondition* condition) {
    _os << "deadlock";
}

void ToTAPAALQueryVisitor::_accept(const UnfoldedUpperBoundsCondition* condition) {
    _os << "bounds (";
    for(size_t i = 0; i < condition->places().size(); ++i)
    {
        if(i != 0) _os << ", ";
        _os << condition->places()[i]._name;
    }
    _os << ")";
}

void ToTAPAALQueryVisitor::_accept(const ShallowCondition *condition) {
    condition->getCompiled()->visit(*this);
}