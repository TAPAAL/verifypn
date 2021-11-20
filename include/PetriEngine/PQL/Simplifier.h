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
#ifndef VERIFYPN_SIMPLIFIER_H
#define VERIFYPN_SIMPLIFIER_H

#include "Visitor.h"

namespace PetriEngine::PQL {

    Retval simplify(std::shared_ptr<Condition> element, SimplificationContext& context);

    class Simplifier : public Visitor {

    public:
        explicit Simplifier(SimplificationContext& context) :
            context(context) {}

        Retval return_value;

    protected:
        SimplificationContext& context;

        Retval simplifyOr(const LogicalCondition* element);
        Retval simplifyAnd(const LogicalCondition *element);

        Retval simplifyAG(Retval &r);
        Retval simplifyAF(Retval &r);
        Retval simplifyAX(Retval &r);

        Retval simplifyEG(Retval &r);
        Retval simplifyEF(Retval &r);
        Retval simplifyEX(Retval &r);

        template <typename Quantifier>
        Retval simplifySimpleQuant(Retval &r);

        void _accept(const NotCondition *element) override;

        void _accept(const AndCondition *element) override;

        void _accept(const OrCondition *element) override;

        void _accept(const LessThanCondition *element) override;

        void _accept(const LessThanOrEqualCondition *element) override;

        void _accept(const EqualCondition *element) override;

        void _accept(const NotEqualCondition *element) override;

        void _accept(const DeadlockCondition *element) override;

        void _accept(const CompareConjunction *element) override;

        void _accept(const UnfoldedUpperBoundsCondition *element) override;

        void _accept(const EFCondition *condition) override;

        void _accept(const EGCondition *condition) override;

        void _accept(const AGCondition *condition) override;

        void _accept(const AFCondition *condition) override;

        void _accept(const EXCondition *condition) override;

        void _accept(const AXCondition *condition) override;

        void _accept(const EUCondition *condition) override;

        void _accept(const AUCondition *condition) override;

        void _accept(const ACondition *condition) override;

        void _accept(const ECondition *condition) override;

        void _accept(const GCondition *condition) override;

        void _accept(const FCondition *condition) override;

        void _accept(const XCondition *condition) override;

        void _accept(const UntilCondition *condition) override;

        void _accept(const BooleanCondition *element) override;
    };
}
#endif //VERIFYPN_SIMPLIFIER_H
