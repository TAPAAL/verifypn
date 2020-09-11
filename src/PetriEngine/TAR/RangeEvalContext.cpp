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

    RangeEvalContext::RangeEvalContext(const prvector_t& vector, const PetriNet& net, const uint64_t* use_count)
    : _ranges(vector), _net(net), _use_count(use_count)
    {

    }

    void RangeEvalContext::_accept(const NotCondition* element) { assert(false); }
    void RangeEvalContext::_accept(const AndCondition* element) 
    {
        prvector_t sufficient;
        sufficient = _sufficient;
        auto best = _sufficient;
        bool found = false;
        for(auto& c : *element)
        {
            c->visit(*this);
            if(!_bool_result)
            {
                c->visit(*this);
                if(!found || _sufficient._ranges.size() < best._ranges.size())
                    best = _sufficient;
                _sufficient = sufficient;
                found = true;
            }
        }
        if(found)
            _sufficient = best;
        else
            _sufficient = sufficient;
        _bool_result = !found; // true on TOS
    }
    
    void RangeEvalContext::_accept(const OrCondition* element)
    {
        prvector_t sufficient = _sufficient;
        for(auto& c : *element)
        {
            c->visit(*this);
            if(_bool_result)
            {
                _sufficient = sufficient;
                return; // leave true on TOS
            }
        }
        _bool_result = false;
    }

    void RangeEvalContext::handle_compare(const Expr_ptr& left, const Expr_ptr& right, bool strict)
    {
        _places.clear();
        // left < right (if strict is set, otherwise <= )
        if(right->placeFree())
        {
            //_limit = vr + (strict ? 0 : 1);
            //_lt = false;
            right->visit(*this);
            auto cnst = _literal;
            _literal = 0;
            left->visit(*this);
            cnst -= _literal;
            assert(cnst >= 0);
            // p < const
            // p <= const
            for(auto p : _places)
            {
                auto val = cnst + (strict ? 0 : 1);
                if(val > _sufficient.upper(p))
                {
                    _bool_result = true;
                    return;
                }
                _sufficient &= placerange_t(p, val, range_t::max());
            }
        } 
        else if(left->placeFree())
        {
            //_limit = vl - (strict ? 0 : 1);
            //_lt = true;
            left->visit(*this);
            auto cnst = _literal;
            _literal = 0;
            cnst -= _literal;
            right->visit(*this);
            // const < p
            // const <= p
            for(auto p : _places)
            {
                auto val = cnst + (strict ? 1 : 0);
                if(val < _sufficient.lower(p))
                {
                    _bool_result = true;
                    return;
                }
                _sufficient &= placerange_t(p, range_t::min(), val);
            }
        }
        else
        {
            _bool_result = true;
            return;
/*            for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                _sufficient &= placerange_t(p, 0, 0);*/
        }
    }
    
    void RangeEvalContext::_accept(const EqualCondition* element)
    {
        _bool_result = true; // TODO handle better
    }
    
    void RangeEvalContext::_accept(const NotEqualCondition* element)
    {
        _bool_result = true; // TODO handle better
    }

    void RangeEvalContext::_accept(const LessThanCondition* element)
    {
        handle_compare((*element)[0], (*element)[1], true);
    }
   
    void RangeEvalContext::_accept(const LessThanOrEqualCondition* element)
    {
        handle_compare((*element)[0], (*element)[1], false);
    }

    void RangeEvalContext::_accept(const GreaterThanCondition* element)
    {
        handle_compare((*element)[1], (*element)[0], true);
    }
    
    void RangeEvalContext::_accept(const GreaterThanOrEqualCondition* element)
    {
        handle_compare((*element)[1], (*element)[0], false);
    }
    
    void RangeEvalContext::_accept(const DeadlockCondition* element)
    {
        _bool_result = true;
    }
    
    void RangeEvalContext::_accept(const CompareConjunction* element)
    {
        placerange_t tmp;
        bool found = false;
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
                _sufficient &= placerange_t(c._place, c._lower, c._upper);
            }
            else
            {
                if(it == nullptr) continue; // we are looking for false instance
                if(!(it->_range._lower <= c._upper &&
                     c._lower <= it->_range._upper))
                {
                    _bool_result = false;
                    if(it->_range._upper < c._lower)
                    {
                        if(_sufficient.upper(c._place) < c._lower)
                        {
                            _bool_result = false;
                            return;
                        }
                        if(!found || (_use_count[c._place] > _use_count[tmp._place] && !tmp._range.no_upper()))
                            tmp = placerange_t(c._place, 0, c._lower-1);
                        found = true;
                    }
                    else
                    {
                        if(_sufficient.lower(c._place) > c._lower)
                        {
                            _bool_result = false;
                            return;
                        }
                        if(!found || _use_count[c._place] > _use_count[tmp._place] || !tmp._range.no_upper())
                            tmp = placerange_t(c._place, c._upper+1, range_t::max());
                        found = true;
                    }
                }
            }
        }
        // negated = disj = "nothing is true at this point", otherwise conj = "everything is true"
        if(!element->isNegated())
        {
            if(found)
            {
                _sufficient &= tmp;
                //tmp.print(std::cerr) << std::endl;
            }
            _bool_result = !found;
            return;
        }
        _bool_result = !element->isNegated(); 
    }
    
    
    void RangeEvalContext::_accept(const LiteralExpr* element)
    {
        _literal = element->value();
    }
    
    void RangeEvalContext::_accept(const UnfoldedIdentifierExpr* element)
    {
        _places.emplace_back(element->offset());
    }
    
    void RangeEvalContext::_accept(const PlusExpr* element)
    {
        for(auto& e : element->expressions())
            e->visit(*this);
        _literal += element->constant();
        for(auto p :  element->places())
            _places.push_back(p.first);
    }
    
    void RangeEvalContext::_accept(const UnfoldedUpperBoundsCondition* element)
    {
        _bool_result = true;
    }
    
    void RangeEvalContext::_accept(const MultiplyExpr*)
    {
        _bool_result = true;        
    }

    void RangeEvalContext::_accept(const MinusExpr*)
    {
        _bool_result = true;        
    }
    
    void RangeEvalContext::_accept(const SubtractExpr*)
    {
        _bool_result = true;        
    }
}

