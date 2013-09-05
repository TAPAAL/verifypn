/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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
#include "BreadthFirstReachabilitySearch.h"
#include "../PQL/PQL.h"
#include "../PQL/Contexts.h"
#include "../Structures/StateSet.h"
#include "../Structures/LimitedStateAllocator.h"

#include <list>
#include <string.h>

using namespace PetriEngine::PQL;
using namespace PetriEngine::Structures;

namespace PetriEngine{ namespace Reachability {

ReachabilityResult BreadthFirstReachabilitySearch::reachable(const PetriNet &net,
															 const MarkVal *m0,
															 const VarVal *v0,
															 PQL::Condition *query){
	//Create StateSet
	StateSet states(net, _kbound);
	std::list<State*> queue;

	LimitedStateAllocator<> allocator(net, _memorylimit);

	State* s0 = allocator.createState();
	if(!s0)
		return ReachabilityResult(ReachabilityResult::Unknown, "Memory bound exceeded");
	memcpy(s0->marking(), m0, sizeof(MarkVal)*net.numberOfPlaces());
	memcpy(s0->valuation(), v0, sizeof(VarVal)*net.numberOfVariables());

	states.add(s0);
	queue.push_back(s0);

	unsigned int max = 0;
	int count = 0;
	BigInt expandedStates = 0;
	BigInt exploredStates = 1;
	
	if(query->evaluate(*s0, &net)){
		return ReachabilityResult(ReachabilityResult::Satisfied, "Query was satisfied",
								  expandedStates, exploredStates, states.discovered(), states.maxTokens(), s0->pathLength(), s0->trace());
	}
	
	State* ns = allocator.createState();
	if(!ns)
		return ReachabilityResult(ReachabilityResult::Unknown, "Memory bound exceeded");
	while(!queue.empty()){
		// Progress reporting and abort checking
		if(count++ & 1<<17){
			if(queue.size() > max)
				max = queue.size();
			count = 0;
			// Report progress
			reportProgress((double)(max - queue.size())/(double)max);
			// Check abort
			if(abortRequested())
				return ReachabilityResult(ReachabilityResult::Unknown,
										"Search was aborted.");
		}

		State* s = queue.front();
		queue.pop_front();
		for(unsigned int t = 0; t < net.numberOfTransitions(); t++){
			if(net.fire(t, s, ns, 1)){
				if(states.add(ns)){
					exploredStates++;
					ns->setParent(s);
					ns->setTransition(t);
					if(query->evaluate(*ns, &net)){
						//ns->dumpTrace(net);
						return ReachabilityResult(ReachabilityResult::Satisfied,
												"A state satisfying the query was found", expandedStates, exploredStates, states.discovered(), states.maxTokens(), ns->pathLength(), ns->trace());
					}

					queue.push_back(ns);
					ns = allocator.createState();
					if(!ns)
						return ReachabilityResult(ReachabilityResult::Unknown,
												   "Memory bound exceeded",
												   expandedStates, exploredStates, states.discovered(), states.maxTokens());
				}
			}
		}
		expandedStates++;
	}

	return ReachabilityResult(ReachabilityResult::NotSatisfied,
						"No state satisfying the query exists.", expandedStates, exploredStates, states.discovered(), states.maxTokens());
}

}}
