/* Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
 *                     Rasmus Grønkjær Tollund <rasmusgtollund@gmail.com>
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

#include "PetriEngine/PQL/PrepareForReachability.h"

#define RETURN(x) { _return_value = x; return; }

namespace PetriEngine::PQL {

    Condition_ptr prepareForReachability(const Condition_ptr& condition) {
        return prepareForReachability(condition.get());
    }

    Condition_ptr prepareForReachability(const Condition* condition) {
        PrepareForReachabilityVisitor visitor;
        Visitor::visit(visitor, condition);
        return visitor.getReturnValue();
    }

    void PrepareForReachabilityVisitor::_accept(const ControlCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const EXCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const EGCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const EFCondition *condition) {
        condition->getCond()->setInvariant(_negated);
        RETURN(condition->getCond());
    }

    void PrepareForReachabilityVisitor::_accept(const AXCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const AGCondition *condition) {
        Condition_ptr cond = std::make_shared<NotCondition>(condition->getCond());
        cond->setInvariant(!_negated);
        RETURN(cond)
    }

    void PrepareForReachabilityVisitor::_accept(const AFCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const ACondition *condition) {
        auto g = std::dynamic_pointer_cast<GCondition>(condition->getCond());
        auto agcond = std::make_shared<AGCondition>((*g)[0]);
        RETURN(g ? subvisit(agcond.get(), _negated) : nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const ECondition *condition) {
        auto f = std::dynamic_pointer_cast<FCondition>(condition->getCond());
        auto efcond = std::make_shared<EFCondition>((*f)[0]);
        RETURN(f ? subvisit(efcond.get(), _negated) : nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const UntilCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const LogicalCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const CompareConjunction *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const CompareCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const NotCondition *condition) {
        RETURN(subvisit(condition->getCond().get(), !_negated))
    }

    void PrepareForReachabilityVisitor::_accept(const BooleanCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const DeadlockCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const UnfoldedUpperBoundsCondition *condition) {
        RETURN(nullptr)
    }

    void PrepareForReachabilityVisitor::_accept(const GCondition* condition) {
        // TODO implement
        assert(false);
        throw base_error("TODO implement");
    }

    void PrepareForReachabilityVisitor::_accept(const FCondition* condition) {
        // TODO implement
        assert(false);
        throw base_error("TODO implement");
    }

    void PrepareForReachabilityVisitor::_accept(const XCondition* condition) {
        // TODO implement
        assert(false); throw base_error("TODO implement");
    }

    void PrepareForReachabilityVisitor::_accept(const ShallowCondition* condition) {
        RETURN(subvisit(condition->getCompiled().get(), _negated))
    }
}