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

            virtual ~ReachabilitySearch()
            {
            }

            /** Perform reachability check using BFS with hasing */
            bool reachable(
                    std::vector<std::shared_ptr<PQL::Condition > >& queries,
                    std::vector<ResultPrinter::Result>& results,
                    Strategy strategy,
                    bool usestubborn,
                    bool statespacesearch,
                    StatisticsLevel printstats,
                    bool keep_trace,
                    size_t seed,
                    int64_t depthRandomWalk = 50000,
                    const int64_t incRandomWalk = 5000,
                    const std::vector<MarkVal>& initPotencies = std::vector<MarkVal>());
            size_t maxTokens() const;
        protected:
            struct searchstate_t {
                size_t expandedStates = 0;
                size_t exploredStates = 1;
                std::vector<size_t> enabledTransitionsCount;
                size_t heurquery = 0;
                bool usequeries;
            };

            template<typename W = Structures::RandomWalkStateSet, typename G>
            bool tryReachRandomWalk(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                bool usequeries,
                StatisticsLevel,
                size_t seed,
                int64_t depthRandomWalk,
                const int64_t incRandomWalk,
                const std::vector<MarkVal>& initPotencies);

            template<typename Q, typename W = Structures::StateSet, typename G>
            bool tryReach(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                bool usequeries,
                StatisticsLevel statisticsLevel,
                size_t seed,
                const std::vector<MarkVal>& initPotencies);

            void printStats(searchstate_t& s, Structures::StateSetInterface*, StatisticsLevel);

            virtual bool checkQueries(std::vector<std::shared_ptr<PQL::Condition > >&,
                              std::vector<ResultPrinter::Result>&,
                              Structures::State&, searchstate_t&,
                              Structures::StateSetInterface*);

            virtual std::pair<ResultPrinter::Result,bool> doCallback(
                std::shared_ptr<PQL::Condition>& query, size_t i,
                ResultPrinter::Result r, searchstate_t &ss,
                Structures::StateSetInterface *states);

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
        bool ReachabilitySearch::tryReach(std::vector<std::shared_ptr<PQL::Condition> >& queries,
                                        std::vector<ResultPrinter::Result>& results, bool usequeries,
                                        StatisticsLevel statisticsLevel, size_t seed,
                                        const std::vector<MarkVal>& initPotencies)
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

            W states(_net, _kbound); // stateset

            Q queue(seed); // Working queue
            if constexpr (std::is_base_of_v<Structures::PotencyQueue, Q>) {
                if (!initPotencies.empty())
                    queue = Q(initPotencies, seed);
            }

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
                        if(statisticsLevel != StatisticsLevel::None)
                            printStats(ss, &states, statisticsLevel);
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
                                if(statisticsLevel != StatisticsLevel::None)
                                    printStats(ss, &states, statisticsLevel);
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

            if(statisticsLevel != StatisticsLevel::None)
                printStats(ss, &states, statisticsLevel);
            _max_tokens = states.maxTokens();
            return false;
        }

        template<typename W, typename G>
        bool ReachabilitySearch::tryReachRandomWalk(std::vector<std::shared_ptr<PQL::Condition> >& queries,
                                                    std::vector<ResultPrinter::Result>& results, bool usequeries,
                                                    StatisticsLevel statisticsLevel, size_t seed,
                                                    int64_t depthRandomWalk, const int64_t incRandomWalk,
                                                    const std::vector<MarkVal>& initPotencies)
        {
            // Set up state
            searchstate_t ss;
            ss.enabledTransitionsCount.resize(_net.numberOfTransitions(), 0);
            ss.expandedStates = 0;
            ss.exploredStates = 1;
            ss.heurquery = queries.size() >= 2 ? std::rand() % queries.size() : 0;
            ss.usequeries = usequeries;
            const PQL::Condition *query = queries[ss.heurquery].get();

            // Set up working area
            _initial.setMarking(_net.makeInitialMarking());
            Structures::State candidate; // Candidate for the next step, modified by the generator
            Structures::State currentStepState;
            candidate.setMarking(_net.makeInitialMarking());
            currentStepState.setMarking(_net.makeInitialMarking());

            W states(_net, _kbound, query, initPotencies, seed); // RandomWalk State Set
            G generator = _makeSucGen<G>(_net, queries); // Successor generator

            // Check initial marking
            if(ss.usequeries)
            {
                if(checkQueries(queries, results, _initial, ss, &states))
                {
                    if(statisticsLevel != StatisticsLevel::None)
                        printStats(ss, &states, statisticsLevel);
                    _max_tokens = states.maxTokens();
                    return true;
                }
            }

            if constexpr (std::is_same_v<W, Structures::TracableRandomWalkStateSet>) {
                // Not to be 0 in case of a printTrace call
                _satisfyingMarking = 1;
            }

            const int64_t maxDepthValue = std::numeric_limits<int64_t>::max() - incRandomWalk;
            while(true) {
                // Start a new random walk
                states.newWalk();

                // Search! Each turn is a random step
                for(int stepCounter = 0; stepCounter < depthRandomWalk; ++stepCounter) {
                    // The currentStepMarking is the nextMarking computed in the previous step
                    if (!states.nextStep(currentStepState.marking())) {
                        // No candidate found at the previous step, do a new walk
                        break;
                    }
                    generator.prepare(&currentStepState);
                    if constexpr (std::is_same_v<W, Structures::TracableRandomWalkStateSet>) {
                        // Add one element to the trace. It will be modified using setHistory
                        states.addStepTrace();
                    }

                    while(generator.next(candidate)) {
                        ss.enabledTransitionsCount[generator.fired()]++;
                        ss.exploredStates++;

                        if constexpr (std::is_same_v<W, Structures::TracableRandomWalkStateSet>) {
                            // Save the current queued transition
                            states.savePreviousTransition();

                            // We have to pretend that the current candidate transition is fired.
                            // Then checkQueries will work for this candidate. In particular, it will
                            // be able to print the correct trace.
                            states.setHistory(static_cast<size_t>(generator.fired()));
                        }

                        if (checkQueries(queries, results, candidate, ss, &states)) {
                            if(statisticsLevel != StatisticsLevel::None)
                                printStats(ss, &states, statisticsLevel);
                            _max_tokens = states.maxTokens();
                            return true;
                        } else {
                            if constexpr (std::is_same_v<W, Structures::TracableRandomWalkStateSet>) {
                                // Returns false if the candidate is not a better candidate to be the next marking
                                if (!states.computeCandidate(candidate.marking(), query, generator.fired())) {
                                    // The candidate does not satisfy the queries and is not marked as a better
                                    // candidate to be the next marking, so we restore the previous queued transition
                                    states.setHistory(states.getPreviousTransition());
                                }
                            } else {
                                states.computeCandidate(candidate.marking(), query, generator.fired());
                            }
                        }
                    }
                    ss.expandedStates++;
                }
                if (depthRandomWalk < maxDepthValue) {
                    depthRandomWalk += incRandomWalk;
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

            if(statisticsLevel != StatisticsLevel::None)
                printStats(ss, &states, statisticsLevel);
            _max_tokens = states.maxTokens();
            return false;
        }
    }
} // Namespaces

#endif // BREADTHFIRSTREACHABILITYSEARCH_H
