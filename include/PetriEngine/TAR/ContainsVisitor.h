/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   ContainsVisitor.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 21, 2020, 10:44 PM
 */

#ifndef CONTAINSVISITOR_H
#define CONTAINSVISITOR_H

#include "PetriEngine/TAR/range.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Visitor.h"

#include <type_traits>

namespace PetriEngine
{
    using namespace Reachability;

    using namespace PQL;
    template<typename T>
    class ContainsVisitor : public Visitor {
    public:
        ContainsVisitor() {}
        bool does_contain() const { return _value; }
    private:
        bool _value = false;
    protected:

        template<typename K>
        bool found_type(K element)
        {
            if(std::is_same<const T*,K>::value)
                _value = true;
            return _value;
        }
        
        virtual void _accept(const NotCondition* element) override 
        { 
            if(found_type(element)) return;
            (*element)[0]->visit(*this);
        }
        virtual void _accept(const PetriEngine::PQL::AndCondition* element) override
        { 
            if(found_type(element)) return;
            for(auto& e : *element)
            {
                e->visit(*this);
                if(_value) return;
            }
        }

        virtual void _accept(const OrCondition* element) override
        { 
            if(found_type(element)) return;
            for(auto& e : *element)
            {
                e->visit(*this);
                if(_value) return;
            }
        }
        
        template<typename K>
        void handleCompare(const CompareCondition* element)
        {
            if(found_type(element)) return;
            (*element)[0]->visit(*this);
            if(_value) return;
            (*element)[1]->visit(*this);            
        }

        virtual void _accept(const LessThanCondition* element) override
        { 
            handleCompare<decltype(element)>(element);
        }
        
        virtual void _accept(const LessThanOrEqualCondition* element) override
        { 
            handleCompare<decltype(element)>(element);
        }
        
        virtual void _accept(const GreaterThanCondition* element) override
        { 
            handleCompare<decltype(element)>(element);
        }
        
        virtual void _accept(const GreaterThanOrEqualCondition* element) override
        { 
            handleCompare<decltype(element)>(element);
        }
        
        virtual void _accept(const NotEqualCondition* element) override
        { 
            handleCompare<decltype(element)>(element);
        }

        virtual void _accept(const EqualCondition* element) override
        { 
            handleCompare<decltype(element)>(element);
        }        
        
        virtual void _accept(const IdentifierExpr* element) override
        { 
            if(found_type(element)) return;
        }        

        virtual void _accept(const LiteralExpr* element) override
        { 
            if(found_type(element)) return;
        }
        
        virtual void _accept(const UnfoldedIdentifierExpr* element) override
        { 
            if(found_type(element)) return;
        }
        
        template<typename K>
        void handleNaryExpr(K element)
        {
            if(found_type(element)) return;
            for(auto& e : element->expressions())
            {
                e->visit(*this);
                if(_value) return;
            }            
        }
        
        virtual void _accept(const PlusExpr* element) override
        { 
            handleNaryExpr<decltype(element)>(element);
        }

        virtual void _accept(const MultiplyExpr* element) override
        { 
            handleNaryExpr<decltype(element)>(element);
        }
        
        virtual void _accept(const MinusExpr* element) override
        { 
            if(found_type(element)) return;
            (*element)[0]->visit(*this);
        }
        
        virtual void _accept(const SubtractExpr* element) override
        { 
            handleNaryExpr<decltype(element)>(element);
        }

        virtual void _accept(const DeadlockCondition* element) override
        { 
            if(found_type(element)) return;
        }        
        
        virtual void _accept(const CompareConjunction* element) override
        { 
            if(found_type(element)) return;
        }        
        
        virtual void _accept(const UnfoldedUpperBoundsCondition* element) override
        { 
            if(found_type(element)) return;
        }

        template<typename K>
        void handleSimpleQuantifierCondition(K element)
        {
            if(found_type(element)) return;
            (*element)[0]->visit(*this);
        }
        
        virtual void _accept(const EFCondition* el)
        {   handleSimpleQuantifierCondition(el); }
        virtual void _accept(const EGCondition* el)
        {   handleSimpleQuantifierCondition(el); }
        virtual void _accept(const AGCondition* el)
        {   handleSimpleQuantifierCondition(el); }
        virtual void _accept(const AFCondition* el)
        {   handleSimpleQuantifierCondition(el); }
        virtual void _accept(const EXCondition* el)
        {   handleSimpleQuantifierCondition(el); }
        virtual void _accept(const AXCondition* el)
        {   handleSimpleQuantifierCondition(el); }
        
        virtual void _accept(const EUCondition* el)
        {   
            if(found_type(el)) return;
            (*el)[0]->visit(*this);
            if(_value) return;
            (*el)[1]->visit(*this);
        }
        
        virtual void _accept(const AUCondition* el)
        {   
            if(found_type(el)) return;
            (*el)[0]->visit(*this);
            if(_value) return;
            (*el)[1]->visit(*this);
        }

        // shallow elements, neither of these should exist in a compiled expression
        virtual void _accept(const UnfoldedFireableCondition* element) 
        {   found_type(element); };
        virtual void _accept(const FireableCondition* element)
        {   found_type(element); };
        virtual void _accept(const UpperBoundsCondition* element)
        {   found_type(element); };
        virtual void _accept(const LivenessCondition* element)
        {   found_type(element); };
        virtual void _accept(const KSafeCondition* element)
        {   found_type(element); };
        virtual void _accept(const QuasiLivenessCondition* element)
        {   found_type(element); };
        virtual void _accept(const StableMarkingCondition* element)
        {   found_type(element); };
        virtual void _accept(const BooleanCondition* element)
        {   found_type(element); };
        
        
    };
}


#endif /* CONTAINSVISITOR_H */

