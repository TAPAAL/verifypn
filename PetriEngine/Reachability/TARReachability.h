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
#include <z3++.h>
#include "ReachabilitySearch.h"
#include "../TAR/TARAutomata.h"
namespace PetriEngine {
    namespace Reachability {
        class TARReachabilitySearch {
        private:
            ResultPrinter& printer;
            

        public:

            TARReachabilitySearch(ResultPrinter& printer, PetriNet& net, Reducer* reducer, int kbound = 0)
            : printer(printer), _net(net), _reducer(reducer) {
                _kbound = kbound;
            }
            
            ~TARReachabilitySearch()
            {
            }
            
            void reachable(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                bool printstats, bool printtrace);
        protected:
            virtual void addNonChanging(state_t& state, std::vector<size_t>& maximal, std::vector<size_t>& nextinter);
            virtual std::vector<size_t> expandSimulation(std::vector<size_t>& from);
            virtual bool followSymbol(std::vector<size_t>& from, std::vector<size_t>& nextinter, size_t symbol);
            z3::expr computeParameters(
                z3::context& context, std::vector<z3::expr>& encoded, z3::expr& param_reach, 
                const std::vector<uint32_t>&, const std::vector<bool>& inq, const std::vector<bool>& read);
        private:
            typedef std::vector<state_t> waiting_t;
            void printTrace(waiting_t& stack);
            bool tryReach(   const std::shared_ptr<PQL::Condition>& query, 
                                        std::vector<ResultPrinter::Result>& results,
                                        bool printstats, bool printtrace, Structures::State& initial);
            size_t computeSimulation(size_t index, size_t sim_hint = 1, size_t simed_hint = 0);
            bool popDone(waiting_t& waiting, size_t& stepno);
            bool checkInclussion(state_t& state, std::vector<size_t>& nextinter, z3::context& ctx);

            void handleInvalidTrace(waiting_t& waiting, int nvalid);
            std::pair<int,bool>  isValidTrace(waiting_t& trace, z3::context& context, bool probe, Structures::State& initial, z3::expr& query, const std::vector<bool>& inq, z3::expr& param);
            bool findValidRange( int& from, 
                                            const int to, 
                                            z3::context& context, 
                                            z3::expr_vector& interpolant, 
                                            std::vector<z3::expr>& encoded);
            int constructAutomata(int from, waiting_t& trace, z3::expr_vector& inter, z3::context& context);
            std::pair<bool, size_t> stateForPredicate(int type, z3::expr pred, z3::context& context, size_t sim_hint = 1, size_t simed_hint = 0);
            void printStats();
            bool checkQueries(  std::vector<std::shared_ptr<PQL::Condition > >&,
                                std::vector<ResultPrinter::Result>&,
                                Structures::State&, bool);
            ResultPrinter::Result printQuery(std::shared_ptr<PQL::Condition>& query, size_t i, ResultPrinter::Result);
            
            int _kbound;
            PetriNet& _net;
            Reducer* _reducer;
            struct el_t
            {
                z3::expr expr;
                size_t state;
                el_t(z3::expr e, size_t s) : expr(e), state(s) {};
            };
            std::unordered_map<size_t, std::vector<el_t>> intmap;
            std::vector<AutomataState> states;
            std::vector<size_t> simorder;
            std::map<size_t, std::vector<size_t>> edge_interpolants;

        };
        
    }
}
#endif /* TARREACHABILITY_H */

