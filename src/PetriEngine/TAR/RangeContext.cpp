/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   RangeContext.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on March 31, 2020, 5:01 PM
 */

#include "PetriEngine/TAR/RangeContext.h"
#include "PetriEngine/PQL/Expressions.h"
namespace PetriEngine {
    using namespace PQL;
   
    RangeContext::RangeContext(prvector_t& vector, MarkVal* base, const PetriNet& net, const uint64_t* uses, MarkVal* marking, const std::vector<bool>& dirty)
    : _ranges(vector), _base(base), _net(net), _uses(uses), _marking(marking), _dirty(dirty)
    {
    }
    
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
   
    void RangeContext::_accept(const NotCondition* element)
    {
        assert(false);
        std::cerr << "UNSUPPORTED QUERY TYPE FOR TAR" << std::endl;
        exit(-1);
    }

   
    void RangeContext::_accept(const PetriEngine::PQL::AndCondition* element)
    {
        if (element->isSatisfied()) return;
        EvaluationContext ctx(_marking, &_net);
        prvector_t vect = _ranges;
        prvector_t res;
        size_t priority = std::numeric_limits<size_t>::max();
        size_t sum = 0;
        bool dirty = false;
        assert(!_is_dirty);
        for (auto& e : *element) {
            if (e->evalAndSet(ctx) == Condition::RFALSE) {
                _ranges = vect;
                e->visit(*this);
                dirty |= _is_dirty;
                if(_is_dirty)
                {
                    _is_dirty = false;
                    continue;
                }
                if (_ranges._ranges.size() < priority) {
                    res = _ranges;
                    priority = _ranges._ranges.size();
                    sum = 0;
                    for (auto& e : _ranges._ranges)
                    {
                        sum += _uses[e._place];
                        assert(!_dirty[e._place]);
                    }
                }
                else if (_ranges._ranges.size() == priority) {
                    size_t lsum = 0;
                    for (auto& e : _ranges._ranges)
                    {
                        lsum += _uses[e._place];
                        assert(!_dirty[e._place]);
                    }
                    if (lsum > sum) {
                        res = _ranges;
                        priority = _ranges._ranges.size();
                        sum = lsum;
                    }
                }
            }
        }
        if(priority != std::numeric_limits<size_t>::max())
        {
            _ranges = res;
        }
        else
        {
            assert(dirty);
            _is_dirty = true;
        }
    }
    
    void RangeContext::_accept(const OrCondition* element)
    {
        // if(element->isSatisfied()) return;
        for (auto& e : *element) {
            //assert(!e->isSatisfied());
            e->visit(*this);
            if(_is_dirty)
                return;
        }
    }
    
    void RangeContext::_accept(const NotEqualCondition* element)    
    {
        _is_dirty = true; // TODO improve        
    }
    
    void RangeContext::_accept(const EqualCondition* element)
    {
        _is_dirty = true; // TODO improve
    }
     
    void RangeContext::_accept(const LessThanCondition* element)
    {
        handle_compare((*element)[0], (*element)[1], true);
    }
   
    void RangeContext::_accept(const LessThanOrEqualCondition* element)
    {
        handle_compare((*element)[0], (*element)[1], false);
    }

    void RangeContext::_accept(const GreaterThanCondition* element)
    {
        handle_compare((*element)[1], (*element)[0], true);
    }
    
    void RangeContext::_accept(const GreaterThanOrEqualCondition* element)
    {
        handle_compare((*element)[1], (*element)[0], false);
    }
   
    void RangeContext::_accept(const LiteralExpr* element)
    {
    }

    
    void RangeContext::_accept(const UnfoldedIdentifierExpr* element)
    {
        if(_dirty[element->offset()])
        {
            _is_dirty = true;
            return;
        }
        auto& pr = _ranges.find_or_add(element->offset());
        if (_lt)
            pr._range._upper = std::min<uint32_t>(pr._range._upper, _limit);
        else
            pr._range._lower = std::max<uint32_t>(pr._range._lower, _limit);
        assert(pr._range._lower <= _base[element->offset()]);
        assert(pr._range._upper >= _base[element->offset()]);
    }
    
