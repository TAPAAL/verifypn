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
 
#include <z3++.h>

#include "TARReachability.h"
#include "../PQL/Contexts.h"
#include "../TAR/AntiChain.h"
#include "../TAR/Renamer.h"
#include "PetriEngine/PQL/Expressions.h"

#define LATTICE
#define NOCHANGE
#define NOKLEENE
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
        
        
        std::pair<bool, size_t> TARReachabilitySearch::stateForPredicate(int type, z3::expr pred, z3::context& context, size_t sim_hint, size_t simed_hint)
        {
            Renamer renamer(context);
            z3::expr predicate = renamer.rename(pred);

            if(Z3_get_bool_value(context, predicate.operator Z3_ast()) == Z3_L_TRUE) 
            {
                return std::make_pair(false, 1);
            }
            else if (Z3_get_bool_value(context, predicate.operator Z3_ast()) == Z3_L_FALSE)
            {        
                return std::make_pair(false, 0);   
            } 

            auto& fres = intmap[pred.hash()];
            for(el_t& r : fres)
            {
                if(r.expr.operator Z3_ast() == pred.operator Z3_ast())
                {
                    return std::make_pair(false, r.state);
                }
            }

            size_t astate = 0;
            bool found = false;


            if(!found)
            {
                auto& res = intmap[predicate.hash()];
                for(el_t& r : res)
                {
                    if(r.expr.operator Z3_ast() == predicate.operator Z3_ast())
                    {
                        found = true;
                        astate = r.state;
                        break;
                    }           
                }
                if(!found)
                {
                    z3::solver solver(context, "QF_LRA");
                    for(size_t hint : {sim_hint, simed_hint})
                    {
                        if(hint > 1)
                        {
                            solver.reset();
                            solver.add(
                                    (predicate      && (!states[hint].interpolant)) ||
                                    ((!predicate)   && states[hint].interpolant)
                                    );
                            if(solver.check() == z3::unsat)
                            {
                                res.emplace_back(predicate, hint);
                                states[hint].type = states[hint].type | type;
                                fres.emplace_back(pred, astate);
                                return std::make_pair(true, hint);
                            }
                        }
                    }

                    astate = states.size();
                    states.emplace_back(predicate);
                    states[astate].restricts = renamer.variables_seen();
                    states[astate].type = type;
                    astate = computeSimulation(astate, sim_hint, simed_hint);
                    res.emplace_back(predicate, astate);
                }
            }

            clock_t end = clock();
            fres.emplace_back(pred, astate);
            return std::make_pair(!found, astate);
        }

        
        int TARReachabilitySearch::constructAutomata(int from, waiting_t& trace, z3::expr_vector& inter, z3::context& context)
        {
            std::vector<std::pair<bool, size_t>> interpolant;
            bool changed = false;
            size_t astate = 1;
            size_t i = 0;
            for(size_t j = 0; j < inter.size(); ++j)
            {
                if(j > 0 && inter[j].operator Z3_ast() == inter[j - 1].operator Z3_ast())
                {
                    interpolant.emplace_back(false, interpolant[j - 1].second);
                }
                else
                {
                    interpolant.push_back(stateForPredicate(8, inter[j], context));
                    changed = changed || interpolant.back().second;
                }

                if(astate == 1)
                {
                    assert(j == 0 || i == j - 1);
                    i = j;
                    astate = interpolant.back().second;
                }
            }

            if((size_t)from == trace.size()) return from;

            if(from > 0 || i > 0)
            {
                const int tpos = from + i;
                
                auto res = states[1].add_edge(trace[tpos - 1].get_edge_cnt(), astate);
                changed = res || changed;
            }

            if(astate == 0)
            {
                if(from == 0)
                {
                    return std::min(i + from, trace.size() - 1);
                }
                else
                {
                    states[1].add_edge(
                            trace[(from + i) - 1].get_edge_cnt(), 0);
                    assert(i + from <= trace.size());
                    return (from + i) - 1;
                }
            }

            for(; i < interpolant.size(); ++i)
            {
                int tpos = from + i;

                if(tpos < (int)trace.size()) trace[tpos].add_interpolant(astate);


                assert(i + 1 < interpolant.size());
                size_t next = interpolant[i + 1].second;         
                if(next == 0)
                {

                    bool added = states[astate].add_edge(trace[tpos].get_edge_cnt(), 0);
                    changed = changed || added;
                    assert(i + from < trace.size());
                    break;
                } 
                
                if(next != 1) {
                    auto res = states[astate].add_edge(trace[tpos].get_edge_cnt(), next);
                    changed = changed || res;
                }
                else
                {
                    assert(false);
                }

                astate = next;          
            }   

            assert(changed || from > 0);
            return i + from;
        }
        
        
        bool TARReachabilitySearch::findValidRange( int& from, 
                                            const int to, 
                                            z3::context& context, 
                                            z3::expr_vector& interpolant, 
                                            std::vector<z3::expr>& encoded)
        {
            bool range_valid = true;
            auto cons = context.bool_val(true);
            z3::model model(context, NULL);
            for(size_t i = (size_t)from; i < (size_t)to; ++i)
            {
                cons = z3::interpolant(cons && encoded[i]);
            }


            z3::params parameters(context);
            auto result = context.compute_interpolant(cons, parameters, interpolant, model);
            switch (result) {
                case z3::unsat: 
                {
                    range_valid = false;
                    break;
                }
                case z3::unknown: assert(from != 0); // TODO this is a problem when from = 0!
                default:
                    break;
            }
            return range_valid; 
        }

        z3::expr TARReachabilitySearch::computeParameters(
                z3::context& context, 
                std::vector<z3::expr>& encoded, 
                z3::expr& param_reach, const std::vector<uint32_t>& used, const std::vector<bool>& inq, const std::vector<bool>& read)
        {
            z3::expr cons = context.bool_val(true);
            for(size_t i = 0; i < encoded.size(); ++i)
            {
                cons = cons && encoded[i];
            }


            z3::expr_vector names(context);
            for(size_t i = 0; i < used.size(); ++i)
            {
                if(used[i] > 0)
                {
                    for(size_t n = 0; n <= used[i]; ++n)
                    {
                        string name = to_string(i) + "~i" + to_string(n);
                        names.push_back(context.int_const(name.c_str()));
                    }
                }
                else if(inq[i] || read[i])
                {
                    string name = to_string(i) + "~i0";
                    names.push_back(context.int_const(name.c_str()));                    
                }
            }
            
            for(size_t i = 0; i < encoded.size(); ++i)
            {
                std::string mname = "m~i" + to_string(i);                
                names.push_back(context.int_const(mname.c_str()));
            }


            z3::expr qf = z3::exists(names, cons);
            z3::tactic qe(context, "qe2");
            z3::tactic eqs(context, "solve-eqs");
            z3::tactic simplify(context, "ctx-solver-simplify");
            z3::goal g(context);
            g.add(qf);
            z3::apply_result res = qe.apply(g);
            auto result = context.bool_val(false);
            for(size_t i = 0; i < res.size(); ++i)
            {
                z3::goal g2 = res[i];
                if(g2.size() == 0)
                {
                    return context.bool_val(true);
                }
                else
                {
                    result = result || g2.as_expr();
                }
            }

            {
                z3::goal g2(context);
                g2.add(result);        
                z3::apply_result res = (eqs & simplify & eqs).apply(g2);
                result =  res[0].as_expr().simplify();    
            }
            return result;
        }

        
        std::pair<int,bool>  TARReachabilitySearch::isValidTrace(waiting_t& trace, z3::context& context, bool probe, Structures::State& initial, z3::expr& query, const std::vector<bool>& inq, z3::expr& param)
        {

            std::vector<z3::expr> encoded = {context.bool_val(true)};
            std::vector<uint32_t> uses(_net.numberOfPlaces(), 0);
            std::vector<bool> in_inhib(uses.size(), false);
            size_t m = 0;
            for(auto& t : trace)
            {
                ++m;
                if(t.get_edge_cnt() == 0) 
                {
                    continue;
                }
                auto pre = _net.preset(t.get_edge_cnt() - 1);
                std::string mname = "m~i" + to_string(m);                
                auto mult = context.int_const(mname.c_str());
                auto begin = mult >= 1;
                bool has_inhib = false;
                for(;pre.first != pre.second; ++pre.first)
                {
                    string name = std::to_string(pre.first->place) + "~i" + std::to_string(uses[pre.first->place]);
                    auto ppre = context.int_const(name.c_str());
                    if(pre.first->inhibitor)
                    {
                        in_inhib[pre.first->place] = true;
                        begin = begin && (ppre < context.int_val(pre.first->tokens));
                        has_inhib = true;
                    }
                    else
                    {
                        ++uses[pre.first->place];
                        string nextname = to_string(pre.first->place) + "~i" + to_string(uses[pre.first->place]);
                        auto ppost = context.int_const(nextname.c_str());
                        
                        begin = begin && ppre >= (context.int_val(pre.first->tokens) * mult) ;
                        begin = begin && (ppost == (ppre - (mult * context.int_val(pre.first->tokens))));
                    }
                }
                auto post = _net.postset(t.get_edge_cnt() - 1);

                for(; post.first != post.second; ++post.first)
                {
                   
                    string name = to_string(post.first->place) + "~i" + to_string(uses[post.first->place]);
                    ++uses[post.first->place];
                    string nextname = to_string(post.first->place) + "~i" + to_string(uses[post.first->place]);
                    auto ppre = context.int_const(name.c_str());
                    auto ppost = context.int_const(nextname.c_str());
                    begin = begin && ppost >= (mult*context.int_val(post.first->tokens));
                    begin = begin && ppost == (ppre + (mult*context.int_val(post.first->tokens)));
                    
                }
                
                if(has_inhib)
                    begin = begin && mult == 1;
                if(_kbound > 0)
                {
                    auto sum = context.int_val(0);
                    for(size_t i = 0; i < uses.size(); ++i)
                    {
                        string name = to_string(i) + "~i" + to_string(uses[i]);
                        sum = sum + context.int_const(name.c_str());
                        in_inhib[i] = true;
                    }
                    begin = begin && (sum <= context.int_val(_kbound));
                }
                encoded.push_back(begin);
            }
            
            
            /*auto q = query;
            for(size_t i = 0; i < _net.numberOfPlaces(); ++i)
            {
                if(inq[i])
                {
                    string qname = to_string(i) + "~i" + to_string(numeric_limits<int32_t>::max());
                    string pname = to_string(i) + "~i" + to_string(uses[i]);
                    q = q && (context.int_const(qname.c_str()) == context.int_const(pname.c_str()));                    
                }
            }*/
            Renamer ren(context);
            encoded.push_back(ren.rename(query, uses.data()));

            if(ren.has_param())
            {
                encoded[0] = encoded[0] && !param;
            }
                    
            for(size_t i = 0; i < uses.size(); ++i)
            {
                if(uses[i] > 0 || in_inhib[i] || inq[i])
                {
                    string name = to_string(i) + "~i0";
                    encoded[0] = encoded[0] && (context.int_const(name.c_str()) == context.int_val(initial.marking()[i]));
                }
            }
            const int to = encoded.size();
            int from = 0;
            int nvalid = to;
            bool valid = true;
            while(from < to)
            {
                z3::expr_vector interpolant(context); 
                bool res = findValidRange(from, to, context, interpolant, encoded);
                if(res) 
                {
                    if(ren.has_param() && from == 0)
                    {
                        
                        /*
                                         z3::context& context, 
                std::vector<z3::expr>& encoded, 
                z3::expr& param_reach, std::vector<uint32_t>& used, std::vector<bool>& inq, std::vector<bool>& read, waiting_t& trace)
                         */
                        param = computeParameters(context, encoded, param, uses, inq, in_inhib);
                        encoded[0] = encoded[0] && !param;
                        continue;
                        assert(false);
                    }
                    return std::pair<int,bool>(nvalid, from == 0);
                }
                else
                {
                    valid = false;
                }

                if(interpolant.size() == 0)
                {
                    trace.clear();
                    return std::pair<int,bool>(-1,false);
                }
                
                from = constructAutomata(from, trace, interpolant, context);
                
                assert(!valid);
                from += 1;
                nvalid = std::min(nvalid, from);
#ifdef NOKLEENE
                break;
#endif
            }

            return std::pair<int,bool>(nvalid,valid);
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
                if(checkInclussion(waiting[i - 1], waiting[i].get_interpolants(), states[0].interpolant.ctx()))
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

        size_t TARReachabilitySearch::computeSimulation(size_t index, size_t sim_hint, size_t simed_hint)
        {
#ifndef LATTICE
            return index;
#else
            AutomataState& state = states[index];
            z3::context& ctx = state.interpolant.ctx();
            z3::solver solver(ctx);
            if(index != sim_hint)
            {
                auto lb = std::lower_bound(state.simulates.begin(), state.simulates.end(), sim_hint);
                if(lb == state.simulates.end() || *lb != sim_hint)
                {
                    state.simulates.insert(lb, sim_hint);
                    inline_union(state.simulates, states[sim_hint].simulates);
                }
            }

            if(index != simed_hint)
            {
                auto lb = std::lower_bound(state.simulators.begin(), state.simulators.end(), simed_hint);
                if(lb == state.simulators.end() || *lb != simed_hint)
                {
                    state.simulators.insert(lb, simed_hint);
                    inline_union(state.simulators, states[simed_hint].simulators);
                }
            }

            for(size_t i = 2; i < states.size(); ++i)
            {
                AutomataState& other = states[i];
                if(i == index) continue;
                auto lb = std::lower_bound(state.simulates.begin(), state.simulates.end(), i);
                if(lb != state.simulates.end() && *lb == i && i != sim_hint) continue;
                auto lb2 = std::lower_bound(state.simulators.begin(), state.simulators.end(), i);
                if(lb2 != state.simulators.end() && *lb2 == i && i != simed_hint) continue;
                bool same = true;
                if(
                        state.restricts.size() >= other.restricts.size() &&
                        std::includes(  state.restricts.begin(), state.restricts.end(),
                                    other.restricts.begin(), other.restricts.end()))
                {
                    solver.reset();
                    solver.add(state.interpolant && (!other.interpolant));
                    auto r = solver.check();
                    if(r == z3::unsat)
                    {
                        // state simulates other (the proof used in state is stronger)
                        if(lb == state.simulates.end() || *lb != i) state.simulates.insert(lb, i);
                        inline_union(state.simulates, other.simulates);
                    }
                    else
                    {
                        same = false;
                    }
                }
                else
                {
                    same = false;
                }
                if( 
                    state.restricts.size() <= other.restricts.size() &&
                    std::includes(  other.restricts.begin(), other.restricts.end(),
                                    state.restricts.begin(), state.restricts.end()))
                {
                    solver.reset();
                    solver.add((!state.interpolant) && other.interpolant);
                    auto r = solver.check();
                    if(r == z3::unsat)
                    {
                        if(!same)
                        {
                            // state is simulated by states[i] (the proof used in state is weaker)
                            if(lb2 == state.simulators.end() || *lb2 != i) state.simulators.insert(lb2, i);
                            inline_union(state.simulators, other.simulators);
                        }
                    }
                    else
                    {
                        same = false;
                    }
                    if(same)
                    {
                        other.type = other.type | state.type;
                        states.erase(states.begin() + index);
                        return i;
                    }
                }
            }

            for(size_t s : state.simulates)
            {
                if(states[s].simulators.size() == 0 || states[s].simulators.back() != index)
                    states[s].simulators.push_back(index);
            }

            for(size_t s : state.simulators)
            {
                if(states[s].simulates.size() == 0 || states[s].simulates.back() != index)
                    states[s].simulates.push_back(index);
            }
            assert(states[1].simulates.size() == 0);
            assert(states[0].simulators.size() == 0);    
            return index;
        #endif
        }
        
        
        bool TARReachabilitySearch::tryReach(   const std::shared_ptr<PQL::Condition> & query, 
                                        std::vector<ResultPrinter::Result>& results,
                                        bool printstats, bool printtrace, Structures::State& initial)
        {

            
            // Construct our constraints
            z3::config config;
            z3::context solver_context(config, z3::context::interpolation());

            states.emplace_back(solver_context.bool_val(false)); // false
            states.emplace_back(solver_context.bool_val(true)); // true
            computeSimulation(0);

#ifdef LATTICE
            assert(states[1].simulates.size() == 0);
            assert(states[1].simulators.size() == 1);
            assert(states[0].simulates.size() == 1);
            assert(states[0].simulators.size() == 0);

            assert(states[1].simulators[0] == 0);
            assert(states[0].simulates[0] == 1);
#endif
    
            intmap[solver_context.bool_val(false).hash()].emplace_back(solver_context.bool_val(false), 0);
            intmap[solver_context.bool_val(true).hash()].emplace_back(solver_context.bool_val(true), 1);

            std::vector<bool> in_query(_net.numberOfPlaces(), false);
            std::vector<int32_t> uses(_net.numberOfPlaces(), 0);
            auto param = solver_context.bool_val(false);//solver_context.int_val(0) <= solver_context.int_const("~b");
            std::cout << param << std::endl;
            auto sat_query = query->encodeSat(_net, solver_context, uses, in_query);
            { // try to simplify the proposition more!
                z3::tactic simplify(solver_context, "ctx-solver-simplify");
                z3::goal g(solver_context);
                g.add(sat_query);
                z3::apply_result res = simplify.apply(g);
                z3::expr result = solver_context.bool_val(false);
               
                for(size_t i = 0; i < res.size(); ++i)
                {
                    z3::goal g2 = res[i];
                    if(g2.size() == 0)
                    {
                        return false;
                    }
                    else
                    {
                        result = result || g2.as_expr();
                    }
                }
                Renamer renamer(solver_context);
                renamer.contains(result);
                in_query = std::vector<bool>(_net.numberOfPlaces(), false);
                for(auto i : renamer.variables_seen())
                    in_query[i] = true;
                sat_query = result;
            }
            
            /* Do the verification */

            bool all_covered = true;
            size_t stepno = 0;

            std::vector<size_t> initial_interpols;
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
                    if(checkInclussion(state, nextinter, solver_context)) // Check if the next state makes the interpolant automata accept.
                    {
                        state.next_edge(_net);
                        continue;
                    }

                    bool next_edge = false;
                    if(waiting.back().get_edge_cnt() == 0) // check if proposition is satisfied
                    {
                        std::pair<int,bool> res = isValidTrace(waiting, solver_context, false, initial, sat_query, in_query, param);
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
                            assert(waiting.size() > 0);
                            initial_interpols = waiting[0].get_interpolants();
                            continue;
                        }
                    }
                    else
                    {
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
            if(auto upper = dynamic_cast<PQL::UpperBoundsCondition*>(query.get()))
            {
                auto str = param.to_string();
                size_t bound = 0;
                if(str.compare("false") != 0)
                {
                    str[str.size() - 4] = 0;
                    sscanf(&str[4], "%zu", &bound);
                }
                upper->setUpperBound(bound);
            }
            return false;            
        }

        bool TARReachabilitySearch::checkInclussion(state_t& state, std::vector<size_t>& nextinter, z3::context& ctx)
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

                if(as.restricts.size() == 0)
                {
                    auto lb = std::lower_bound(nextinter.begin(), nextinter.end(), i);
                    if(lb == nextinter.end() || *lb != i) nextinter.insert(lb, i);
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
            std::vector<size_t> writes;

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
    
                    bool ok = true;
                    auto iw = writes.begin(); 
                    auto ic = as.restricts.begin();
                    while(iw != writes.end() && ic != as.restricts.end())
                    {
                        if(*ic < *iw)
                        {
                            ++ic;
                        } else if(*ic > *iw)
                        {
                            ++iw;
                        }
                        else if(*ic == *iw) 
                        {
                            ok = false;
                            break;
                        }
                    }
                    if(ok) 
                    {
                        nextinter.insert(next, i);
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
                std::cerr << "\t<transition id=\"" << tname << "\">\n";
                
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
                                        bool printstats, bool printtrace)
        {

            // set up working area
            Structures::State state;
            state.setMarking(_net.makeInitialMarking());
            
            // check initial marking
            if(checkQueries(queries, results, state, true))
            {
                if(printstats) printStats();
                    return;
            }
            
            // Search!
            for(size_t i = 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    bool res = tryReach(queries[i], results, printstats, printtrace, state);
                    if(res)
                        results[i] = ResultPrinter::Satisfied;
                    else
                        results[i] = ResultPrinter::NotSatisfied;
                    results[i] = printQuery(queries[i], i, results[i]);  
                }
            }

            if(printstats) printStats();
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

