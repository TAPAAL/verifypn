/*
 * File:   InterestingTransitionVisitor.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 03/02/2021
 */

#ifndef VERIFYPN_INTERESTINGTRANSITIONVISITOR_H
#define VERIFYPN_INTERESTINGTRANSITIONVISITOR_H

#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/Stubborn/StubbornSet.h"

namespace PetriEngine {
    class InterestingTransitionVisitor : public PQL::Visitor {
    public:
        InterestingTransitionVisitor(PetriEngine::StubbornSet &stubbornSet)
                : _stubborn(stubbornSet), incr(stubbornSet), decr(stubbornSet) {
            incr.decr = &decr;
            decr.incr = &incr;
        }

    private:
        PetriEngine::StubbornSet &_stubborn;
        bool negated = false;

        void negate() { negated = !negated; }

    protected:
        void _accept(const PQL::NotCondition *element) override;

        void _accept(const PQL::AndCondition *element) override;

        void _accept(const PQL::OrCondition *element) override;

        void _accept(const PQL::LessThanCondition *element) override;

        void _accept(const PQL::LessThanOrEqualCondition *element) override;

        void _accept(const PQL::GreaterThanCondition *element) override;

        void _accept(const PQL::GreaterThanOrEqualCondition *element) override;

        void _accept(const PQL::EqualCondition *element) override;

        void _accept(const PQL::NotEqualCondition *element) override;

        void _accept(const PQL::DeadlockCondition *element) override;

        void _accept(const PQL::CompareConjunction *element) override;

        void _accept(const PQL::UnfoldedUpperBoundsCondition *element) override;

        void _accept(const PQL::UnfoldedIdentifierExpr *element) override {
            assert(false);
            std::cerr << "No accept for UnfoldedIdentifierExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::LiteralExpr *element) override {
            assert(false);
            std::cerr << "No accept for LiteralExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::PlusExpr *element) override {
            assert(false);
            std::cerr << "No accept for PlusExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::MultiplyExpr *element) override {
            assert(false);
            std::cerr << "No accept for MultiplyExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::MinusExpr *element) override {
            assert(false);
            std::cerr << "No accept for MinusExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::SubtractExpr *element) override {
            assert(false);
            std::cerr << "No accept for SubtractExpr" << std::endl;
            exit(0);
        };

        void _accept(const PQL::SimpleQuantifierCondition *element);

        void _accept(const PQL::UntilCondition *element) override;

        void _accept(const PQL::BooleanCondition *element) override;

    private:

        /*
         * Mutually recursive visitors for incrementing/decrementing of places.
         */

        class DecrVisitor;
        class IncrVisitor : public PQL::ExpressionVisitor {
        public:
            IncrVisitor(StubbornSet &stubbornSet)
                    : _stubborn(stubbornSet) {}

            DecrVisitor *decr;
        private:
            StubbornSet &_stubborn;

            void _accept(const PQL::IdentifierExpr *element) override {
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
            DecrVisitor(StubbornSet &stubbornSet)
                    : _stubborn(stubbornSet) {}

            IncrVisitor *incr;

        private:
            StubbornSet &_stubborn;

            void _accept(const PQL::IdentifierExpr *element) override {
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

}


#endif //VERIFYPN_INTERESTINGTRANSITIONVISITOR_H
