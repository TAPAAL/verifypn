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
                        ++_use_count[it->place];
                    }
                }
        }
        
        
        std::pair<int64_t,int64_t> Solver::findFailure(trace_t& trace)
        {
            int64_t fail = 0;
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
                        return std::make_pair(-1, std::numeric_limits<decltype(fail)>::max());
                    }
                    else
                    {
                        assert(r == Condition::RFALSE);
                        return std::make_pair(-1, fail);
                    }
                }
                else
                {
                    --t;
                    auto pre = _net.preset(t);
#ifndef NDEBUG
                    for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                            assert(_mark[p] == _m[p]);
#endif
                    for(; pre.first != pre.second; ++pre.first)
                    {
                        _m[pre.first->place] -= pre.first->tokens;
                        if(_m[pre.first->place] < 0)
                            return std::make_pair(pre.first->place, fail);
                    }
                    auto post = _net.postset(t);
                    for(; post.first != post.second; ++post.first)
                    {
                        _m[post.first->place] += post.first->tokens;
                    }
#ifndef NDEBUG
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
#endif
                }
            }
            assert(false);
            return std::make_pair(-1, -1);
        }
        
        void Solver::computeTerminal(state_t& end, inter_t& last, int64_t place)
        {
            if(end.get_edge_cnt() == 0)
            {
                RangeContext ctx(last.first, _mark.get(), _net, _use_count.get());
                _query->visit(ctx);
#ifdef VERBOSETAR
                std::cerr << "TERMINAL IS Q" << std::endl;
                last.first.print(std::cerr) << std::endl;
#endif
            }
            else
            {
                assert(place != -1);
#ifdef VERBOSETAR
                std::cerr << "TERMINAL IS T" << (end.get_edge_cnt()-1) << std::endl;
#endif
                
#ifndef NDEBUG
                bool some = false;
#endif
                auto pre = _net.preset(end.get_edge_cnt()-1);
                uint64_t mx = 0;
                for(; pre.first != pre.second; ++pre.first)
                {
                    assert(!pre.first->inhibitor);
                    assert(pre.first->tokens >= 1);
                    if(_mark[pre.first->place] < pre.first->tokens && mx < _use_count[pre.first->place])
                    {
#ifndef NDEBUG
                        some = true;
                        assert(_mark[pre.first->place] < pre.first->tokens);
#endif
                        auto& npr = last.first.find_or_add(pre.first->place);
                        assert(npr._place == pre.first->place);
                        npr._range._upper = pre.first->tokens-1;
                        break;
                    }
                }
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
                for(; post.first != post.second; ++post.first)
                {
                    auto* pr = ranges[fail+1].first[post.first->place];
                    if(pr == nullptr || pr->_range.unbound()) continue;
                    ranges[fail].first.find_or_add(post.first->place) -= post.first->tokens;
                    touches = true;
                }

                auto pre = _net.preset(t);
                for(; pre.first != pre.second; ++pre.first)
                {
                    assert(!pre.first->inhibitor);
                    assert(pre.first->tokens >= 1);
                    auto* pr = ranges[fail+1].first[pre.first->place];
                    if(pr == nullptr || pr->_range.unbound()) continue;
                    ranges[fail].first.find_or_add(pre.first->place) += pre.first->tokens;
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
            auto [place, fail] = findFailure(trace);
            interpolant_t ranges;
            if(fail == std::numeric_limits<decltype(fail)>::max())
            {
                return std::make_pair(true, std::move(ranges));
            }
            ranges.resize(fail+1);
            computeTerminal(trace[fail], ranges.back(), place);
            computeHoare(trace, ranges, fail-1);
            assert(!ranges.empty());
            return std::make_pair(false, std::move(ranges));
        }
    }
}
