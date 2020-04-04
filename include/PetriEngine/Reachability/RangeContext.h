/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   RangeContext.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on March 31, 2020, 5:01 PM
 */

#ifndef RANGECONTEXT_H
#define RANGECONTEXT_H

#include "PetriEngine/TAR/range.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PQL/Expressions.h"

#include <type_traits>

namespace PetriEngine
{
    using namespace Reachability;

    using namespace PQL;
    class RangeContext {
    public:
        RangeContext(prvector_t& vector, MarkVal* base, const PetriNet& net)
        : _ranges(vector), _base(base), _net(net) {}
        
        template<typename T>
        void accept(T element)
        {
            typedef typename std::remove_pointer<T>::type T1;
            typedef typename std::remove_cv<T1>::type T2;
            _accept<T2>(element);
        }
        
    private:
        template<typename T>
        void _accept(const T* element)
        {
            assert(false);
            std::cerr << "UNSUPPORTED QUERY TYPE FOR TAR" << std::endl;
            exit(-1);
        }
        
        void handle_compare(const Expr_ptr& left, const Expr_ptr& right, bool strict);
                
        prvector_t& _ranges;
        MarkVal* _base;
        const PetriNet& _net;
        int64_t _limit = 0;
        bool _lt = false;
    };

    // for whatever reason, specialization is not picked up unless inlined here?!
    template<>
    inline void RangeContext::_accept(const NotCondition* element)
    {
        assert(false);
        std::cerr << "UNSUPPORTED QUERY TYPE FOR TAR" << std::endl;
        exit(-1);
    }
    
    template<>
    inline void RangeContext::_accept(const PetriEngine::PQL::AndCondition* element)
    {
        if(element->isSatisfied()) return;
        for(auto& e : *element)
        {
            if(!e->isSatisfied())
            {
                e->visit(*this);
                return;
            }
        }
    }

    template<>
    inline void RangeContext::_accept(const OrCondition* element)
    {
        if(element->isSatisfied()) return;
        for(auto& e : *element)
        {
            assert(!e->isSatisfied());
            e->visit(*this);
        }
    }
    
    template<>
    inline void RangeContext::_accept(const LessThanCondition* element)
    {
        handle_compare((*element)[0], (*element)[1], true);
    }

    template<>
    inline void RangeContext::_accept(const LessThanOrEqualCondition* element)
    {
        handle_compare((*element)[0], (*element)[1], false);
    }
    
    template<>
    inline void RangeContext::_accept(const GreaterThanCondition* element)
    {
        handle_compare((*element)[1], (*element)[0], true);
    }

    template<>
    inline void RangeContext::_accept(const GreaterThanOrEqualCondition* element)
    {
        handle_compare((*element)[1], (*element)[0], false);
    }

    template<>
    inline void RangeContext::_accept(const IdentifierExpr* element)
    {
        assert(false);
    }

    template<>
    inline void RangeContext::_accept(const LiteralExpr* element)
    {}

    template<>
    inline void RangeContext::_accept(const UnfoldedIdentifierExpr* element)
    {
        auto& pr = _ranges.find_or_add(element->offset());
        if(_lt)
            pr._range._upper = _limit;
        else
            pr._range._lower = _limit;
        assert(pr._range._lower <= _base[element->offset()]);
        assert(pr._range._upper >= _base[element->offset()]);
    }
    
    template<>
    inline void RangeContext::_accept(const PlusExpr* element)
    {
        //auto fdf = std::abs(_limit - element->getEval())/
            (element->places().size() + element->expressions().size());
        for(auto& p : element->places())
        {
            auto& pr = _ranges.find_or_add(p.first);
            if(_lt)
                pr._range._upper = _base[p.first];
            else
                pr._range._lower = _base[p.first];
        }
        for(auto& e : element->expressions())
        {
            _limit = e->getEval();
            e->visit(*this);
        }
    }
    
    template<>
    inline void RangeContext::_accept(const DeadlockCondition* element)
    {
        assert(!element->isSatisfied());
        for(size_t t = 0; t < _net.numberOfTransitions(); ++t)
        {
            auto pre = _net.preset(t);
            bool ok = true;
            for(; pre.first != pre.second; ++pre.first)
            {
                assert(!pre.first->inhibitor);
                if(_base[pre.first->place] < pre.first->tokens)
                {
                    ok = false;
                }
            }
            if(!ok) continue;
            pre = _net.preset(t);
            for(; pre.first != pre.second; ++pre.first)
            {
                auto& pr = _ranges.find_or_add(pre.first->place);
                pr._range._lower = pre.first->tokens;
            }
            return;
        }
        assert(false);
    }
    
    template<>
    inline void RangeContext::_accept(const CompareConjunction* element)
    {
        assert(!element->isSatisfied());
        bool disjunction = element->isNegated();
        for(auto& c : element->constraints())
        {
            if(!disjunction)
            {
                if(c._lower > _base[c._place])
                {
                    _ranges.find_or_add(c._place)._range._upper = c._lower-1;
                    return;
                }
                else if(c._upper < _base[c._place])
                {
                    _ranges.find_or_add(c._place)._range._lower = c._upper+1;
                    return;
                }
            }
            else
            {
                _ranges.find_or_add(c._place)._range._lower = c._lower;
                _ranges.find_or_add(c._place)._range._upper = c._upper;
            }
        }
    }
}

#endif /* RANGECONTEXT_H */

