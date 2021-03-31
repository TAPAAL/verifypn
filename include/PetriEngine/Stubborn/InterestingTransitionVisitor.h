/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
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

#ifndef VERIFYPN_INTERESTINGTRANSITIONVISITOR_H
#define VERIFYPN_INTERESTINGTRANSITIONVISITOR_H

#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/Stubborn/StubbornSet.h"

namespace PetriEngine {
    class InterestingTransitionVisitor : public PQL::Visitor {
    public:
        InterestingTransitionVisitor(PetriEngine::StubbornSet &stubbornSet, bool closure)
                : _stubborn(stubbornSet), closure(closure), incr(stubbornSet, closure), decr(stubbornSet, closure)
        {
            incr.decr = &decr;
            decr.incr = &incr;
        }

        void negate() { negated = !negated; }

    private:
        PetriEngine::StubbornSet &_stubborn;

        bool closure;

    protected:
        void _accept(const PQL::NotCondition *element) override;

        void _accept(const PQL::AndCondition *element) override;

        void _accept(const PQL::OrCondition *element) override;

        void _accept(const PQL::LessThanCondition *element) override;

        void _accept(const PQL::LessThanOrEqualCondition *element) override;

        void _accept(const PQL::EqualCondition *element) override;

        void _accept(const PQL::NotEqualCondition *element) override;

        void _accept(const PQL::DeadlockCondition *element) override;

        void _accept(const PQL::CompareConjunction *element) override;

        void _accept(const PQL::UnfoldedUpperBoundsCondition *element) override;

        void _accept(const PQL::UnfoldedIdentifierExpr *element) override
        {
            assert(false);
            std::cerr << "No accept for UnfoldedIdentifierExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::LiteralExpr *element) override
        {
            assert(false);
            std::cerr << "No accept for LiteralExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::PlusExpr *element) override
        {
            assert(false);
            std::cerr << "No accept for PlusExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::MultiplyExpr *element) override
        {
            assert(false);
            std::cerr << "No accept for MultiplyExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::MinusExpr *element) override
        {
            assert(false);
            std::cerr << "No accept for MinusExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::SubtractExpr *element) override
        {
            assert(false);
            std::cerr << "No accept for SubtractExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::SimpleQuantifierCondition *element);

        void _accept(const PQL::UntilCondition *element) override;

        void _accept(const PQL::GCondition *condition) override;

        void _accept(const PQL::FCondition *condition) override;

        void _accept(const PQL::BooleanCondition *element) override;

        bool negated = false;
    private:

        /*
         * Mutually recursive visitors for incrementing/decrementing of places.
         */

        class DecrVisitor;

        class IncrVisitor : public PQL::ExpressionVisitor {
        public:
            IncrVisitor(StubbornSet &stubbornSet, bool closure)
                    : _stubborn(stubbornSet), closure(closure) {}

            DecrVisitor *decr;
        private:
            StubbornSet &_stubborn;
            bool closure;

            void _accept(const PQL::IdentifierExpr *element) override
            {
                element->compiled()->visit(*this);
            }

            void _accept(const PQL::UnfoldedIdentifierExpr *element) override;

            void _accept(const PQL::LiteralExpr *element) override;

            void _accept(const PQL::PlusExpr *element) override;

            void _accept(const PQL::MultiplyExpr *element) override;

            void _accept(const PQL::MinusExpr *element) override;

            void _accept(const PQL::SubtractExpr *element) override;
        };

        class DecrVisitor : public PQL::ExpressionVisitor {
        public:
            DecrVisitor(StubbornSet &stubbornSet, bool closure)
                    : _stubborn(stubbornSet), closure(closure) {}

            IncrVisitor *incr;

        private:
            StubbornSet &_stubborn;
            bool closure;

            void _accept(const PQL::IdentifierExpr *element) override
            {
                element->compiled()->visit(*this);
            }

            void _accept(const PQL::UnfoldedIdentifierExpr *element) override;

            void _accept(const PQL::LiteralExpr *element) override;

            void _accept(const PQL::PlusExpr *element) override;

            void _accept(const PQL::MultiplyExpr *element) override;

            void _accept(const PQL::MinusExpr *element) override;

            void _accept(const PQL::SubtractExpr *element) override;
        };

        IncrVisitor incr;
        DecrVisitor decr;
    };

    class InterestingLTLTransitionVisitor : public InterestingTransitionVisitor {
    public:
        explicit InterestingLTLTransitionVisitor(StubbornSet &stubbornSet, bool closure) : InterestingTransitionVisitor(stubbornSet, closure) {}

    protected:
        void _accept(const PQL::LessThanCondition *element) override;

        void _accept(const PQL::LessThanOrEqualCondition *element) override;

        void _accept(const PQL::EqualCondition *element) override;

        void _accept(const PQL::NotEqualCondition *element) override;

        void _accept(const PQL::CompareConjunction *element) override;

        template<typename Condition>
        void negate_if_satisfied(const Condition *element);
    };

}


#endif //VERIFYPN_INTERESTINGTRANSITIONVISITOR_H
