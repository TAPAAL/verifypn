/* 
 * File:   TARReachability.cpp
 * Author: Peter G. Jensen
 * 
 * Created on January 2, 2018, 8:36 AM
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef VERIFYPN_TAR

#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/TAR/AntiChain.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/Reachability/TARReachability.h"
#include "PetriEngine/Reachability/RangeContext.h"

#define NOCHANGE
#define ANTISIM

namespace PetriEngine {
    using namespace PQL;
    namespace Reachability {

        void inline_union(std::vector<size_t>& into, const std::vector<size_t>& other)
        {
            into.reserve(into.size() + other.size());
            auto iit = into.begin();
            auto oit = other.begin();

            while(oit != other.end())
            {
                while(iit != into.end() && *iit < *oit) ++iit;
                if(iit == into.end())
                {
                    into.insert(iit, oit, other.end());
                    break;
                }
                else if(*iit != *oit)
                {
                    iit = into.insert(iit, *oit);
                }
                ++oit;
            }
        }       
        
        
        std::pair<bool, size_t> TARReachabilitySearch::stateForPredicate(prvector_t& predicate, size_t sim_hint, size_t simed_hint)
        {
            predicate.compress();
            assert(predicate.is_compact());
            if(predicate.is_true()) 
            {
                return std::make_pair(false, 1);
            }
            else if (predicate.is_false(_net.numberOfPlaces()))
            {        
                return std::make_pair(false, 0);   
            } 
            
            auto astate = states.size();
            auto res = intmap.emplace(predicate, astate);
            if(!res.second)
            {
#ifndef NDEBUG
                if(!(states[res.first->second].interpolant == predicate))
                {
                    assert(!(states[res.first->second].interpolant < predicate));
                    assert(!(predicate < states[res.first->second].interpolant));
                    assert(false);
                }
                for(auto& e : intmap)
                {
                    assert(e.first == states[e.second].interpolant);
                }
#endif
                return std::make_pair(false, res.first->second);
            }
            else
            {
                states.emplace_back(predicate);
                computeSimulation(astate);
                res.first->second = astate;
                assert(states[astate].interpolant == predicate);
#ifndef NDEBUG
                for(auto& s : states)
                {
                    if(s.interpolant == predicate)
                    {
                        if(&s == &states[astate]) continue;
                        assert( (s.interpolant < predicate) ||
                                (predicate < s.interpolant));
                        assert(false);
                    }
                }
                for(auto& e : intmap)
                {
                    assert(e.first == states[e.second].interpolant);
                }
#endif

                return std::make_pair(true, astate);
            }
        }

        
        void TARReachabilitySearch::constructAutomata(waiting_t& trace, std::vector<std::pair<prvector_t,bool>>& inter)
        {
            assert(inter.size() > 0);
            bool some = false;

            size_t last = 1;
            {
                auto res = stateForPredicate(inter[0].first);
                some |= res.first;
                auto lb = std::lower_bound(initial_interpols.begin(), initial_interpols.end(), res.second);
                if(lb == std::end(initial_interpols) || *lb != res.second)
                {
                    initial_interpols.insert(lb, res.second);
                    trace[0].add_interpolant(res.second);
                }
                last = res.second;
            }
#ifndef NDEBUG
            bool added_terminal = false;
#endif
            for(size_t i = 0; i < inter.size(); ++i)
            {
                size_t j = i+1;

                if(j == inter.size())
                {
                    some |= states[last].add_edge(trace[i].get_edge_cnt(), 0);
#ifndef NDEBUG                    
                    added_terminal = true;
#endif
                }
                else
                {
                    if(!inter[i].second)
                        trace[j].add_interpolant(last);
                    else
                    {
                        auto res = stateForPredicate(inter[j].first);
                        some |= res.first;
                        assert(inter[i].second || res.second == last); 
                        some |= states[last].add_edge(trace[i].get_edge_cnt(), res.second);
                        last = res.second;
                    }
                }
            }
            assert(some);
            assert(added_terminal);
            /*std::vector<size_t> waiting = initial_interpols;
            std::set<size_t> passed;
            passed.insert(waiting.begin(), waiting.end());
            while(!waiting.empty())
            {
                auto s = waiting.back();
                waiting.pop_back();
                std::cerr << "[" << s <<"]:";
                states[s].print(std::cerr);
                for(auto& e : states[s].get_edges())
                {
                    for(auto t : e.to)
                    {
                        if(passed.count(t) == 0)
                        {
                            passed.insert(t);
                            waiting.push_back(t);
                        }
                    }
                }
            }*/
        }
                
        std::pair<int,bool>  TARReachabilitySearch::isValidTrace(waiting_t& trace, Structures::State& initial, const std::vector<bool>& inq, Condition* query)
        {
            size_t nvalid = 0;
            {
                std::unique_ptr<int64_t[]> lastfailplace = std::make_unique<int64_t[]>(_net.numberOfPlaces());
                std::unique_ptr<int64_t[]> lfpc = std::make_unique<int64_t[]>(_net.numberOfPlaces());
                std::unique_ptr<int64_t[]> m = std::make_unique<int64_t[]>(_net.numberOfPlaces());
                std::unique_ptr<MarkVal[]> mark = std::make_unique<MarkVal[]>(_net.numberOfPlaces());
                for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                {
                    m[p] = initial.marking()[p];
                    mark[p] = m[p];
                    lastfailplace[p] = -1;
                    lfpc[p] = 0;
                }
                    
                int64_t fail = 0; 
#ifndef NDEBUG
                bool qfail = false;
#endif
                int64_t lastfail = -1;
                int64_t place = 0;
#ifdef VERBOSETAR
                SuccessorGenerator gen(_net);
#endif
                for(; fail < (int64_t)trace.size(); ++fail)
                {
                    state_t& s = trace[fail];
                    auto t = s.get_edge_cnt();
                    if(t == 0)
                    {
#ifdef VERBOSETAR
                        std::cerr << "CHECKQ" << std::endl;
#endif
                        bool ok = true;
                        place = -1;
                        for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                        {
                            if(inq[p])
                            {
                                ok &= m[p] >= 0;
                                ok &= m[p] <= std::numeric_limits<MarkVal>::max();
                                mark[p] = std::min<int64_t>(std::max<int64_t>(0,m[p]), std::numeric_limits<MarkVal>::max());
                            }
                            if(lastfailplace[p] != -1 && place == -1)
                            {
                                lastfail = lastfailplace[p];
                                place = p;
                            }
                        }

                        EvaluationContext ctx(mark.get(), &_net);
                        if(lastfail != -1 || query->evalAndSet(ctx))
                        {
                            if(lastfail != -1)
                            {
                                fail = lastfail;
                                break;
                            }
                            for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                            {
                                mark[p] = initial.marking()[p];
                            }
#ifdef VERBOSETAR                            
                            for(auto& t : trace)
                            {
                                Structures::State s;
                                s.setMarking(mark.get());
                                gen.prepare(&s);
                                if(t.get_edge_cnt() == 0)
                                    assert(query->evaluate(ctx));
                                else if(gen.checkPreset(t.get_edge_cnt()-1))
                                {

                                    gen.consumePreset(s, t.get_edge_cnt()-1);
                                    gen.producePostset(s, t.get_edge_cnt()-1);
                                }
                                else
                                {
                                    assert(false);
                                }
                                s.setMarking(nullptr);
                            }
#endif
                            return std::make_pair(fail, true);
                        }
                        else
                        {
#ifndef NDEBUG
                            qfail = true;
#endif
                            break;
                        }
                    }
                    else
                    {
                        --t;
                        auto pre = _net.preset(t);
                        if(fail == -1)
                        {
                            for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                                    assert(mark[p] == m[p]);
                        }
                        for(; pre.first != pre.second; ++pre.first)
                        {
                            m[pre.first->place] -= pre.first->tokens;
                            if(m[pre.first->place] < 0)
                            {
                                if(lastfailplace[pre.first->place] == -1)
                                    lastfailplace[pre.first->place] = fail;

                                lastfail = fail;
                            }
                            else if(lastfailplace[pre.first->place] == -1)
                            {
                                lfpc[pre.first->place] += 1;
                            }
                        }
                        auto post = _net.postset(t);
                        for(; post.first != post.second; ++post.first)
                        {
                            m[post.first->place] += post.first->tokens;
                            if(lastfailplace[post.first->place] == -1)
                            {
                                lfpc[post.first->place] += 1;
                            }
                        }
#ifdef VERBOSETAR
                        Structures::State s;
                        s.setMarking(mark.get());
                        gen.prepare(&s);
                        if(lastfail == -1)
                        {
                            if(gen.checkPreset(t))
                            {
                                gen.consumePreset(s, t);
                                gen.producePostset(s, t);
                            }
                            else
                            {
                                assert(false);
                            }
                            for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                                assert(mark[p] == m[p]);
                        }
                        s.setMarking(nullptr);
#endif
                    }
                }
                std::vector<std::pair<prvector_t,bool>> ranges(fail+1);
                if(trace[fail].get_edge_cnt() == 0)
                {
                    assert(qfail);
                    RangeContext ctx(ranges[fail].first, mark.get(), _net);
                    query->visit(ctx);
#ifdef VERBOSETAR
                    ranges[fail].first.print(std::cerr) << std::endl;
#endif
                    nvalid = fail+1;
                    --fail;
                }
                else
                {
#ifndef NDEBUG
                    bool some = false;
#endif
                    auto pre = _net.preset(trace[fail].get_edge_cnt()-1);
                    for(; pre.first != pre.second; ++pre.first)
                    {
                        assert(!pre.first->inhibitor);
                        assert(pre.first->tokens >= 1);
                        if(pre.first->place != place)
                            continue;
#ifndef NDEBUG
                        some = true;
#endif
                        auto& npr = ranges[fail].first.find_or_add(pre.first->place);
                        assert(npr._place == pre.first->place);
                        npr &= pre.first->tokens-1;
                        break;
                    }
                    nvalid = fail+1;
                    assert(some);
                    --fail;
                }
#ifdef VERBOSETAR
                std::cerr << "[FAIL] : ";
                ranges[fail+1].first.print(std::cerr) << std::endl;
#endif
                ranges[fail+1].second = true;
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
                            ranges[fail].first.find_or_add(post.first->place) -= post.second->tokens;
                            touches = true;
                        }
                        
                        auto pre = _net.preset(t);
                        for(; pre.first != pre.second; ++pre.first)
                        {
                            assert(!pre.first->inhibitor);
                            assert(pre.first->tokens >= 1);
                            auto* pr = ranges[fail+1].first[pre.first->place];
                            if(pr == nullptr || pr->_range.unbound()) continue;
                            ranges[fail].first.find_or_add(pre.first->place) += pre.second->tokens;
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
                                std::cerr << "INT : ";
                            for(auto& s : trace[fail].get_interpolants())
                            {
                                std::cerr << s << ", ";
                            }
                            std::cerr << std::endl;
                        }
#endif
                    }
                }
                constructAutomata(trace, ranges);
