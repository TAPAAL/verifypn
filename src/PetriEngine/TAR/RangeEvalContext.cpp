/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   RangeEvalContext.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on April 11, 2020, 1:36 PM
 */

#include "PetriEngine/TAR/RangeEvalContext.h"


namespace PetriEngine
{

    RangeEvalContext::RangeEvalContext(const prvector_t& vector, const PetriNet& net)
    : _ranges(vector), _net(net), _in_use(_net.numberOfPlaces())
    {

    }

    void RangeEvalContext::_accept(const NotCondition* element) { assert(false); }
    void RangeEvalContext::_accept(const AndCondition* element) 
    {
        std::vector<bool> in_use(_net.numberOfPlaces());
        _in_use.swap(in_use);
        for(auto& c : *element)
        {
            c->visit(*this);
            if(!_bool_result)
            {
                for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                    _in_use[p] = (in_use[p] | _in_use[p]);
                return; // leave false on TOS (_bool_result)
            }
        }
        _in_use.swap(in_use);
        _bool_result = true; // true on TOS
    }
    
    void RangeEvalContext::_accept(const OrCondition* element)
    {
        std::vector<bool> in_use(_net.numberOfPlaces());
        _in_use.swap(in_use);
        for(auto& c : *element)
        {
            c->visit(*this);
            if(_bool_result)
            {
                _in_use.swap(in_use);
                return; // leave true on TOS
            }
        }
        _bool_result = false;
        for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
            _in_use[p] = (in_use[p] | _in_use[p]);
    }

    void RangeEvalContext::_accept(const IdentifierExpr* element)
    {
        assert(false);
    }
    
    void RangeEvalContext::_accept(const LessThanCondition* element){ assert(false); }
    void RangeEvalContext::_accept(const LessThanOrEqualCondition* element){ assert(false); }
    void RangeEvalContext::_accept(const GreaterThanCondition* element){ assert(false); }
    void RangeEvalContext::_accept(const GreaterThanOrEqualCondition* element){ assert(false); }
    void RangeEvalContext::_accept(const DeadlockCondition* element){ assert(false); }
    
    void RangeEvalContext::_accept(const CompareConjunction* element)
    {
        for(const CompareConjunction::cons_t& c : *element)
        {
            auto it = _ranges[c._place];
            if(it == nullptr && element->isNegated())
            {
                _bool_result = true; // disjunction, one element has viable satisfiability
                return;
            }
            if(element->isNegated())
            {
                if(it == nullptr || // unconstraint
                   it->_range._upper < c._lower || // -INF [... it->_upper] [c._lower, c._upper] + INF
                   c._upper < it->_range._lower    // -INF [c._lower, c._upper] [it->_lower ...] + INF                   
                   )
                {
                    _bool_result = true;
                    return;
                }
                _in_use[c._place] = true;
            }
            else
            {
                if(it == nullptr) continue; // we are looking for false instance
                if(!(it->_range._lower <= c._upper &&
                     c._lower <= it->_range._upper))
                {
                    _bool_result = false;
                    _in_use[c._place] = true;
                    return;
                }
            }
        }
        // negated = disj = "nothing is true at this point", otherwise conj = "everything is true"
        _bool_result = !element->isNegated(); 
    }
    
    
    void RangeEvalContext::_accept(const LiteralExpr* element)
    {
        _lower_result = _upper_result = element->value();
    }
    
    void RangeEvalContext::_accept(const UnfoldedIdentifierExpr* element)
    {
        auto it = _ranges[element->offset()];
        if(it == nullptr || it->_range.no_lower())
            _lower_result = 0;
        else
            _lower_result = it->_range._lower;

        if(it == nullptr || it->_range.no_upper())
            _upper_result = std::numeric_limits<decltype(_lower_result)>::max();
        else
            _upper_result = it->_range._lower;
        _in_use[element->offset()] = true;
    }
    
    void RangeEvalContext::_accept(const PlusExpr* element)
    {
        int64_t _low_sum = 0;
        int64_t _high_sum = 0;
        assert(false);
    }    
}

