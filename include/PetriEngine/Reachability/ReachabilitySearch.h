/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef BREADTHFIRSTREACHABILITYSEARCH_H
#define BREADTHFIRSTREACHABILITYSEARCH_H

#include "../Structures/State.h"
#include "ReachabilityResult.h"
#include "../PQL/PQL.h"
#include "../PetriNet.h"
#include "../Structures/StateSet.h"
#include "../Structures/Queue.h"
#include "../Structures/PotencyQueue.h"
#include "../SuccessorGenerator.h"
#include "../ReducingSuccessorGenerator.h"
#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "PetriEngine/options.h"

#include <memory>
#include <vector>



namespace PetriEngine {
    namespace Reachability {

        /** Implements reachability check in a BFS manner using a hash table */
        class ReachabilitySearch {
        public:

            ReachabilitySearch(PetriNet& net, AbstractHandler& callback, int kbound = 0, bool early = false)
            : _net(net), _kbound(kbound), _callback(callback) {
            }

            ~ReachabilitySearch()
            {
            }

            /** Perform reachability check using BFS with hasing */
            bool reachable(
                    std::vector<std::shared_ptr<PQL::Condition > >& queries,
                    std::vector<ResultPrinter::Result>& results,
                    Strategy strategy,
                    bool usestubborn,
                    bool statespacesearch,
                    bool printstats,
                    bool keep_trace,
                    size_t seed);
            size_t maxTokens() const;
        private:
            struct searchstate_t {
                size_t expandedStates = 0;
                size_t exploredStates = 1;
                std::vector<size_t> enabledTransitionsCount;
                size_t heurquery = 0;
                bool usequeries;
            };

            template<typename W, typename G>
            bool tryReachRandomWalk(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                bool usequeries,
                bool printstats,
                size_t maxSteps,
                size_t seed);

            template<typename Q, typename W = Structures::StateSet, typename G>
            bool tryReach(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                bool usequeries,
                bool printstats,
                size_t seed);
            void printStats(searchstate_t& s, Structures::StateSetInterface*);
            bool checkQueries(  std::vector<std::shared_ptr<PQL::Condition > >&,
                                    std::vector<ResultPrinter::Result>&,
                                    Structures::State&, searchstate_t&, Structures::StateSetInterface*);
            std::pair<ResultPrinter::Result,bool> doCallback(std::shared_ptr<PQL::Condition>& query, size_t i, ResultPrinter::Result r, searchstate_t &ss, Structures::StateSetInterface *states);

            PetriNet& _net;
            int _kbound;
            size_t _satisfyingMarking = 0;
            Structures::State _initial;
            AbstractHandler& _callback;
            size_t _max_tokens = 0;
        };

        template <typename G>
        inline G _makeSucGen(PetriNet &net, std::vector<PQL::Condition_ptr> &queries) {
            return G{net, queries};
        }
        template <>
        inline ReducingSuccessorGenerator _makeSucGen(PetriNet &net, std::vector<PQL::Condition_ptr> &queries) {
            auto stubset = std::make_shared<ReachabilityStubbornSet>(net, queries);
            stubset->setInterestingVisitor<InterestingTransitionVisitor>();
            return ReducingSuccessorGenerator{net, stubset};
        }

        template<typename Q, typename W, typename G>
        bool ReachabilitySearch::tryReach(   std::vector<std::shared_ptr<PQL::Condition> >& queries,
                                        std::vector<ResultPrinter::Result>& results, bool usequeries,
                                        bool printstats, size_t seed)
        {

            // set up state
            searchstate_t ss;
            ss.enabledTransitionsCount.resize(_net.numberOfTransitions(), 0);
            ss.expandedStates = 0;
            ss.exploredStates = 1;
            ss.heurquery = queries.size() >= 2 ? std::rand() % queries.size() : 0;
            ss.usequeries = usequeries;

            // set up working area
            Structures::State state;
            Structures::State working;
            _initial.setMarking(_net.makeInitialMarking());
            state.setMarking(_net.makeInitialMarking());
            working.setMarking(_net.makeInitialMarking());

            W states(_net, _kbound);    // stateset
            Q queue(seed);           // working queue
            G generator = _makeSucGen<G>(_net, queries); // successor generator
            auto r = states.add(state);
            // this can fail due to reductions; we push tokens around and violate K
            if(r.first){
                // add initial to states, check queries on initial state
                _satisfyingMarking = r.second;
                // check initial marking
                if(ss.usequeries)
                {
                    if(checkQueries(queries, results, working, ss, &states))
                    {
                        if(printstats)
                            printStats(ss, &states);
                        _max_tokens = states.maxTokens();
                        return true;
                    }
                }
                // add initial to queue
                {
                    PQL::DistanceContext dc(&_net, working.marking());
                    queue.push(r.second, &dc, queries[ss.heurquery].get());
                }

                // Search!
                for(auto nid = queue.pop(); nid != Structures::Queue::EMPTY; nid = queue.pop()) {
                    states.decode(state, nid);
                    generator.prepare(&state);

                    while(generator.next(working)){
                        ss.enabledTransitionsCount[generator.fired()]++;
                        auto res = states.add(working);
                        // If we have not seen this state before
                        if (res.first) {
                            {
                                PQL::DistanceContext dc(&_net, working.marking());
                                if constexpr (std::is_same_v<Q, Structures::RandomPotencyQueue>)
                                    queue.push(res.second, &dc, queries[ss.heurquery].get(), generator.fired());
                                else
                                    queue.push(res.second, &dc, queries[ss.heurquery].get());
                            }
                            states.setHistory(res.second, generator.fired());
                            _satisfyingMarking = res.second;
                            ss.exploredStates++;
                            if (checkQueries(queries, results, working, ss, &states)) {
                                if(printstats)
                                    printStats(ss, &states);
                                _max_tokens = states.maxTokens();
                                return true;
                            }
                        }
                    }
                    ss.expandedStates++;
                }
            }

            // no more successors, print last results
            for(size_t i= 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    results[i] = doCallback(queries[i], i, ResultPrinter::NotSatisfied, ss, &states).first;
                }
            }

