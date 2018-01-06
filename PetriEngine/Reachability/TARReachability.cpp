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

//#define CLOSURE
#define LATTICE
//#define NOCHANGE
//#define POST
//#define PRE
//#define GENERIC
//#define GENERICPRE
//#define NOKLEENE
#define ANTISIM
//#define PROBING
//#define REMOVEEDGES
//#define RESET

namespace PetriEngine {
    using namespace PQL;
    namespace Reachability {

        
        std::pair<bool, size_t> TARReachabilitySearch::stateForPredicate(int type, z3::expr pred, z3::context& context, size_t sim_hint, size_t simed_hint)
        {
        //    sim_hint = 1;
        //    simed_hint = 0;
            clock_t begin = clock();
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
            /*else if(renamer.variables_seen().size() == 0)
            {
                return std::make_pair(false, 1);
            }*/

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
            //            std::cout << "MATCH\n" << r.expr << "\n #<=>#\n" <<  predicate  << std::endl;
                        found = true;
                        astate = r.state;
                        break;
                    }           
            /*            else
                    {
                        std::cout << r.expr << "\n" << target << std::endl;
                        std::cout << "NOMATCH" << std::endl;
                    }*/
                }
                if(!found)
                {
        //        std::cout << "INSERTING " << predicate << std::endl;
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
            //                    std::cout << "UNSEEN" << std::endl;
            //                    std::cout << opred << std::endl;
            //                    std::cout << predicate << std::endl;
            //                    std::cout << states[hint].interpolant << std::endl;
                                //                    sanity();
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
            //        sanity(context, trace, interpolant);
                    astate = computeSimulation(astate, sim_hint, simed_hint);
            //        sanity(context, trace, interpolant);
            //        sanity();
                    res.emplace_back(predicate, astate);
            //        std::cout << "NEW" << std::endl;
            //        std::cout << opred << std::endl;
            //        std::cout << predicate << std::endl;
            //        sanity(context, trace, interpolant);
                }
            }

            clock_t end = clock();
            /*if(type == 8)
            {
                tSimNormal += double(end - begin)/ CLOCKS_PER_SEC;
            }
            else if(type == 2)
            {
                tSimPost += double(end - begin)/ CLOCKS_PER_SEC;
            } 
            else if(type == 16)
            {
                tSimPre += double(end - begin)/ CLOCKS_PER_SEC;        
            }*/
            fres.emplace_back(pred, astate);
        //    sanity();
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

                /*
                std::cout   << "[" << j << "]" << inter[j] << " ==> "
                    << interpolant.back().second << " --> "
                    << states[interpolant.back().second].interpolant << std::endl;
        */

                if(astate == 1)
                {
                    assert(j == 0 || i == j - 1);
                    i = j;
                    astate = interpolant.back().second;
                }

              //  if(j < trace.size()) std::cout << trace[j].location << " ::::> " << trace[j].get_edge_cnt() << std::endl;

            }

        //    std::cout << "CONSTRUCT" << std::endl;
        //    std::cout << inter << std::endl;
        //    printTrace(trace);
        //    printInterpolants(trace[0].get_interpolants());
            if((size_t)from == trace.size()) return from;

            if(from > 0 || i > 0)
            {
                const int tpos = from + i;
                
        #ifdef GENERIC
                generatePost(context, encoder, loopstate, state, param_reach, 
                            context.bool_val(true), 1, 0, trace[tpos - 1].get_edge_cnt(), &trace[tpos - 1]);
        #endif
        #ifdef GENERICPRE
                generatePre(context, encoder, loopstate, state, param_reach, 
                        states[interpolant[i].second].interpolant, astate, 0, trace[tpos - 1].get_edge_cnt(), &trace[tpos - 1]);
        #endif
                auto res = states[1].add_edge(trace[tpos - 1].get_edge_cnt(), astate);
                changed = res || changed;
            }

            if(astate == 0)
            {
                if(from == 0)
                {
                    assert(i + from < trace.size());
                    return i + from;
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

                if(tpos < trace.size()) trace[tpos].add_interpolant(astate);


                assert(i + 1 < interpolant.size());
                size_t next = interpolant[i + 1].second;

        #ifdef PRE
        //        if(next != astate)
                {
                    generatePre(context, encoder, loopstate, state, param_reach, 
                        context.bool_val(false), 0, loc, trace[tpos].get_edge_cnt(), &trace[tpos]);
                }
        #endif


        #ifdef GENERICPRE        
                {
                   generatePre(context, encoder, loopstate, state, param_reach, 
                            context.bool_val(false), 0, 0, trace[tpos].get_edge_cnt(), &trace[tpos]);
                }
        #endif           
                if(next == 0)
                {

                    bool added = states[astate].add_edge(trace[tpos].get_edge_cnt(), 0);
                    changed = changed || added;
                    assert(i + from < trace.size());
                    break;
                }



                if(true //astate != next
        #ifdef POST
                        && generatePost(context, encoder, loopstate, state, param_reach, 
                            context.bool_val(true), 1, loc, trace[tpos].get_edge_cnt(), &trace[tpos]))
        #else
                    )
        #endif

                {
        #ifdef GENERIC
                    generatePost(context, encoder, loopstate, state, param_reach, 
                        context.bool_val(true), 1, 0, trace[tpos].get_edge_cnt(), &trace[tpos]);
        #endif
                }

                if(next != 1) {

                    // construct POST -- but only if we have not constructed it before!
                    if( true //next != astate
        #ifdef POST
                         && generatePost(context, encoder, loopstate, state, param_reach, 
                        states[astate].interpolant, astate, loc, trace[tpos].get_edge_cnt(), &trace[tpos])
        #endif
                    )

                    {

        #ifdef GENERIC
                        generatePost(context, encoder, loopstate, state, param_reach, 
                                            states[astate].interpolant, astate, 0, trace[tpos].get_edge_cnt(), &trace[tpos]);
        #endif
                    }

                    if(astate != 1)
                    {

                        if(true //next != astate
        #ifdef PRE
                            && generatePre(context, encoder, loopstate, state, param_reach, 
                            states[next].interpolant, next, loc, trace[tpos].get_edge_cnt(), &trace[tpos])
        #endif
                                )

                        {
        #ifdef GENERICPRE
                            generatePre(context, encoder, loopstate, state, param_reach, 
                                    states[next].interpolant, next, 0, trace[tpos].get_edge_cnt(), &trace[tpos]);
        #endif
                        }

                    }

                    {
                        auto res = states[astate].add_edge(trace[tpos].get_edge_cnt(), next);
                        changed = changed || res;
                    }
                }
                else
                {
//                    std::cout << inter << std::endl;
//                    std::cout << inter[i] << std::endl;
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
/*            std::cout << "CONSTRAINT" << std::endl;
            std::cout << cons << std::endl;
            std::cout << "INTERPOLANTS " << std::endl;
            std::cout << interpolant << std::endl;
            std::cout << "DONE" << std::endl;*/
            return range_valid; 
        }
        
        std::pair<int,bool>  TARReachabilitySearch::isValidTrace(waiting_t& trace, z3::context& context, bool probe, Structures::State& initial, const PQL::Condition* condition)
        {

//            std::cout << "IS VALID " << std::endl;

            std::vector<z3::expr> encoded = {context.bool_val(true)};
            std::vector<int32_t> uses(_net.numberOfPlaces(), 0);
            for(auto& t : trace)
            {

                if(t.get_edge_cnt() == 0) 
                {
                    continue;
                }
//                std::cout << _net.transitionNames()[t.get_edge_cnt() - 1] << std::endl;

                auto begin = context.bool_val(true);
                auto pre = _net.preset(t.get_edge_cnt() - 1);
                for(;pre.first != pre.second; ++pre.first)
                {
                    string name = std::to_string(pre.first->place) + "~i" + std::to_string(uses[pre.first->place]);
                    ++uses[pre.first->place];
                    string nextname = to_string(pre.first->place) + "~i" + to_string(uses[pre.first->place]);
                    begin = begin && context.int_const(nextname.c_str()) >= context.int_val(0);
                    if(pre.first->inhibitor)
                    {
                        begin = begin && (context.int_const(name.c_str()) < context.int_val(pre.first->tokens));
                        begin = begin && (context.int_const(nextname.c_str()) == context.int_const(name.c_str()) );
                    }
                    else
                    {
                        begin = begin && (context.int_const(name.c_str()) >= context.int_val(pre.first->tokens));
                        begin = begin && (context.int_const(nextname.c_str()) == (context.int_const(name.c_str()) - context.int_val(pre.first->tokens)));
                    }
                }
                auto post = _net.postset(t.get_edge_cnt() - 1);
                for(; post.first != post.second; ++post.first)
                {
                    string name = to_string(pre.first->place) + "~i" + to_string(uses[post.first->place]);
                    ++uses[pre.first->place];
                    string nextname = to_string(pre.first->place) + "~i" + to_string(uses[post.first->place]);
                    begin = begin && context.int_const(name.c_str()) >= context.int_val(0);
                    begin = begin && context.int_const(nextname.c_str()) == (context.int_const(name.c_str()) + context.int_val(post.first->tokens));
                }
                encoded.push_back(begin);
            }
            std::vector<bool> incremented(uses.size(), false);
            encoded.push_back(condition->encodeSat(context, uses, incremented));
                        
            for(size_t i = 0; i < uses.size(); ++i)
            {
                if(uses[i] > 0)
                {
                    string name = to_string(i) + "~i0";
                    encoded[0] = encoded[0] && (context.int_const(name.c_str()) == context.int_val(initial.marking()[i]));
                }
            }
            
/*            for(auto& e : encoded)
            {
                std::cout << e << std::endl;
            }*/
            
            const int to = encoded.size();
            int from = 0;
            int nvalid = to;
            bool valid = true;
            while(from < to)
            {
                z3::expr_vector interpolant(context); 
                clock_t begin = clock();
                bool res = findValidRange(from, to, context, interpolant, encoded);
                clock_t end = clock();
                //tvalidRange += double(end - begin)/ CLOCKS_PER_SEC;
                if(res) 
                {
                    return std::pair<int,bool>(nvalid, from > 0);
                }
                else
                {
                    valid = false;
                }


                //    std::cout << interpolant.size() << std::endl;
                if(interpolant.size() == 0)
                {
                    trace.clear();
                    return std::pair<int,bool>(-1,false);
                }
        /*        std::cerr << "TRACE" << std::endl;
                printTrace(trace);*/ 
/*                std::cerr << "INTERPOLANT" << std::endl;
                std::cerr << interpolant << std::endl;
                std::cerr << "DONE" << std::endl;*/
         
                {
                    clock_t begin = clock();
                    from = constructAutomata(from, trace, interpolant, context);
                    clock_t end = clock();
                    //tconstructAutomata += double(end - begin)/ CLOCKS_PER_SEC;
                }
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
                    waiting.resize(i - 1);
                    break;
                }
            }
            if(waiting.size() == 0) return;
            //sanity(waiting);
            waiting.back().reset_edges(_net);
//            sanity(waiting);
        }
        
        
        bool TARReachabilitySearch::popDone(waiting_t& waiting, size_t& stepno)
        {
            bool popped = false;
            while(waiting.back().get_edge_cnt() > _net.numberOfTransitions()) // we have tried all transitions for this state-pair!
            {
//                ++stepno;
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
        //    clock_t end = clock();
        //    tSim += double(end - begin)/ CLOCKS_PER_SEC;
            return index;
        #else
            clock_t begin = clock();


            AutomataState& state = states[index];
            z3::context& ctx = state.interpolant.ctx();
            z3::solver solver(ctx, "QF_LRA");
        //    std::cout << "SIM FOR : " << state.interpolant << std::endl;
            if(index != sim_hint)
            {
                auto lb = std::lower_bound(state.simulates.begin(), state.simulates.end(), sim_hint);
                if(lb == state.simulates.end() || *lb != sim_hint)
                {
                    state.simulates.insert(lb, sim_hint);
                    std::vector<size_t> b2;
                    std::set_union( state.simulates.begin(), state.simulates.end(), 
                                    states[sim_hint].simulates.begin(), states[sim_hint].simulates.end(),
                                    std::back_inserter(b2));
                    state.simulates.swap(b2);
                }
            }

            if(index != simed_hint)
            {
                auto lb = std::lower_bound(state.simulators.begin(), state.simulators.end(), simed_hint);
                if(lb == state.simulators.end() || *lb != simed_hint)
                {
                    state.simulators.insert(lb, simed_hint);
                    std::vector<size_t> b2;
                    std::set_union( state.simulators.begin(), state.simulators.end(), 
                                    states[simed_hint].simulators.begin(), states[simed_hint].simulators.end(),
                                    std::back_inserter(b2));
                    state.simulators.swap(b2);
                }
            }

            for(size_t i = 2; i < states.size(); ++i)
            {
                AutomataState& other = states[i];
        //        if((state.type & states[i].type) == 0) continue;
                if(i == index) continue;
                auto lb = std::lower_bound(state.simulates.begin(), state.simulates.end(), i);
                if(lb != state.simulates.end() && *lb == i && i != sim_hint) continue;
                auto lb2 = std::lower_bound(state.simulators.begin(), state.simulators.end(), i);
                if(lb2 != state.simulators.end() && *lb2 == i && i != simed_hint) continue;
                bool same = true;
                if(
        //            (state.type == 2 ||
        //            (other.type % state.type) != 0)
                    /*&&*/ (
//                        (!state.only_discrete || other.only_discrete) &&
                        state.restricts.size() >= other.restricts.size() &&
                        std::includes(  state.restricts.begin(), state.restricts.end(),
                                    other.restricts.begin(), other.restricts.end())))
                {
                    clock_t sbegin = clock();
                    solver.reset();
                    solver.add(state.interpolant && (!other.interpolant));
                    auto r = solver.check();
                    clock_t send = clock();
//                    tSims += double(send - sbegin)/ CLOCKS_PER_SEC;
                    if(r == z3::unsat)
                    {
                        // state simulates other (the proof used in state is stronger)
                        if(lb == state.simulates.end() || *lb != i) state.simulates.insert(lb, i);
                        std::vector<size_t> b2;
                        std::set_union( state.simulates.begin(), state.simulates.end(), 
                                        other.simulates.begin(), other.simulates.end(),
                                        std::back_inserter(b2));
                        state.simulates.swap(b2);
            //            assert(state.simulators.size() == 0 || state.simulators[0] != 0);
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
        //        else
                if( /*(    state.type == 16 ||
                        (state.type == 8  && ((other.type % (2 + 8)) != 0)) 
                        (state.type == 2  && ((other.type %  2)      != 0))
                    )
                        (state.type == 16 ||
                        (other.type % state.type) != 0)
                    && */(
//                    (!other.only_discrete || state.only_discrete) &&
                    state.restricts.size() <= other.restricts.size() &&
                    std::includes(  other.restricts.begin(), other.restricts.end(),
                                    state.restricts.begin(), state.restricts.end())))
                {
                    clock_t sbegin = clock();
                    solver.reset();
                    solver.add((!state.interpolant) && other.interpolant);
                    auto r = solver.check();
                    clock_t send = clock();
//                    tSimed += double(send - sbegin)/ CLOCKS_PER_SEC;
                    if(r == z3::unsat)
                    {
                        if(!same)
                        {
                            // state is simulated by states[i] (the proof used in state is weaker)
                            if(lb2 == state.simulators.end() || *lb2 != i) state.simulators.insert(lb2, i);

                            std::vector<size_t> b2;
                            std::set_union( state.simulators.begin(), state.simulators.end(), 
                                            other.simulators.begin(), other.simulators.end(),
                                            std::back_inserter(b2));
                            state.simulators.swap(b2);
                        }
        //                assert(state.simulators.size() == 0 || state.simulators[0] != 0);
                    }
                    else
                    {
                        same = false;
                    }
                    if(same)
                    {
                        other.type = other.type | state.type;
                        states.erase(states.begin() + index);
                        clock_t end = clock();
//                        tSim += double(end - begin)/ CLOCKS_PER_SEC;
        //                sanity();
                        return i;
                    }
        /*            else
                    {
                        // Proofs are incomparable.
                    }*/
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
            clock_t end = clock();
        //    tSim += double(end - begin)/ CLOCKS_PER_SEC;
        //    sanity();
            return index;
        #endif
        }
        
        
        bool TARReachabilitySearch::tryReach(   const std::shared_ptr<PQL::Condition> & query, 
                                        std::vector<ResultPrinter::Result>& results,
                                        bool printstats, Structures::State& initial)
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


            /* Explore unexplored states.
             */

    
    
            bool all_covered = true;
            size_t stepno = 0;

            std::vector<size_t> initial_interpols;
            initial_interpols.push_back(1);
            do {
                all_covered = true;
                auto checked = AntiChain<char, size_t>();
                char dummy = 0;
                // waiting-list with levels
                waiting_t waiting;
                // initialize
                {
                    state_t state;
                    //state.location = saveLocs(successor->state);
                    state.reset_edges(_net);
                    state.set_interpolants(initial_interpols);
                    waiting.push_back(state);
                }
                while (!waiting.empty()) 
                {
        //            sanity(waiting);
//                    if((stepno % 1000) == 0)
                    {
//                        cout << stepno << " : " << waiting.size() << " : " << waiting[0].get_interpolants().size() << " : " << states.size() << std::endl;
//                        printStats();
                    }

        /*            std::cout << "TRACE" << std::endl;
                    printTrace(waiting);
                    printInterpolants(waiting[0]);*/

                    /*if(kleeneCovered(waiting, symstate))
                    {
                        continue; // all traces from this state is covered by some kleene-automata
                    }*/

                    //loadLocs(waiting.back().location, symstate);  
                    {
                        clock_t b = clock();
                        bool r = popDone(waiting, /*symstate,*/ stepno);
                        //sanity(waiting);
                        clock_t e = clock();
                        //tpop += double(e-b)/CLOCKS_PER_SEC;
                        if(r)
                        {
                            continue;  // we have reached the end of the edge-iterator for this part of the trace
                        }                
                    }

                    ++stepno;

                    assert(waiting.size() > 0 );
                    state_t& state = waiting.back();
                    std::vector<size_t> nextinter;
                    {
                        clock_t b = clock();
        //                std::cout << "ITT " << stepno << std::endl;
                        bool r = checkInclussion(state, nextinter, solver_context);
                        clock_t e = clock();
    //                    tinccheck += double(e-b)/CLOCKS_PER_SEC;
                        if(r) // Check if the next state makes the interpolant automata accept.
                        {
                            state.next_edge(_net);
                            //std::cout << "NEXT EDGE " << state.get_edge_cnt() << std::endl;
                            //sanity(waiting);
                            continue;
                        }
                    }

                    bool next_edge = false;
                    if(waiting.size() > 1 && waiting.back().get_edge_cnt() == 0) // check if proposition is satisfied
                    {
        //                sanity(waiting);
        //                ++validTraces;
                        clock_t begin = clock();
                        std::pair<int,bool> res = isValidTrace(waiting, solver_context, false, initial, query.get()); 
                        initial_interpols = waiting[0].get_interpolants();
                        clock_t end = clock();
        //                tcheckTrace += double(end - begin) / CLOCKS_PER_SEC;
                        if(res.second)
                        {
                            {
        /*                        printTrace(waiting); 
                                std::cerr << "VALID TRACE FOUND!" << std::endl;
                                std::cerr << "STEPS : " << stepno << std::endl;
                                std::cerr << "INTERPOLANT AUTOMATAS : " << waiting[0].get_interpolants().size() << std::endl;
                                sink->tryPut(successor);
                                printStats();
        //                        printInterpolants(initial_interpols);*/
                                intmap.clear();
                                states.clear();
                                return true;
                            }
                        }
                        else
                        {
                            handleInvalidTrace(waiting, res.first);
                            all_covered = false;
        //                    sanity(waiting);
                            initial_interpols = waiting[0].get_interpolants();
        #ifdef ROBUSTNESS
        #ifndef RESET
                            continue;
        #endif
                            if(param_reach.operator Z3_ast() != old.operator Z3_ast())
                            {
                                break;
                            }
                            else
                            {
                                continue;
                            }
        #else
                            continue;
        #endif
                        }
                    }
                    else
                    {
                        next_edge = true;
                    }

                    if(next_edge)
                    {
    //                    sanity(waiting);
                        state_t next;
                        clock_t b = clock();
                        size_t tmp = 0; 
                        bool res = checked.subsumed(dummy, nextinter);
                        clock_t e = clock();
    //                    tantiChain += double(e-b)/CLOCKS_PER_SEC;
                        if(res)
                        {
                            waiting.back().next_edge(_net);
                        }
                        else
                        {
                            clock_t b = clock();                    
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
                            //std::cout << "VS " << minimal.size() << " <= " << waiting.back().get_interpolants().size() << std::endl;
                            checked.insert(dummy, minimal);
        //                    assert(inserted);
                            clock_t e = clock();
    //                        tantiInsert += double(e-b)/CLOCKS_PER_SEC;
                            next.reset_edges(_net);
                            next.set_interpolants(minimal);  
                            waiting.push_back(next);
    //                        sanity(waiting);
                        }
                    }
                }
//            ++oloops;
            } while(!all_covered);

            //std::cout << "STEPS : " << stepno << std::endl;
            intmap.clear();
            states.clear();
            //    std::cout << "STEPS : " << stepno << std::endl;
        //    std::cout << "INTERPOLANT AUTOMATAS : " << waiting[0].get_interpolants().size() << std::endl;
            return false;            
        }
        
        bool TARReachabilitySearch::checkInclussion(state_t& state, std::vector<size_t>& nextinter, z3::context& ctx)
        {
         //   Encoder enc(ctx, *system.get());
        //    enc.reset();
        //    z3::expr dv = enc.delay_var();
        //    z3::expr edge = encodeEdges(ctx, enc, state.get_edge_cnt(), symstate);

            if(nextinter.size() == 0 || nextinter[0] != 1) nextinter.insert(nextinter.begin(), 1);
        //    Renamer rn(ctx);
        //    z3::solver solver(ctx, "QF_LRA");
            std::vector<size_t> maximal = state.get_interpolants();
        #ifdef ANTISIM
            if(maximal.size() == 0 || maximal[0] != 1) maximal.insert(maximal.begin(), 1);

            assert(maximal.size() == 0 || maximal[0] != 0);
            assert(is_sorted(maximal.begin(), maximal.end()));
            for(size_t i : state.get_interpolants())
            {
                std::vector<size_t> buffer;
                std::set_union(states[i].simulates.begin(), states[i].simulates.end(),
                                maximal.begin(), maximal.end(), std::back_inserter(buffer));
                maximal.swap(buffer);
                assert(maximal.size() == 0 || maximal[0] != 0);
            }
        #endif

/*            std::cout << "FROM INTER IS ";
            for(size_t j : state.get_interpolants()) std::cout << j << ", ";
            std::cout << std::endl;
            for(size_t j : maximal) std::cout << j << ", ";
            std::cout << std::endl;*/

            assert(is_sorted(maximal.begin(), maximal.end()));
            for(size_t i : maximal)
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

                /*std::cout << "FROM " << &state << std::endl;
                std::cout << "#####LOOKING FOR " << state.get_edge_cnt() << " -->   " << std::endl;
                auto it = as.get_edges().begin();
                while(it != as.get_edges().end()) {
                    std::cout << "SKIP " << std::endl;
                    std::cout << it->edge << std::endl;
                    ++it;
                }*/
                {
                    auto it = as.first_edge(state.get_edge_cnt());
                    //std::cout << (cit - as.get_edges().begin()) << " VS " << (it - as.get_edges().begin()) << std::endl;
                    //assert(cit == as.get_edges().end() || it == cit);
                    while(it != as.get_edges().end())
                    {
                        if(it->edge != state.get_edge_cnt())    { break; }
            //            std::cout << "OK!" << std::endl;
                        AutomataEdge& ae = *it;
                        ++it;
                        assert(ae.to.size() > 0);
                        auto other = nextinter.begin();
                        auto next = ae.to.begin();
                        if(*next == 0) return true;
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
                                    std::vector<size_t> buffer;
                                    std::set_union( nextinter.begin(), nextinter.end(),
                                                    states[*next].simulates.begin(), states[*next].simulates.end(),
                                                    std::back_inserter(buffer));
                                    nextinter.swap(buffer);
                                    other = std::lower_bound(nextinter.begin(), nextinter.end(), (*next) + 1);
                                }
                                else
                                {
                                    ++other;
                                }
                            }
                            assert(nextinter.size() == 0 || nextinter[0] != 0);
                        }
                        /*if(ae.to == 0)
                        {
                            return true;
                        }
                        {
                            auto lb = std::lower_bound(nextinter.begin(), nextinter.end(), ae.to);
                            if(lb == nextinter.end() || *lb != ae.to)
                            {
                                nextinter.insert(lb, ae.to);
                                std::vector<size_t> buffer;
                                std::set_union( nextinter.begin(), nextinter.end(),
                                                states[ae.to].simulates.begin(), states[ae.to].simulates.end(),
                                                std::back_inserter(buffer));
                                nextinter.swap(buffer);
                            }
                            assert(nextinter[0] != 0);
                        }*/
                    }
        #ifndef NDEBUG
                    assert(is_sorted(as.get_edges().begin(), as.get_edges().end()));

                    while(it != as.get_edges().end())
                    {
                        assert(it->edge != state.get_edge_cnt());
                        ++it;
                    }
        #endif
                }
            }


        #ifdef NOCHANGE
            bool loaded = false;
            auto next = nextinter.begin();
            std::vector<size_t> writes;


            for(size_t i : maximal)
            {
                if(i == 0)
                {
                    assert(false);
                    continue;
                }
                AutomataState& as = states[i];
        //            assert(as.restricts.size() > 0);
                while(next != nextinter.end() && *next < i) { ++next; };
                if(next == nextinter.end() || *next != i)
                {
                    // added non-interfering
                    /*if(!loaded)
                    {
                        auto* se = state.get_edge_cnt().getSender(system.get());
                        auto* re = state.get_edge_cnt().getReciever(system.get());

                        if(re) 
                        {
                            auto& rew = re->getWrites();
                            auto& sew = se->getWrites();
                            std::set_union(rew.begin(), rew.end(), sew.begin(), sew.end(), std::back_inserter(writes));
                        }
                        else
                        {
                            auto& sew = se->getWrites();
                            std::copy(sew.begin(), sew.end(), std::back_inserter(writes));
                        }
                        loaded = true;
                    }*/


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
                            std::vector<size_t> buffer;
                            std::set_union( nextinter.begin(), nextinter.end(),
                                            as.simulates.begin(), as.simulates.end(),
                                            std::back_inserter(buffer));
                            assert(buffer[0] != 0);
                            nextinter.swap(buffer);
                            next = std::lower_bound(nextinter.begin(), nextinter.end(), i + 1);
                        }
                    }
                }
            }
        #endif

