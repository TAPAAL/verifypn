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
                _accept(element);
            }

        protected:

            virtual void _accept(const NotCondition* element) = 0;
            virtual void _accept(const AndCondition* element) = 0;
            virtual void _accept(const OrCondition* element) = 0;
            virtual void _accept(const LessThanCondition* element) = 0;
            virtual void _accept(const LessThanOrEqualCondition* element) = 0;
            virtual void _accept(const GreaterThanCondition* element) = 0;
            virtual void _accept(const GreaterThanOrEqualCondition* element) = 0;
            virtual void _accept(const EqualCondition* element) = 0;
            virtual void _accept(const NotEqualCondition* element) = 0;

            virtual void _accept(const DeadlockCondition* element) = 0;
            virtual void _accept(const CompareConjunction* element) = 0;
            virtual void _accept(const UnfoldedUpperBoundsCondition* element) = 0;
            
            // Quantifiers, most uses of the visitor will not use the quantifiers - so we give a default implementation.
            // default behaviour is error
            virtual void _accept(const EFCondition*)
            {   assert(false); std::cerr << "No accept for EFCondition" << std::endl; exit(0);};
            virtual void _accept(const EGCondition*)
            {   assert(false); std::cerr << "No accept for EGCondition" << std::endl; exit(0);};
            virtual void _accept(const AGCondition*)
            {   assert(false); std::cerr << "No accept for AGCondition" << std::endl; exit(0);};
            virtual void _accept(const AFCondition*)
            {   assert(false); std::cerr << "No accept for AFCondition" << std::endl; exit(0);};
            virtual void _accept(const EXCondition*)
            {   assert(false); std::cerr << "No accept for EXCondition" << std::endl; exit(0);};            
            virtual void _accept(const AXCondition*)
            {   assert(false); std::cerr << "No accept for AXCondition" << std::endl; exit(0);};            
            virtual void _accept(const EUCondition*)
            {   assert(false); std::cerr << "No accept for EUCondition" << std::endl; exit(0);};
            virtual void _accept(const AUCondition*)
            {   assert(false); std::cerr << "No accept for AUCondition" << std::endl; exit(0);};            
            
            // shallow elements, neither of these should exist in a compiled expression
            virtual void _accept(const UnfoldedFireableCondition* element) 
            {   assert(false); std::cerr << "No accept for UnfoldedFireableCondition" << std::endl; exit(0);};
            virtual void _accept(const FireableCondition* element)
            {   assert(false); std::cerr << "No accept for FireableCondition" << std::endl; exit(0);};            
            virtual void _accept(const UpperBoundsCondition* element)
            {   assert(false); std::cerr << "No accept for UpperBoundsCondition" << std::endl; exit(0);};
            virtual void _accept(const LivenessCondition* element)
            {   assert(false); std::cerr << "No accept for LivenessCondition" << std::endl; exit(0);};
            virtual void _accept(const KSafeCondition* element)
            {   assert(false); std::cerr << "No accept for KSafeCondition" << std::endl; exit(0);};
            virtual void _accept(const QuasiLivenessCondition* element)
            {   assert(false); std::cerr << "No accept for QuasiLivenessCondition" << std::endl; exit(0);};
            virtual void _accept(const StableMarkingCondition* element)
            {   assert(false); std::cerr << "No accept for StableMarkingCondition" << std::endl; exit(0);};
            virtual void _accept(const BooleanCondition* element)
            {   assert(false); std::cerr << "No accept for BooleanCondition" << std::endl; exit(0);};
            
            // Expression
            virtual void _accept(const UnfoldedIdentifierExpr* element) = 0;
            virtual void _accept(const LiteralExpr* element) = 0;
            virtual void _accept(const PlusExpr* element) = 0;
            virtual void _accept(const MultiplyExpr* element) = 0;
            virtual void _accept(const MinusExpr* element) = 0;
            virtual void _accept(const SubtractExpr* element) = 0;
            
            // shallow expression, default to error
            virtual void _accept(const IdentifierExpr* element)
            {   assert(false); std::cerr << "No accept for IdentifierExpr" << std::endl; exit(0);};
        }; 
    }
}

#endif /* VISITOR_H */

