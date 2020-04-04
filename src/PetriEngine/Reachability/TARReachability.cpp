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

#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/TAR/AntiChain.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/Reachability/TARReachability.h"
#include "PetriEngine/Reachability/RangeContext.h"
#include "PetriEngine/TAR/Solver.h"


namespace PetriEngine {
    using namespace PQL;
    namespace Reachability {

        void TARReachabilitySearch::handleInvalidTrace(trace_t& waiting, int nvalid)
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
                if(checkInclussion(waiting[i - 1], waiting[i].get_interpolants()))
                {
                    if(i != 1) waiting.resize(i - 1);
                    break;
                }
            }
            if(waiting.size() == 0) return;
        }
        
        
        bool TARReachabilitySearch::popDone(trace_t& waiting, size_t& stepno)
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

        void TARReachabilitySearch::nextEdge(AntiChain<uint32_t, size_t>& checked, state_t& state, trace_t& waiting, std::vector<size_t>&& nextinter)
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
                std::vector<size_t> minimal = _traceset.minimize(nextinter);
                checked.insert(dummy, minimal);
                next.reset_edges(_net);
                next.set_interpolants(std::move(minimal));
                waiting.push_back(next);
            }            
        }
        
        bool TARReachabilitySearch::runTAR( bool printtrace,
                                            Solver& solver)
        {
            auto checked = AntiChain<uint32_t, size_t>();
            // waiting-list with levels
            bool all_covered = true;
            trace_t waiting;
            // initialize
            {
                state_t state;
                state.reset_edges(_net);
                state.set_interpolants(_traceset.initial());
                waiting.push_back(state);
            }
            while (!waiting.empty()) 
            {
                if(popDone(waiting, _stepno)) 
                    continue;  // we have reached the end of the edge-iterator for this part of the trace

                ++_stepno;

                assert(waiting.size() > 0 );
                state_t& state = waiting.back();
                std::vector<size_t> nextinter;
                if(checkInclussion(state, nextinter)) // Check if the next state makes the interpolant automata accept.
                {
                    state.next_edge(_net);
                    continue;
                }

                if(waiting.back().get_edge_cnt() == 0) // check if proposition is satisfied
                {
                    auto res = solver.check(waiting);
                    if(res.first)
                    {
                        if(printtrace)
                            printTrace(waiting);
                        return true;
                    }
                    else
                    {
                        auto some = _traceset.addTrace(waiting, res.second);
                        assert(some || !all_covered);
                        if(!some)
                            return false;                        
                        handleInvalidTrace(waiting, res.second.size());
                        all_covered = false;
                        continue;
                    }
                }
                else
                {
#ifdef VERBOSETAR
//                    printStats();
#endif
                    nextEdge(checked, state, waiting, std::move(nextinter));
                }
            }
            return all_covered;
        }
        
        bool TARReachabilitySearch::tryReach(bool printtrace, Solver& solver)
        {            
            _traceset.removeEdges(0);
            while(!runTAR(printtrace, solver)) 
            {} // redo TAR until we have one complete run-through without interpolant traces.

#ifdef VERBOSETAR
            _traceset.print(std::cerr);
            for(size_t t = 0; t < _net.numberOfTransitions(); ++t)
            {
                auto pre = _net.preset(t);
                std::cerr << "T" << t << "\n";
                for(; pre.first != pre.second; ++pre.first)
                {
                    std::cerr << "\tP" << pre.first->place << " - " << pre.first->tokens << std::endl;
                }
                auto post = _net.postset(t);
                for(; post.first != post.second; ++post.first)
                {
                    std::cerr << "\tP" << post.first->place << " + " << post.first->tokens << std::endl;
                }
            }
            for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
            {
                if(_net.initial()[p] != 0)
                    std::cerr << "P" << p << " (" << _net.initial()[p] << ")\n";
            }  
#endif
            if(printtrace)
                _traceset.print(std::cerr);            
            return false;            
        }

        bool TARReachabilitySearch::checkInclussion(state_t& state, std::vector<size_t>& nextinter)
        {
            auto maximal = _traceset.maximize(state.get_interpolants());
            // if NFA accepts the trace after this instruction, abort.
            if(_traceset.follow(maximal, nextinter, state.get_edge_cnt()))
            {
                return true;
            }

            if(state.get_edge_cnt() == 0)
                return false;
            addNonChanging(state, maximal, nextinter);
        
            nextinter = _traceset.maximize(nextinter);
            return false;
        }
                
        void TARReachabilitySearch::addNonChanging(state_t& state, std::vector<size_t>& maximal, std::vector<size_t>& nextinter)
        {
            
            std::vector<int64_t> changes;
            auto pre = _net.preset(state.get_edge_cnt() - 1);
            auto post = _net.postset(state.get_edge_cnt() - 1);

            for(; pre.first != pre.second; ++pre.first)
            {
                if(pre.first->inhibitor) { assert(false); continue;}
                if(pre.first->direction < 0)
                    changes.push_back(pre.first->place);
            }

            for(; post.first != post.second; ++post.first)
            {
                if(pre.first->direction >= 0)
                    changes.push_back(post.first->place);
            }
            std::sort(changes.begin(), changes.end());
            _traceset.copyNonChanged(maximal, changes, nextinter);
        }
        
        void TARReachabilitySearch::printTrace(trace_t& stack)
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
                    Solver solver(_net, state.marking(), queries[i].get(), used);
                    bool res = tryReach(printtrace, solver);
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
            std::cerr << "STEPS : " << _stepno << std::endl;
            std::cerr << "INTERPOLANT AUTOMATAS : " << _traceset.initial().size() << std::endl;
        }        
    }
}
