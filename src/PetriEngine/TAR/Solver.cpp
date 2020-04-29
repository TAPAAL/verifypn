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
#include "PetriEngine/TAR/RangeContext.h"
#include "PetriEngine/TAR/RangeEvalContext.h"

#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Contexts.h"
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
            _dirty.resize(_net.numberOfPlaces());
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
            /*uint64_t mn = std::numeric_limits<decltype(mn)>::max();
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
            }*/
        }

        Solver::interpolant_t Solver::findFree(trace_t& trace)
        {
            assert(trace.back().get_edge_cnt() == 0);
            for(int64_t step = ((int64_t)trace.size())-2; step >= 1; --step)
            {
                interpolant_t inter(2);
                {
                    state_t& s = trace[step];
                    inter[0].second = s.get_edge_cnt();
                    auto t = s.get_edge_cnt() - 1;
                    auto post = _net.postset(t);
                    for(; post.first != post.second; ++post.first)
                    {
                        if(_inq[post.first->place])
                        {
                            auto& pr = inter.back().first.find_or_add(post.first->place);
                            pr._range._lower = post.first->tokens;
                        }
                    }
                    inter.back().first.compact();
                    if(inter.back().first.is_true()) continue;
                }

                // OK, lets try to forward approximate result from here
                {
                    for(size_t f = step+1; f < trace.size(); ++f)
                    {
                        state_t& s = trace[f];
                        auto t = s.get_edge_cnt();
                        if(t == 0)
                        {
                            inter.back().second = 0;
/*                            std::cerr << "BACK " << std::endl;
                            // got check the query with the set!
                            for(auto& in : inter)
                            {
                                in.first.print(std::cerr) << std::endl;
                                auto t = in.second;
                                if( t == 0)
                                    std::cerr << ">> Q << " << std::endl;
                                else
                                    std::cerr << ">> T" << (t-1) << " << " << std::endl;
                            }*/
                            RangeEvalContext ctx(inter.back().first, _net, _use_count.get());
                            //inter.back().first.print(std::cerr) << std::endl;
                            _query->visit(ctx);
/*                            _query->toString(std::cerr);
                            std::cerr << std::endl;
                            std::cerr << "AFTER QUERY" << std::endl;
                            inter.back().first.print(std::cerr) << std::endl;*/
                            if(!ctx.satisfied() && !ctx.constraint().is_false(_net.numberOfPlaces()))
                            {
                                
                                /*std::cerr << "\n\nBETTER\n";
                                ctx.constraint().print(std::cerr) << std::endl;
                                inter.back().first.print(std::cerr) << std::endl;                              
                                std::cerr << "\n IS INVALID IN " << std::endl;
                                if(ctx.constraint().is_true()){
                                    RangeEvalContext ctx(inter.back().first, _net, _use_count.get());
                                    _query->visit(ctx);
                                }
                                */

                                inter.back().first = ctx.constraint();
                                assert(!ctx.constraint().is_true());
                                inter.back().first.compact();
                                assert(inter.back().first.is_compact());
                                // flush non-used
                                for(int64_t i = inter.size()-2; i >= 0; --i)
                                {
                                    auto j = i+1;
                                    auto t = inter[i].second;
                                    inter[i].first = inter[j].first;
                                    assert(t > 0);
                                    --t;
                                    auto post = _net.postset(t);
                                    //std::cerr << "REDUCE " ;
                                    //inter[i].first.print(std::cerr) << std::endl;
                                    for(; post.first != post.second; ++post.first)
                                    {
                                        auto it = inter[i].first[post.first->place];
                                        if(it == nullptr) continue;
                                        if(it->_range._upper < post.first->tokens)
                                        {
                                            break;
                                        }
                                        it->_range -= post.first->tokens;
                                    }
                                    inter[i].first.compact();
/*                                    inter[i].first.print(std::cerr) << std::endl;
                                    assert(inter[i].first.is_compact());
                                    inter[i].first.print(std::cerr) << std::endl;*/
                                    if(inter[i].first.is_true())
                                    {
                                        inter.erase(inter.begin(), inter.begin());
                                        assert(inter.front().first.is_true());
                                        break;
                                    }
                                    auto pre = _net.preset(t);
                                    for(; pre.first != pre.second; ++pre.first)
                                    {
                                        auto it = inter[i].first[pre.first->place];
                                        if(it == nullptr) continue;
                                        it->_range += pre.first->tokens;
                                    }
                                    assert(inter[i].first.is_compact());
                                }
                                /*for(auto& in : inter)
                                {
                                    in.first.print(std::cerr) << std::endl;
                                    auto t = in.second;
                                    if( t == 0)
                                        std::cerr << ">> Q << " << std::endl;
                                    else
                                        std::cerr << ">> T" << (t-1) << " << " << std::endl;
                                    assert(in.first.is_compact());
                                }*/
                                assert(!ctx.satisfied());
                                if(!inter.front().first.is_true())
                                    break;
                                return inter;
                            }
                            break;
                        }
                        else
                        {
                            inter.emplace_back();
                            inter[inter.size()-2].second = t;
                            inter.back().first = inter[inter.size()-2].first;
                            auto& range = inter.back().first;
                            --t;
                            auto pre = _net.preset(t);
                            for(; pre.first != pre.second; ++pre.first)
                            {
                                if(_inq[pre.first->place])
                                {
                                    auto it = range[pre.first->place];
                                    if(it == nullptr) continue;
                                    if(it->_range._lower <= pre.first->tokens)
                                    {
                                        // free backwards
                                        for(auto& tmp : inter)
                                        {
                                            auto it = tmp.first[pre.first->place];
                                            if(it)
                                            {
                                                it->_range._lower = 0;
                                                tmp.first.compact();
                                            }
                                        }
                                    }
                                    else
                                    {
                                        it->_range._lower -= pre.first->tokens;
                                    }
                                }
                            }
                            
                            auto post = _net.postset(t);
                            for(; post.first != post.second; ++post.first)
                            {
                                if(_inq[post.first->place])
                                {
                                    auto& pr = range.find_or_add(post.first->place);
                                    if(pr._range._lower == 0)
                                        pr._range._lower = post.first->tokens;
                                    else
                                        pr._range._lower += post.first->tokens;
                                }
                            }
                        }
                        inter.back().first.compact();
                    }
                }
            }
            return {};
        }
        
        int64_t Solver::findFailure(trace_t& trace, bool to_end)
        {
            for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
            {
                _m[p] = _initial[p];
                _mark[p] = _m[p];
            }
            int64_t fail = 0;
            int64_t first_fail = std::numeric_limits<decltype(first_fail)>::max();
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
                        {
                            _dirty[p] = true;
                            _mark[p] = 0;
                        }
                        else _mark[p] = _m[p];
                    }
                    if(_query->getQuantifier() != Quantifier::UPPERBOUNDS)
                    {                        
                        EvaluationContext ctx(_mark.get(), &_net);
                        auto r = _query->evalAndSet(ctx);
#ifndef NDEBUG 
                        if(first_fail == std::numeric_limits<decltype(first_fail)>::max())
                        {
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
                                {
                                    EvaluationContext ctx(_mark.get(), &_net);
                                    auto otherr = _query->evalAndSet(ctx);
                                    assert(otherr == r);
                                }
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
                        }
#endif

                        if(r == Condition::RTRUE)
                        {
                            if(first_fail != std::numeric_limits<decltype(first_fail)>::max())
                            {
                                return first_fail;
                            }
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
                        UnfoldedUpperBoundsCondition* ub = static_cast<UnfoldedUpperBoundsCondition*>(_query);
                        if(first_fail != std::numeric_limits<decltype(first_fail)>::max())
                        {
                            auto value = ub->value(_mark.get());
                            if(value <= ub->bounds(false))
                                return fail;
                            else
                                return first_fail;
                        }
                        else
                        {
                            EvaluationContext ctx(_mark.get(), &_net);
                            _query->evalAndSet(ctx);
                            return fail;
                        }
                    }
                }
                else
                {
                    --t;
                    auto pre = _net.preset(t);
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
//                                std::cerr << "FAIL " << fail << " : T" << t << std::endl;
                            }
                            if(_inq[pre.first->place] && !to_end)
                            {
//                                std::cerr << "## INQ" << std::endl;
                                return first_fail;
                            }
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
                }
            }
            assert(false);
            return -1;
        }
        
        bool Solver::computeTerminal(state_t& end, inter_t& last)
        {
            last.second = end.get_edge_cnt();
            if(end.get_edge_cnt() == 0)
            {
                RangeContext ctx(last.first, _mark.get(), _net, _use_count.get(), _mark.get(), _dirty);
                _query->visit(ctx);
                if(ctx.is_dirty())
                    return false;
                last.first.compact();
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
            return true;
        }

        bool Solver::computeHoare(trace_t& trace, interpolant_t& ranges, int64_t fail)
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
                        auto& r = ranges[fail].first.find_or_add(post.first->place);
                        if(r._range._upper < post.first->tokens)
                        {
                            _dirty[post.first->place] = true;
                            return false;
                        }
                        r -= post.first->tokens;   
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
                            auto& r = ranges[fail].first.find_or_add(post.first->place);
                            if(r._range._upper < (post.first->tokens - pre.first->tokens))
                            {
                                _dirty[post.first->place] = true;
                                return false;
                            }
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
                ranges[fail].second = s.get_edge_cnt();
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
            return true;
        }

        bool Solver::check(trace_t& trace, TraceSet& interpolants)
        {
            std::fill(_dirty.begin(), _dirty.end(), false);
//            std::cerr << "SOLVE! " << (++cnt) << std::endl;
            auto back_inter = findFree(trace);
            if(back_inter.size() > 0)
                interpolants.addTrace(back_inter);

            auto fail = findFailure(trace, true);
            interpolant_t ranges;
            if(fail == std::numeric_limits<decltype(fail)>::max())
            {
                assert(back_inter.empty());
                return true;
            }
            ranges.resize(fail+1);
            bool ok = computeTerminal(trace[fail], ranges.back());
            if(ok)
            {
                if(computeHoare(trace, ranges, fail-1))
                    interpolants.addTrace(ranges);
            }
            do {
                fail = findFailure(trace, true);
                assert(fail != std::numeric_limits<decltype(fail)>::max());
                ranges.clear();
                ranges.resize(fail+1);
                if(!ok)
                    break;
                ok = computeTerminal(trace[fail], ranges.back());
                if(ok)
                {
                    if(computeHoare(trace, ranges, fail-1))
                    {
                        interpolants.addTrace(ranges);                        
                        if(fail != trace.size() - 1)
                            break;
                    }
                    bool some = false;
                    for(auto& r : ranges.back().first._ranges)
                    {
                        some |= !_dirty[r._place];
                        _dirty[r._place] = true;
                    }
                    if(!some) goto NXT;
                    assert(some);
                }
                else
                {
                    NXT:
                    fail = findFailure(trace, false);
                    ranges.clear();
                    ranges.resize(fail+1);
                    std::fill(_dirty.begin(), _dirty.end(), false);
                    computeTerminal(trace[fail], ranges.back());
                    computeHoare(trace, ranges, fail-1);
                    interpolants.addTrace(ranges);
                    break;
                }
            } while(true);
            assert(!ranges.empty());
/*#ifndef NDEBUG
            if(!back_inter.empty())
                std::cerr << "BACKWARDS SOLVED" << std::endl;
#endif*/
            return false;
        }
    }
}