    void RangeContext::_accept(const PlusExpr* element)
    {
        //auto fdf = std::abs(_limit - element->getEval())/
        //(element->places().size() + element->expressions().size());
        for (auto& p : element->places()) {
            if(_dirty[p.first])
            {
                _is_dirty = true;
                return;
            }
            auto& pr = _ranges.find_or_add(p.first);
            if (_lt)
                pr._range._upper = std::min(_base[p.first], pr._range._upper);
            else
                pr._range._lower = std::max(_base[p.first], pr._range._lower);
            assert(pr._range._lower <= _base[p.first]);
            assert(pr._range._upper >= _base[p.first]);
        }
        for (auto& e : element->expressions()) {
            _limit = e->getEval();
            e->visit(*this);
            if(_is_dirty) return;
        }
    }

    void RangeContext::_accept(const MultiplyExpr*)
    {
        _is_dirty = true; // TODO improve
    }

    void RangeContext::_accept(const MinusExpr*)
    {
        _is_dirty = true; // TODO improve
    }
    
    void RangeContext::_accept(const SubtractExpr*)
    {
        _is_dirty = true; // TODO improve
    }
    
    
    void RangeContext::_accept(const DeadlockCondition* element)
    {
        assert(!element->isSatisfied());
        uint64_t priority = 0;
        size_t cand = 0;
        bool dirty = false;
        for (size_t t = 0; t < _net.numberOfTransitions(); ++t) {
            auto pre = _net.preset(t);
            bool ok = true;
            for (; pre.first != pre.second; ++pre.first) {
                assert(!pre.first->inhibitor);
                if (_base[pre.first->place] < pre.first->tokens) {
                    ok = false;
                } 
                if(_dirty[pre.first->place])
                {
                    dirty = true;
                    ok = false;
                }
            }
            if (!ok) continue;
            pre = _net.preset(t);
            uint64_t sum = 0;
            for (; pre.first != pre.second; ++pre.first) {
                sum += _uses[pre.first->place];
                assert(!_dirty[pre.first->place]);
            }
            if (sum > priority) {
                priority = sum;
                cand = t;
            }
        }

        if (priority != 0) {
            auto pre = _net.preset(cand);
            for (; pre.first != pre.second; ++pre.first) {
                auto& pr = _ranges.find_or_add(pre.first->place);
                pr._range._lower = std::max(pre.first->tokens, pr._range._lower);
                assert(pr._range._lower <= _base[pre.first->place]);
                assert(pr._range._upper >= _base[pre.first->place]);
            }
            return;
        }
        if(dirty)
        {
            _is_dirty = true;
            return;
        }

        assert(false);
    }
    
    void RangeContext::_accept(const CompareConjunction* element)
    {
        assert(!element->isSatisfied());
        bool disjunction = element->isNegated();
        placerange_t pr;
        uint64_t priority = 0;
        for (auto& c : element->constraints()) {
            if (!disjunction) {
                if (c._lower > _base[c._place]) {
                    auto& added = _ranges.find_or_add(c._place);
                    if (added._range._upper <= c._lower - 1)
                        return;
                    if (!_dirty[c._place] && priority < _uses[c._place]) {
                        pr = placerange_t();
                        pr._place = c._place;
                        pr._range._upper = c._lower - 1;
                        priority = _uses[c._place];
                        assert(pr._range._lower <= _base[c._place]);
                        assert(pr._range._upper >= _base[c._place]);
                    }
                }
                else if (c._upper < _base[c._place]) {
                    auto& added = _ranges.find_or_add(c._place);
                    if (added._range._lower >= c._upper + 1)
                        return;
                    if (!_dirty[c._place] && priority < _uses[c._place]) {
                        pr = placerange_t();
                        priority = _uses[c._place];
                        pr._place = c._place;
                        pr._range._lower = c._upper + 1;
                        pr._place = c._place;
                        assert(pr._range._lower <= _base[c._place]);
                        assert(pr._range._upper >= _base[c._place]);
                    }
                }
            }
            else {
                auto& added = _ranges.find_or_add(c._place);
                added._range._lower = std::max(c._lower, pr._range._lower);
                added._range._upper = std::min(c._upper, pr._range._upper);
                assert(added._range._lower <= _base[c._place]);
                assert(added._range._upper >= _base[c._place]);
                if(_dirty[c._place])
                {
                    _is_dirty = true;
                    return;
                }
            }
        }
        if (!disjunction) {
            if(priority == 0)
            {
                _is_dirty = true;
                return;
            }
            assert(priority > 0);
            _ranges.find_or_add(pr._place) = pr;
            _ranges.compact();
        }
    }
    
    void RangeContext::_accept(const UnfoldedUpperBoundsCondition* element)
    {
        for(auto& pb : element->places())
        {
            _ranges.find_or_add(pb._place)._range._upper = std::min<double>(pb._max, _marking[pb._place]);
        }
    }
}
