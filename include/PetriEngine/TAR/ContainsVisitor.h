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

        virtual void _accept(const NotCondition* element) override 
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
            (*element)[0]->visit(*this);
        }
        virtual void _accept(const PetriEngine::PQL::AndCondition* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
            for(auto& e : *element)
            {
                e->visit(*this);
                if(_value) return;
            }
        }

        virtual void _accept(const OrCondition* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
            for(auto& e : *element)
            {
                e->visit(*this);
                if(_value) return;
            }
        }

        virtual void _accept(const LessThanCondition* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
            (*element)[0]->visit(*this);
            if(_value) return;
            (*element)[1]->visit(*this);
        }
        
        virtual void _accept(const LessThanOrEqualCondition* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
            (*element)[0]->visit(*this);
            if(_value) return;
            (*element)[1]->visit(*this);
        }
        
        virtual void _accept(const GreaterThanCondition* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
            (*element)[0]->visit(*this);
            if(_value) return;
            (*element)[1]->visit(*this);
        }
        
        virtual void _accept(const GreaterThanOrEqualCondition* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
            (*element)[0]->visit(*this);
            if(_value) return;
            (*element)[1]->visit(*this);
        }
        
        virtual void _accept(const NotEqualCondition* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
            (*element)[0]->visit(*this);
            if(_value) return;
            (*element)[1]->visit(*this);
        }

        virtual void _accept(const EqualCondition* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
            (*element)[0]->visit(*this);
            if(_value) return;
            (*element)[1]->visit(*this);
        }        
        
        virtual void _accept(const IdentifierExpr* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
        }        

        virtual void _accept(const LiteralExpr* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
        }
        
        virtual void _accept(const UnfoldedIdentifierExpr* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
        }
        
        virtual void _accept(const PlusExpr* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
            for(auto& e : element->expressions())
            {
                e->visit(*this);
                if(_value) return;
            }
        }        
        
        virtual void _accept(const DeadlockCondition* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
        }        
        
        virtual void _accept(const CompareConjunction* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
        }        
        
        virtual void _accept(const UnfoldedUpperBoundsCondition* element) override
        { 
            if constexpr (std::is_same<const T*,decltype(element)>::value) { _value = true; return; }
        }        

    };
}


#endif /* CONTAINSVISITOR_H */

