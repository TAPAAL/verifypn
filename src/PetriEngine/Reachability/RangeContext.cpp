/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   RangeContext.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on March 31, 2020, 5:01 PM
 */

#include "PetriEngine/Reachability/RangeContext.h"
#include "PetriEngine/PQL/Expressions.h"
namespace PetriEngine {
    using namespace PQL;
    
    void RangeContext::handle_compare(const Expr_ptr& left, const Expr_ptr& right, bool strict)
    {
        auto vl = left->getEval();
        auto vr = right->getEval();
        if(right->placeFree())
        {
            _limit = vr + (strict ? 0 : 1);
            _lt = false;
            left->visit(*this);
        } 
        else if(left->placeFree())
        {
            _limit = vl - (strict ? 0 : 1);
            _lt = true;
            right->visit(*this);            
        }
        else
        {
            _lt = false;
            _limit = vr + (strict ? 0 : 1);
            left->visit(*this);
            _lt = true;
            _limit = vr;
            right->visit(*this);
        }
    }
    
}