            if(printstats)
                printStats(ss, &states);
            _max_tokens = states.maxTokens();
            return false;
        }

        template<typename W, typename G>
        bool ReachabilitySearch::tryReachRandomWalk(std::vector<std::shared_ptr<PQL::Condition> >& queries,
                                                    std::vector<ResultPrinter::Result>& results, bool usequeries,
                                                    bool printstats, size_t maxSteps, size_t seed)
        {
            // set up state
            searchstate_t ss;
            ss.enabledTransitionsCount.resize(_net.numberOfTransitions(), 0);
            ss.expandedStates = 0;
            ss.exploredStates = 1;
            ss.heurquery = queries.size() >= 2 ? std::rand() % queries.size() : 0;
            ss.usequeries = usequeries;

            // set up working area
            Structures::State state;
            Structures::State working;
            _initial.setMarking(_net.makeInitialMarking());
            state.setMarking(_net.makeInitialMarking());
            working.setMarking(_net.makeInitialMarking());

            W states(_net, _kbound);                        // stateset
            Structures::RandomWalkPotencyQueue queue(seed); // working queue
            G generator = _makeSucGen<G>(_net, queries);    // successor generator
            auto r = states.add(state);
            // this can fail due to reductions; we push tokens around and violate K
            if(r.first){
                // add initial to states, check queries on initial state
                _satisfyingMarking = r.second;
                size_t _initialMarking = r.second;
                // check initial marking
                if(ss.usequeries)
                {
                    if(checkQueries(queries, results, working, ss, &states))
                    {
                        if(printstats)
                            printStats(ss, &states);
                        _max_tokens = states.maxTokens();
                        return true;
                    }
                }
                // Typically a timeout
                bool stopRandomWalkCondition = false;
                while(!stopRandomWalkCondition) {
                    // add initial to queue
                    {
                        PQL::DistanceContext dc(&_net, _initial.marking());
                        queue.push(r.second, &dc, queries[ss.heurquery].get());
                    }

                    // Search! Each step is a random walk and resets the queue
                    size_t stepCounter = 0;
                    for(auto nid = queue.pop(); nid != Structures::Queue::EMPTY && stepCounter < maxSteps; nid = queue.pop(), ++stepCounter) {
                        states.decode(state, nid);
                        generator.prepare(&state);

                        while(generator.next(working)) {
                            ss.enabledTransitionsCount[generator.fired()]++;
                            auto res = states.add(working);
                            // If we have not seen this state before
                            if (res.first) {
                                {
                                    PQL::DistanceContext dc(&_net, working.marking());
                                    queue.push(res.second, &dc, queries[ss.heurquery].get(), generator.fired());
                                }
                                // states.setHistory(res.second, generator.fired()); // No need to setHistory
                                _satisfyingMarking = res.second;
                                ss.exploredStates++;
                                if (checkQueries(queries, results, working, ss, &states)) {
                                    if(printstats)
                                        printStats(ss, &states);
                                    _max_tokens = states.maxTokens();
                                    return true;
                                }
                            }
                        }
                        ss.expandedStates++;
                    }
                }
            }

            // no more successors, print last results
            for(size_t i= 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    results[i] = doCallback(queries[i], i, ResultPrinter::NotSatisfied, ss, &states).first;
                }
            }

            if(printstats)
                printStats(ss, &states);
            _max_tokens = states.maxTokens();
            return false;
        }
    }
} // Namespaces

#endif // BREADTHFIRSTREACHABILITYSEARCH_H