#ifdef VERBOSETAR                
                std::cerr << "S " << states.size() << std::endl;
#endif
            }
            return std::pair<int,bool>(nvalid, false);
        }


        void TARReachabilitySearch::handleInvalidTrace(waiting_t& waiting, int nvalid)
        {
            //sanity(waiting);
            assert(waiting.size() >= (size_t)nvalid);
            waiting.resize(nvalid); // remove invalid part of trace

            for(size_t i = 1; i < waiting.size(); ++i)
            {
                bool brk = false;
                for(size_t j = 0 ; j < i; ++j)
                {
                    if(waiting[j] <= waiting[i])
                    {
                        waiting.resize(i);
                        brk = true;
                        break;
                    }
                }
                if(brk) break;
                std::vector<size_t> buffer;
                if(checkInclussion(waiting[i - 1], waiting[i].get_interpolants()))
                {
                    if(i != 1) waiting.resize(i - 1);
                    break;
                }
            }
            if(waiting.size() == 0) return;
            waiting.back().reset_edges(_net);
        }
        
        
        bool TARReachabilitySearch::popDone(waiting_t& waiting, size_t& stepno)
        {
            bool popped = false;
            while(waiting.back().get_edge_cnt() > _net.numberOfTransitions()) // we have tried all transitions for this state-pair!
            {
                assert(waiting.size() > 0);
                waiting.pop_back(); 
                popped = true;
                if(waiting.size() > 0)
                {
                    // count up parents edge counter!
                    waiting.back().next_edge(_net);
                }
                if(waiting.size() == 0) break;
            }
            return popped;
        }

        void TARReachabilitySearch::computeSimulation(size_t index)
        {
            AutomataState& state = states[index];
            assert(index == states.size()-1 || index == 0);
            for(size_t i = 0; i < states.size(); ++i)
            {
                if(i == index) continue;
                AutomataState& other = states[i];
                std::pair<bool,bool> res = other.interpolant.compare(state.interpolant);
                assert(!res.first || !res.second);
                if(res.first)
                {
                    state.simulates.emplace_back(i);
                    auto lb = std::lower_bound(other.simulators.begin(), other.simulators.end(), index);
                    if(lb == std::end(other.simulators) || *lb != index)
                        other.simulators.insert(lb, index);
                    other.interpolant.compare(state.interpolant);
                }
                if(res.second)
                {
                    state.simulators.emplace_back(i);
                    auto lb = std::lower_bound(other.simulates.begin(), other.simulates.end(), index);
                    if(lb == std::end(other.simulates) || *lb != index)
                        other.simulates.insert(lb, index);
                    other.interpolant.compare(state.interpolant);
                }
            }

            assert(states[1].simulates.size() == 0);
            assert(states[0].simulators.size() == 0);    
        }
        
        
        bool TARReachabilitySearch::tryReach(   const std::shared_ptr<PQL::Condition> & query, 
                                        std::vector<ResultPrinter::Result>& results,
                                        bool printstats, bool printtrace, Structures::State& initial,
                                        const std::vector<bool>& places_in_query)
        {
            
            // Construct our constraints

            prvector_t truerange;
            prvector_t falserange; 
            {
                for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                    falserange.find_or_add(p) &= 0;
            }
            assert(falserange.is_false(_net.numberOfPlaces()));
            assert(truerange.is_true());
            assert(!falserange.is_true());
            assert(falserange.is_false(_net.numberOfPlaces()));
            assert(truerange.compare(falserange).first);
            assert(!truerange.compare(falserange).second);
            assert(falserange.compare(truerange).second);
            assert(!falserange.compare(truerange).first);
            states.emplace_back(falserange); // false
            states.emplace_back(truerange); // true
            computeSimulation(0);

            assert(states[1].simulates.size() == 0);
            assert(states[1].simulators.size() == 1);
            assert(states[0].simulates.size() == 1);
            assert(states[0].simulators.size() == 0);

            assert(states[1].simulators[0] == 0);
            assert(states[0].simulates[0] == 1);
    
            intmap.emplace(falserange, 0);
            intmap.emplace(truerange, 1);

            /* Do the verification */

            bool all_covered = true;
            size_t stepno = 0;

            initial_interpols.push_back(1);
            do {
                all_covered = true;
                auto checked = AntiChain<uint32_t, size_t>();
                // waiting-list with levels
                waiting_t waiting;
                // initialize
                {
                    state_t state;
                    state.reset_edges(_net);
                    state.set_interpolants(initial_interpols);
                    waiting.push_back(state);
                }
                while (!waiting.empty()) 
                {
                    if(popDone(waiting, stepno)) 
                        continue;  // we have reached the end of the edge-iterator for this part of the trace

                    ++stepno;

                    assert(waiting.size() > 0 );
                    state_t& state = waiting.back();
                    std::vector<size_t> nextinter;
                    if(checkInclussion(state, nextinter)) // Check if the next state makes the interpolant automata accept.
                    {
                        state.next_edge(_net);
                        continue;
                    }

                    bool next_edge = false;
                    if(waiting.back().get_edge_cnt() == 0) // check if proposition is satisfied
                    {
                        std::pair<int,bool> res = isValidTrace(waiting, initial, places_in_query, query.get());
                        if(res.second)
                        {
                            std::cerr << "VALID TRACE FOUND!" << std::endl;
                            std::cerr << "STEPS : " << stepno << std::endl;
                            std::cerr << "INTERPOLANT AUTOMATAS : " << waiting[0].get_interpolants().size() << std::endl;
                            intmap.clear();
                            states.clear();
                            if(printtrace)
                                printTrace(waiting);
                            return true;
                        }
                        else
                        {
                            handleInvalidTrace(waiting, res.first);
                            all_covered = false;
                            continue;
                        }
                    }
                    else
                    {
#ifdef VERBOSETAR
                        std::cerr << "STEPS : " << stepno << std::endl;
                        std::cerr << "INTERPOLANT AUTOMATAS : " << waiting[0].get_interpolants().size() << std::endl;
#endif
                        next_edge = true;
                    }

                    if(next_edge)
                    {
                        uint32_t dummy = state.get_edge_cnt() == 0 ? 0 : 1;
                        state_t next;
                        bool res = checked.subsumed(dummy, nextinter);
                        if(res)
                        {
                            waiting.back().next_edge(_net);
                        }
                        else
                        {
                            std::vector<size_t> minimal = nextinter;
#ifdef ANTISIM
                            std::vector<size_t> buffer;
                            size_t cur = 2;
                            while(true)
                            {
                                buffer.clear();
                                auto lb = std::lower_bound(minimal.begin(), minimal.end(), cur);
                                if(lb == minimal.end()) break;
                                cur = *lb;
                                std::set_difference(minimal.begin(), minimal.end(), 
                                                    states[cur].simulates.begin(), states[cur].simulates.end(),
                                                    std::back_inserter(buffer));
                                minimal.swap(buffer);
                                ++cur;
                            }
#endif      
                            checked.insert(dummy, minimal);
                            next.reset_edges(_net);
                            next.set_interpolants(minimal);  
                            waiting.push_back(next);
                        }
                    }
                }
            } while(!all_covered);

            intmap.clear();
            states.clear();
            std::cout << "STEPS : " << stepno << std::endl;
            std::cout << "INTERPOLANT AUTOMATAS : " << initial_interpols.size() << std::endl;
            if(auto upper = dynamic_cast<PQL::UnfoldedUpperBoundsCondition*>(query.get()))
            {
                assert(false);
            }
            
            /*for(size_t t = 0; t < _net.numberOfTransitions(); ++t)
            {
                std::cerr << "T" << t << ":\n";
                std::cerr << "\tPRE:\n";
                auto pre = _net.preset(t);
                for(;pre.first != pre.second; ++pre.first)
                    std::cerr << "\t\t-" << pre.first->tokens << " (P" << pre.first->place << ")\n";
                std::cerr << "\tPOST:\n";
                auto post = _net.postset(t);
                for(;post.first != post.second; ++post.first)
                    std::cerr << "\t\t+" << post.first->tokens << " (P" << post.first->place << ")\n";
            }*/
            
            return false;            
        }

        bool TARReachabilitySearch::checkInclussion(state_t& state, std::vector<size_t>& nextinter)
        {
#ifdef ANTISIM
            auto maximal = expandSimulation(state.get_interpolants());
#else
            auto maximal = state.get_interpolants();
#endif
            // if NFA accepts the trace after this instruction, abort.
            if(followSymbol(maximal, nextinter, state.get_edge_cnt()))
            {
                return true;
            }

#ifdef NOCHANGE
            addNonChanging(state, maximal, nextinter);
#endif
        
#ifdef ANTISIM
            nextinter = expandSimulation(nextinter);
#endif
            return false;
        }
        
        bool TARReachabilitySearch::followSymbol(std::vector<size_t>& from, std::vector<size_t>& nextinter, size_t symbol)
        {
            if(nextinter.size() == 0 || nextinter[0] != 1) nextinter.insert(nextinter.begin(), 1);
            assert(is_sorted(from.begin(), from.end()));
            for(size_t i : from)
            {
                if(i == 0)
                {
                    assert(false);
                    continue;
                }
                AutomataState& as = states[i];
                if(as.is_accepting())
                {
                    assert(false);
                    break;
                }

                auto it = as.first_edge(symbol);

                while(it != as.get_edges().end())
                {
                    if(it->edge != symbol)    { break; }
                    AutomataEdge& ae = *it;
                    ++it;
                    assert(ae.to.size() > 0);
                    auto other = nextinter.begin();
                    auto next = ae.to.begin();
                    if(*next == 0) 
                    {
                        return true;
                    }
                    for(;next != ae.to.end(); ++next)
                    {
                        while(*other < *next && other != nextinter.end())
                        {
                            ++other;
                        }
                        if(other != nextinter.end() && *next == *other)
                        {
                            ++other;
                        }
                        else
                        {
                            other = nextinter.insert(other, *next);
                            assert(*next < states.size());
                            if(states[*next].simulates.size() > 0)
                            {
                                inline_union(nextinter, states[*next].simulates);
                                other = std::lower_bound(nextinter.begin(), nextinter.end(), (*next) + 1);
                            }
                            else
                            {
                                ++other;
                            }
                        }
                        assert(nextinter.size() == 0 || nextinter[0] != 0);
                    }
                }
            }
            return false;            
        }
        
        std::vector<size_t> TARReachabilitySearch::expandSimulation(std::vector<size_t>& org)
        {
            std::vector<size_t> maximal = org;
            assert(is_sorted(maximal.begin(), maximal.end()));
            if(maximal.size() == 0 || maximal[0] != 1) maximal.insert(maximal.begin(), 1);

            assert(maximal.size() == 0 || maximal[0] != 0);
            assert(is_sorted(maximal.begin(), maximal.end()));
            for(size_t i : org)
            {
                inline_union(maximal, states[i].simulates);
            }            
            return maximal;
        }
        
        
        void TARReachabilitySearch::addNonChanging(state_t& state, std::vector<size_t>& maximal, std::vector<size_t>& nextinter)
        {
            bool loaded = false;
            auto next = nextinter.begin();
            std::vector<uint32_t> writes;

            if(!state.get_edge_cnt() == 0)
            for(size_t i : maximal)
            {
                if(i == 0)
                {
                    assert(false);
                    continue;
                }
                AutomataState& as = states[i];
                while(next != nextinter.end() && *next < i) { ++next; };
                if(next == nextinter.end() || *next != i)
                {
                    // added non-interfering
                    if(!loaded)
                    {
                        auto pre = _net.preset(state.get_edge_cnt() - 1);
                        auto post = _net.postset(state.get_edge_cnt() - 1);
                        auto lb = writes.begin();
                        for(; pre.first != pre.second; ++pre.first)
                        {
                            if(pre.first->inhibitor) continue;
                            while(lb != writes.end() && *lb < pre.first->place) ++lb;
                            if(lb == writes.end() || *lb != pre.first->place)
                                lb = writes.insert(lb, pre.first->place);
                        }
                        assert(std::is_sorted(writes.begin(), writes.end()));
                        lb = writes.begin();
                        for(; post.first != post.second; ++post.first)
                        {
                            while(lb != writes.end() && *lb < post.first->place) ++lb;
                            if(lb == writes.end() || *lb != post.first->place)
                                lb = writes.insert(lb, post.first->place);
                        }
                        assert(std::is_sorted(writes.begin(), writes.end()));

                        loaded = true;
                    }
    
                    if(!as.interpolant.restricts(writes))
                    {
                        next = nextinter.insert(next, i);
                        if(as.simulates.size() > 0)
                        {
                            inline_union(nextinter, as.simulates);
                            next = std::lower_bound(nextinter.begin(), nextinter.end(), i + 1);
                        }
                    }
                }
            }            
        }
        
        void TARReachabilitySearch::printTrace(waiting_t& stack)
        {
            std::cerr << "Trace:\n<trace>\n";
            
            if(_reducer != NULL)
                _reducer->initFire(std::cerr);
            
            for(auto& t : stack)
            {
                if(t.get_edge_cnt() == 0) break;
                std::string tname = _net.transitionNames()[t.get_edge_cnt() - 1];
                std::cerr << "\t<transition id=\"" << tname << "\" index=\"" << (t.get_edge_cnt() - 1) <<  "\">\n";
                
                // well, yeah, we are not really efficient in constructing the trace.
                // feel free to improve
                auto pre = _net.preset(t.get_edge_cnt() - 1);
                for(; pre.first != pre.second; ++pre.first)
                {                    
                    for(size_t token = 0; token < pre.first->tokens; ++token )
                    {
                        std::cerr << "\t\t<token place=\"" << _net.placeNames()[pre.first->place] << "\" age=\"0\"/>\n";
                    }
                }
                
                if(_reducer != NULL)
                    _reducer->extraConsume(std::cerr, tname);
                
                std::cerr << "\t</transition>\n";
                
                if(_reducer != NULL)
                    _reducer->postFire(std::cerr, tname);
                
            }
            
            std::cerr << "</trace>\n" << std::endl;
        }
        
        void TARReachabilitySearch::reachable(   std::vector<std::shared_ptr<PQL::Condition> >& queries, 
                                        std::vector<ResultPrinter::Result>& results,
                                        bool printstats, bool printtrace, PetriNetBuilder& builder)
        {

            // set up working area
            Structures::State state;
            state.setMarking(_net.makeInitialMarking());
            
            // check initial marking
            if(checkQueries(queries, results, state, true))
            {
                if(printstats) 
                    printStats();
                return;
            }
            
            // Search!
            std::vector<bool> used(_net.numberOfPlaces(), false);
            for(size_t i = 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    QueryPlaceAnalysisContext pa(builder.getPlaceNames(), builder.getTransitionNames(), &_net);
                    queries[i]->analyze(pa);
                    for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                        used[p] = pa.getQueryPlaceCount()[p] > 0;
                    bool res = tryReach(queries[i], results, printstats, printtrace, state, used);
                    if(res)
                        results[i] = ResultPrinter::Satisfied;
                    else
                        results[i] = ResultPrinter::NotSatisfied;
                    results[i] = printQuery(queries[i], i, results[i]);  
                }
            }

            if(printstats) 
                printStats();
        }


        bool TARReachabilitySearch::checkQueries(  std::vector<std::shared_ptr<PQL::Condition > >& queries,
                                                std::vector<ResultPrinter::Result>& results,
                                                Structures::State& state, bool usequeries)
        {
            if(!usequeries) return false;
            
            bool alldone = true;
            for(size_t i = 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    EvaluationContext ec(state.marking(), &_net);
                    if(queries[i]->evaluate(ec) == Condition::RTRUE)
                        results[i] = printQuery(queries[i], i, ResultPrinter::Satisfied);
                    else
                        alldone = false;
                }
            }  
            return alldone;
        }        
        
        ResultPrinter::Result TARReachabilitySearch::printQuery(std::shared_ptr<PQL::Condition>& query, size_t i,  ResultPrinter::Result r)
        {
            return printer.printResult(i, query.get(), r,
                            0, 0, 0,
                            {}, 0, 
                            {}, nullptr, 0);  
        }
        
        void TARReachabilitySearch::printStats()
        {
            // Stats
        }        
        
    }
}

#endif
