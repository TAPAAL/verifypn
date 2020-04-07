/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   Solver.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on April 3, 2020, 8:08 PM
 */

#include "PetriEngine/TAR/Solver.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reachability/RangeContext.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"
#include <memory>
#include <vector>

namespace PetriEngine {
    namespace Reachability {
        using namespace PQL;
        Solver::Solver(PetriNet& net, MarkVal* initial, Condition* query, std::vector<bool>& inq)
        : _net(net), _initial(initial), _query(query), _inq(inq) 
#ifndef NDEBUG
        , _gen(_net)
#endif
        {
            _m = std::make_unique<int64_t[]>(_net.numberOfPlaces());
            _failm = std::make_unique<int64_t[]>(_net.numberOfPlaces());
            _mark = std::make_unique<MarkVal[]>(_net.numberOfPlaces());    
            _use_count = std::make_unique<uint64_t[]>(_net.numberOfPlaces());
            for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                if(inq[p])
                    ++_use_count[p];
            for(size_t t = 0; t < _net.numberOfTransitions(); ++t)
            {
                auto [it, end] = _net.preset(t);
                for(; it != end; ++it)
                {
                    _use_count[it->place] += _net.numberOfTransitions();
                }
            }
            
            // make total order
            uint64_t mn = std::numeric_limits<decltype(mn)>::max();
            for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
            {
                mn = std::min(mn, _use_count[p]);
            }
            bool changed = true;
            while(changed)
            {
                changed = false;
                uint64_t next = std::numeric_limits<decltype(next)>::max();
                bool found = false;
                for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                {
                    if(_use_count[p] > mn)
                    {
                        _use_count[p] += 2;
                        next = std::min(_use_count[p], next);
                        changed = true;
                    }
                    else if(_use_count[p] == mn)
                    {
                        if(found)
                        {
                            _use_count[p] += 1;
                            next = _use_count[p];
                            changed = true;
                        }
                        else found = true;
                    }
                }
                mn = next;
            }
        }
        
        
        int64_t Solver::findFailure(trace_t& trace)
        {
            int64_t fail = 0;
            int64_t first_fail = std::numeric_limits<decltype(first_fail)>::max();
            int64_t old_place = -1;
            for(; fail < (int64_t)trace.size(); ++fail)
            {
                state_t& s = trace[fail];
                auto t = s.get_edge_cnt();
                if(t == 0)
                {
#ifdef VERBOSETAR
                    std::cerr << "CHECKQ" << std::endl;
                    _query->toString(std::cerr);
                    std::cerr << std::endl;
#endif
                    for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                    {
                        if(_m[p] < 0)
                            return first_fail;
                        _mark[p] = _m[p];
                    }
                    EvaluationContext ctx(_mark.get(), &_net);
                    auto r = _query->evalAndSet(ctx);
#ifndef NDEBUG 
                    for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                    {
                        _mark[p] = _initial[p];
                    }
                    for(auto& t : trace)
                    {
                        Structures::State s;
                        s.setMarking(_mark.get());
                        _gen.prepare(&s);
                        if(t.get_edge_cnt() == 0)
                            assert(_query->evaluate(ctx) == r);
                        else if(_gen.checkPreset(t.get_edge_cnt()-1))
                        {
                            _gen.consumePreset(s, t.get_edge_cnt()-1);
                            _gen.producePostset(s, t.get_edge_cnt()-1);
                        }
                        else
                        {
                            assert(false);
                        }
                        s.setMarking(nullptr);
                    }
#endif

                    if(r == Condition::RTRUE)
                    {
                        if(first_fail != std::numeric_limits<decltype(first_fail)>::max())
                            return first_fail;
                        return std::numeric_limits<decltype(fail)>::max();
                    }
                    else
                    {
                        assert(r == Condition::RFALSE);
                        return fail;
                    }
                }
                else
                {
                    --t;
                    auto pre = _net.preset(t);
#ifndef NDEBUG
                    if(first_fail > fail)
                    {
                        for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                                assert(_mark[p] == _m[p]);
                    }
#endif
/*                    std::cerr << "F" << fail << " (T" << t << ") " << " : ";
                    for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                    {
                        if(_m[p])
                            std::cerr << "P" << p << "<" << _m[p] << ">,";
                    }
                    std::cerr << std::endl;
*/ 
                    for(; pre.first != pre.second; ++pre.first)
                    {
                        if(_m[pre.first->place] < pre.first->tokens)
                        {
                            if(fail < first_fail/* || _use_count[pre.first->place] > _use_count[old_place]*/)
                            {
                                std::copy(_m.get(), _m.get() + _net.numberOfPlaces(), _failm.get());
                                first_fail = fail;
                                old_place = pre.first->place;
//                                std::cerr << "FAIL " << fail << " : T" << t << std::endl;
                            }
                            //if(_inq[pre.first->place])
                                return first_fail;
                        }
                    }
                    pre = _net.preset(t);
                    for(; pre.first != pre.second; ++pre.first)
                    {
                        _m[pre.first->place] -= pre.first->tokens;
                    }
                    auto post = _net.postset(t);
                    for(; post.first != post.second; ++post.first)
                    {
                        _m[post.first->place] += post.first->tokens;
                    }
#ifndef NDEBUG
                    if(first_fail > fail)
                    {
                        Structures::State s;
                        s.setMarking(_mark.get());
                        _gen.prepare(&s);
                        if(_gen.checkPreset(t))
                        {
                            _gen.consumePreset(s, t);
                            _gen.producePostset(s, t);
                        }
                        else
                        {
                            assert(false);
                        }
                        for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                            assert(_mark[p] == _m[p]);
                        s.setMarking(nullptr);
                    }
#endif
                }
            }
            assert(false);
            return -1;
        }
        
