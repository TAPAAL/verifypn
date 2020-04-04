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
#include "ReachabilitySearch.h"
#include "../TAR/TARAutomata.h"
#include "PetriEngine/TAR/TraceSet.h"
#include "PetriEngine/TAR/AntiChain.h"

namespace PetriEngine {
    namespace Reachability {
        class Solver;
        class TARReachabilitySearch {
        private:
            ResultPrinter& printer;
            

        public:

            TARReachabilitySearch(ResultPrinter& printer, PetriNet& net, Reducer* reducer, int kbound = 0)
            : printer(printer), _net(net), _reducer(reducer), _traceset(net.numberOfPlaces()) {
                _kbound = kbound;
            }
            
            ~TARReachabilitySearch()
            {
            }
            
            void reachable(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                bool printstats, bool printtrace, PetriNetBuilder& builder);
        private:

            void printTrace(trace_t& stack);
            void nextEdge(AntiChain<uint32_t, size_t>& checked, state_t& state, trace_t& waiting, std::vector<size_t>&& nextinter);
            bool tryReach(  bool printtrace, Solver& solver);
            std::pair<bool,bool> runTAR(    bool printtrace, Solver& solver);
            bool popDone(trace_t& waiting, size_t& stepno);
            bool doStep(state_t& state, std::vector<size_t>& nextinter);
            void addNonChanging(state_t& state, std::vector<size_t>& maximal, std::vector<size_t>& nextinter);
            bool validate(const std::vector<size_t>& transitions);

            void handleInvalidTrace(trace_t& waiting, int nvalid);
            std::pair<int,bool>  isValidTrace(trace_t& trace, Structures::State& initial, const std::vector<bool>&, PQL::Condition* query);
            void printStats();
            bool checkQueries(  std::vector<std::shared_ptr<PQL::Condition > >&,
                                std::vector<ResultPrinter::Result>&,
                                Structures::State&, bool);
            ResultPrinter::Result printQuery(std::shared_ptr<PQL::Condition>& query, size_t i, ResultPrinter::Result);
            
            int _kbound;
            size_t _stepno = 0;
            PetriNet& _net;
            Reducer* _reducer;
            TraceSet _traceset;

        };
        
    }
}
#endif /* TARREACHABILITY_H */