/*        std::cout << "NEXT INTER IS ";
            for(size_t j : nextinter) std::cout << j << ", ";
            std::cout << std::endl;*/

        #ifdef ANTISIM
        //#ifndef NDEBUG
            maximal.clear();
            maximal = nextinter;
            if(maximal.size() == 0 || maximal[0] != 1) maximal.insert(maximal.begin(), 1);

            for(size_t i : nextinter)
            {
                std::vector<size_t> buffer;
                std::set_union(states[i].simulates.begin(), states[i].simulates.end(),
                                maximal.begin(), maximal.end(), std::back_inserter(buffer));
                maximal.swap(buffer);
            }
        //    assert(maximal.size() == nextinter.size());
            maximal.swap(nextinter);
        //#endif
        #endif

            return false;
        }
        
        
        
        void TARReachabilitySearch::reachable(   std::vector<std::shared_ptr<PQL::Condition> >& queries, 
                                        std::vector<ResultPrinter::Result>& results,
                                        bool printstats)
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
                    bool res = tryReach(queries[i], results, printstats, state);
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
                    {
                        results[i] = printQuery(queries[i], i, ResultPrinter::Satisfied);
//                        std::cout << queries[i]->toString() << std::endl;
//                        state.print(_net);
                    }
                    else
                    {
                        alldone = false;
                    }
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