        void Solver::computeTerminal(state_t& end, inter_t& last)
        {
            if(end.get_edge_cnt() == 0)
            {
                RangeContext ctx(last.first, _mark.get(), _net, _use_count.get(), _mark.get());
                _query->visit(ctx);
#ifdef VERBOSETAR
                std::cerr << "TERMINAL IS Q" << std::endl;
                //last.first.print(std::cerr) << std::endl;
#endif
            }
            else
            {
#ifdef VERBOSETAR
                std::cerr << "TERMINAL IS T" << (end.get_edge_cnt()-1) << std::endl;
#endif
                
#ifndef NDEBUG
                bool some = false;
#endif
                auto pre = _net.preset(end.get_edge_cnt()-1);
                uint64_t mx = 0;
                placerange_t pr;
                for(; pre.first != pre.second; ++pre.first)
                {
                    assert(!pre.first->inhibitor);
                    assert(pre.first->tokens >= 1);
                    if(_failm[pre.first->place] < pre.first->tokens && mx < _use_count[pre.first->place])
                    {
#ifndef NDEBUG
                        some = true;
#endif
                        pr = placerange_t();
                        pr._place = pre.first->place;
                        pr._range._upper = pre.first->tokens-1;
                    }
                }
                last.first.find_or_add(pr._place) = pr;
                assert(some);
            }
#ifdef VERBOSETAR
            std::cerr << "[FAIL] : ";
            last.first.print(std::cerr) << std::endl;
#endif
            last.second = true;
        }

        void Solver::computeHoare(trace_t& trace, interpolant_t& ranges, int64_t fail)
        {
            for(; fail >= 0; --fail)
            {
                ranges[fail].first.copy(ranges[fail+1].first);
                state_t& s = trace[fail];
                bool touches = false;
                auto t = s.get_edge_cnt()-1;
                auto post = _net.postset(t);
                auto pre = _net.preset(t);
//                std::cerr << "T" << t << "\n";
                for(; post.first != post.second; ++post.first)
                {
                    auto* pr = ranges[fail+1].first[post.first->place];
                    if(pr == nullptr || pr->_range.unbound()) continue;
                    assert(post.first->place == pr->_place);
                    for(; pre.first != pre.second; ++pre.first)
                    {
                        if(pre.first->place < post.first->place)
                        {
                            auto* prerange = ranges[fail+1].first[pre.first->place];
                            if(prerange == nullptr || prerange->_range.unbound()) continue;
                            ranges[fail].first.find_or_add(pre.first->place) += pre.first->tokens;
                            touches = true;
//                            std::cerr << "\t1P" << pre.first->place << " -" << pre.first->tokens << std::endl;
                        }
                        else break;
                    }
                    if(pre.first == pre.second || pre.first->place != post.first->place)
                    {
//                        std::cerr << "\t2P" << post.first->place << " +" << post.first->tokens << std::endl;
                        ranges[fail].first.find_or_add(post.first->place) -= post.first->tokens;   
                    }
                    else
                    {
                        if(pre.first->direction < 0)
                        {
                            assert(pre.first->tokens > post.first->tokens);
                            ranges[fail].first.find_or_add(post.first->place) += (pre.first->tokens - post.first->tokens);
//                            std::cerr << "\t3P" << pre.first->place << " -" << (pre.first->tokens - post.first->tokens) << std::endl;
                        }
                        else
                        {
                            assert(pre.first->tokens <= post.first->tokens);
                            ranges[fail].first.find_or_add(post.first->place) -= (post.first->tokens - pre.first->tokens);
//                            std::cerr << "\t4P" << pre.first->place << " +" << (pre.first->tokens - post.first->tokens) << std::endl;
                        }
                        ++pre.first;
                    }
                    touches = true;
                }

                // handles rest
                for(; pre.first != pre.second; ++pre.first)
                {
                    assert(!pre.first->inhibitor);
                    assert(pre.first->tokens >= 1);
                    auto* pr = ranges[fail+1].first[pre.first->place];
                    if(pr == nullptr || pr->_range.unbound()) continue;
                    ranges[fail].first.find_or_add(pre.first->place) += pre.first->tokens;
//                    std::cerr << "\t5P" << pre.first->place << " -" << pre.first->tokens << std::endl;
                    touches |= true;
                }
                ranges[fail].second = touches;
#ifdef VERBOSETAR
                if(ranges[fail].second)
                {
                    std::cerr << "[" << fail << "] : <T" << t << "> ";
                    if(ranges[fail].second)
                        std::cerr << "TOUCHES" << std::endl;
                    else
                        std::cerr << "NO TOUCH" << std::endl;
                    ranges[fail].first.print(std::cerr) << std::endl;
                    std::cerr << std::endl;
                }
#endif
            }            
        }
        
        std::pair<bool,Solver::interpolant_t> Solver::check(trace_t& trace)
        {
            for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
            {
                _m[p] = _initial[p];
                _mark[p] = _m[p];
            }
            auto fail = findFailure(trace);
            interpolant_t ranges;
            if(fail == std::numeric_limits<decltype(fail)>::max())
            {
                return std::make_pair(true, std::move(ranges));
            }
            ranges.resize(fail+1);
            computeTerminal(trace[fail], ranges.back());
            computeHoare(trace, ranges, fail-1);
            assert(!ranges.empty());
            return std::make_pair(false, std::move(ranges));
        }
    }
}
