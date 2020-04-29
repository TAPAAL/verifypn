/* 
 * File:   TARReachability.h
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

#ifndef TARREACHABILITY_H
#define TARREACHABILITY_H
#include "TARAutomata.h"
#include "TraceSet.h"
#include "AntiChain.h"

#include "PetriEngine/Reachability/ReachabilitySearch.h"


namespace PetriEngine {
    namespace Reachability {
        class Solver;
        class TARReachabilitySearch {
        private:
            AbstractHandler& _printer;
            

        public:

            TARReachabilitySearch(AbstractHandler& printer, PetriNet& net, Reducer* reducer, int kbound = 0)
            : _printer(printer), _net(net), _reducer(reducer), _traceset(net) {
                _kbound = kbound;
            }
            
            ~TARReachabilitySearch()
            {
            }
            
            void reachable(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                bool printstats, bool printtrace);
        private:

            void printTrace(trace_t& stack);
            void nextEdge(AntiChain<uint32_t, size_t>& checked, state_t& state, trace_t& waiting, std::set<size_t>& nextinter);
            bool tryReach(  bool printtrace, Solver& solver);
            std::pair<bool,bool> runTAR(    bool printtrace, Solver& solver, std::vector<bool>& use_trans);
            bool popDone(trace_t& waiting, size_t& stepno);
            bool doStep(state_t& state, std::set<size_t>& nextinter);
            void addNonChanging(state_t& state, std::set<size_t>& maximal, std::set<size_t>& nextinter);
            bool validate(const std::vector<size_t>& transitions);

            void handleInvalidTrace(trace_t& waiting, int nvalid);
            std::pair<int,bool>  isValidTrace(trace_t& trace, Structures::State& initial, const std::vector<bool>&, PQL::Condition* query);
            void printStats();
            bool checkQueries(  std::vector<std::shared_ptr<PQL::Condition > >&,
                                std::vector<ResultPrinter::Result>&,
                                Structures::State&, bool);
            
            int _kbound;
            size_t _stepno = 0;
            PetriNet& _net;
            Reducer* _reducer;
            TraceSet _traceset;

#ifdef TAR_TIMING
            double _check_time = 0;
            double _next_time = 0;
            double _do_step = 0;
            double _do_step_next = 0;
            double _non_change_time = 0;
            double _follow_time = 0;
#endif
        };
        
    }
}
#endif /* TARREACHABILITY_H */

