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
#include "DepthFirstReachabilitySearch.h"
#include "../PQL/PQL.h"
#include "../PQL/Contexts.h"
#include "../Structures/StateSet.h"
#include "../Structures/LimitedStateAllocator.h"
//#include "../Structures/StateAllocator.h"

#include <list>
#include <string.h>

using namespace PetriEngine::PQL;
using namespace PetriEngine::Structures;

namespace PetriEngine{ namespace Reachability {

ReachabilityResult DepthFirstReachabilitySearch::reachable(const PetriNet &net,
														   const MarkVal *m0,
														   const VarVal *v0,
														   PQL::Condition *query){
	//Create StateSet
	StateSet states(net, _kbound);
	std::list<Step> stack;

	LimitedStateAllocator<> allocator(net, _memorylimit);
//	StateAllocator<> allocator(net);

	State* s0 = allocator.createState();
	if(!s0)
		return ReachabilityResult(ReachabilityResult::Unknown,
								   "Memory bound exceeded");
	memcpy(s0->marking(), m0, sizeof(MarkVal)*net.numberOfPlaces());
	memcpy(s0->valuation(), v0, sizeof(VarVal)*net.numberOfVariables());

  	states.add(s0);
	stack.push_back(Step(s0, 0));

	unsigned int max = 0;
	int count = 0;
	BigInt exploredStates = 1;
	BigInt expandedStates = 0;
	std::vector<BigInt> enabledTransitionsCount (net.numberOfTransitions());
	
	if(query->evaluate(*s0, &net)){
		return ReachabilityResult(ReachabilityResult::Satisfied, "Query was satisfied",
								  expandedStates, exploredStates, states.discovered(), enabledTransitionsCount, states.maxTokens(), states.maxPlaceBound(), s0->pathLength(), s0->trace());
	}
	
	State* ns = allocator.createState();
	if(!ns)
		return ReachabilityResult(ReachabilityResult::Unknown,
								   "Memory bound exceeded",
								   expandedStates, exploredStates);
	while(!stack.empty()){
		if(count++ & 1<<18){
			if(stack.size() > max)
				max = stack.size();
			count = 0;
			//report progress
			reportProgress((double)(max-stack.size())/(double)max);
			//check abort
			if(abortRequested())
				return ReachabilityResult(ReachabilityResult::Unknown,
										"Search was aborted.");
		}

		//Take first step of the stack
		State* s = stack.back().state;
		//if(stack.back().t == 0)
			//expandedStates++;
		ns->setParent(s);
		bool foundSomething = false;
		for(unsigned int t = stack.back().t; t < net.numberOfTransitions(); t++){
			if(net.fire(t, s, ns, 1)){
				enabledTransitionsCount[t]++;
				if(states.add(ns)){
					ns->setTransition(t);
					if(query->evaluate(PQL::EvaluationContext(ns->marking(), ns->valuation(), &net)))
						return ReachabilityResult(ReachabilityResult::Satisfied,
									  "A state satisfying the query was found", expandedStates, exploredStates,
									  states.discovered(), enabledTransitionsCount, states.maxTokens(), states.maxPlaceBound(), ns->pathLength(), ns->trace());
					stack.back().t = t + 1;
					stack.push_back(Step(ns, 0));
					exploredStates++;
					foundSomething = true;
					ns = allocator.createState();
					if(!ns)
						return ReachabilityResult(ReachabilityResult::Unknown,
												   "Memory bound exceeded",
												   expandedStates, exploredStates, states.discovered(), enabledTransitionsCount, states.maxTokens(), states.maxPlaceBound());
					break;
				}
			}
		}
		if(!foundSomething){
			stack.pop_back();
			expandedStates++;
		}
	}
	return ReachabilityResult(ReachabilityResult::NotSatisfied,
							"No state satisfying the query exists.", expandedStates, exploredStates, states.discovered(), enabledTransitionsCount, states.maxTokens(), states.maxPlaceBound());
}

} // Reachability
} // PetriEngine
