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

    void PrepareForReachabilityVisitor::_accept(const ACondition *condition) {
        if (auto g = std::dynamic_pointer_cast<GCondition>(condition->getCond())) {
            Condition_ptr cond = std::make_shared<NotCondition>(g->getCond());
            cond->setInvariant(!_negated);
            RETURN(cond)
        } else {
            RETURN(nullptr)
        }
    }

    void PrepareForReachabilityVisitor::_accept(const ECondition *condition) {
        if (auto f = std::dynamic_pointer_cast<FCondition>(condition->getCond())) {
            f->getCond()->setInvariant(_negated);
            RETURN(f->getCond());
        } else {
            RETURN(nullptr)
        }
    }

    void PrepareForReachabilityVisitor::_accept(const NotCondition *condition) {
        RETURN(subvisit(condition->getCond().get(), !_negated))
    }

    void PrepareForReachabilityVisitor::_accept(const ShallowCondition* condition) {
        RETURN(subvisit(condition->getCompiled().get(), _negated))
    }
}