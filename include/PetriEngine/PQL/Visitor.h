/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   Visitor.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 11, 2020, 1:07 PM
 */

#ifndef VISITOR_H
#define VISITOR_H

#include "PetriEngine/PQL/Expressions.h"
#include <type_traits>

namespace PetriEngine
{
    namespace PQL
    {
        class Visitor {
        public:
            Visitor() {}

            template<typename T>
            void accept(T element)
            {
                typedef typename std::remove_pointer<T>::type T1;
                typedef typename std::remove_cv<T1>::type T2;
                _accept(element);
            }

        protected:
            virtual void _accept(const Condition* element) final {
                assert(false);
                std::cerr << "NOT IMPLEMENTED VISITOR" << std::endl;
                exit(-1);
            }
            virtual void _accept(const Expr* element) final {
                assert(false);
                std::cerr << "NOT IMPLEMENTED VISITOR" << std::endl;
                exit(-1);
            }
            virtual void _accept(const NotCondition* element) = 0;
            virtual void _accept(const PetriEngine::PQL::AndCondition* element) = 0;
            virtual void _accept(const OrCondition* element) = 0;
            virtual void _accept(const LessThanCondition* element) = 0;
            virtual void _accept(const LessThanOrEqualCondition* element) = 0;
            virtual void _accept(const GreaterThanCondition* element) = 0;
            virtual void _accept(const GreaterThanOrEqualCondition* element) = 0;
            virtual void _accept(const IdentifierExpr* element) = 0;
            virtual void _accept(const LiteralExpr* element) = 0;
            virtual void _accept(const UnfoldedIdentifierExpr* element) = 0;
            virtual void _accept(const PlusExpr* element) = 0;
            virtual void _accept(const DeadlockCondition* element) = 0;
            virtual void _accept(const CompareConjunction* element) = 0;
            virtual void _accept(const UnfoldedUpperBoundsCondition* element) = 0;
        }; 
    }
}

#endif /* VISITOR_H */

