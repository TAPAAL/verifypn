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

#ifndef VERIFYPN_TOTAPAALQUERY_H
#define VERIFYPN_TOTAPAALQUERY_H

#include "PetriEngine/PQL/Visitor.h"

/** Export condition to TAPAAL query (add EF manually!) */
namespace PetriEngine::PQL {
    class ToTAPAALQueryVisitor : Visitor {
        ToTAPAALQueryVisitor(std::ostream& os, TAPAALConditionExportContext& context)
            : _os(os), _context(context) {}

    private:
        std::ostream& _os;
        TAPAALConditionExportContext& _context;

        void _accept(const SimpleQuantifierCondition *condition) override;

        void _accept(const UntilCondition *condition) override;

        void _accept(const LogicalCondition *condition) override;

        void _accept(const CompareConjunction *condition) override;

        void _accept(const CompareCondition *condition) override;

        void _accept(const NotEqualCondition *condition) override;

        void _accept(const NotCondition *condition) override;

        void _accept(const BooleanCondition *condition) override;

        void _accept(const DeadlockCondition *condition) override;

        void _accept(const UnfoldedUpperBoundsCondition *condition) override;

        void _accept(const ShallowCondition *condition) override;
    };
}

#endif //VERIFYPN_TOTAPAALQUERY_H
