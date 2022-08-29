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

#ifndef VERIFYPN_PREPAREFORREACHABILITY_H
#define VERIFYPN_PREPAREFORREACHABILITY_H

#include "Visitor.h"

namespace PetriEngine { namespace PQL {

    Condition_ptr prepareForReachability(const Condition* condition);
    Condition_ptr prepareForReachability(const Condition_ptr& condition);

    class PrepareForReachabilityVisitor : public Visitor {

    public:
        Condition_ptr getReturnValue() {
            return _return_value;
        }

    private:
        Condition_ptr _return_value = nullptr;
        bool _negated = false;

        Condition_ptr subvisit(const Condition* condition, bool negated) {
            bool prev_negated = _negated;
            _negated = negated;

            Visitor::visit(this, condition);

            _negated = prev_negated;
            return _return_value;
        }

        void _accept(const ControlCondition *condition) override;

        void _accept(const EGCondition *condition) override;

        void _accept(const EXCondition *condition) override;

        void _accept(const EFCondition *condition) override;

        void _accept(const AXCondition *condition) override;

        void _accept(const AGCondition *condition) override;

        void _accept(const AFCondition *condition) override;

        void _accept(const ACondition *condition) override;

        void _accept(const ECondition *condition) override;

        void _accept(const UntilCondition *condition) override;

        void _accept(const LogicalCondition *condition) override;

        void _accept(const CompareConjunction *condition) override;

        void _accept(const CompareCondition *condition) override;

        void _accept(const NotCondition *condition) override;

        void _accept(const BooleanCondition *condition) override;

        void _accept(const DeadlockCondition *condition) override;

        void _accept(const UnfoldedUpperBoundsCondition *condition) override;

        void _accept(const GCondition *condition) override;

        void _accept(const FCondition *condition) override;

        void _accept(const XCondition *condition) override;

        void _accept(const ShallowCondition *condition) override;
    };
} }

#endif //VERIFYPN_PREPAREFORREACHABILITY_H
