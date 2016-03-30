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
#ifndef REACHABILITYRESULT_H
#define REACHABILITYRESULT_H

#include <vector>
#include "../PQL/PQL.h"

struct options_t;

namespace PetriEngine {
    class PetriNetBuilder;
    namespace Reachability {

        /** Result of a reachability search */

        class ResultPrinter {
        private:
            PetriNetBuilder* builder;
            options_t* options;
            std::vector<std::string>& querynames;
        public:
                        /** Types of results */
            enum Result {
                /** The query was satisfied */
                Satisfied,
                /** The query cannot be satisfied */
                NotSatisfied,
                /** We're unable to say if the query can be satisfied */
                Unknown
            };
            
            ResultPrinter(PetriNetBuilder* b, options_t* o, std::vector<std::string>& querynames) 
            : builder(b), options(o), querynames(querynames)
            {};
            
            Result printResult(
                size_t index,
                PQL::Condition* query, 
                ResultPrinter::Result result = Unknown,
                size_t expandedStates = 0,
                size_t exploredStates = 0,
                size_t discoveredStates = 0,
                const std::vector<size_t> enabledTransitionsCount = std::vector<size_t>(),
                int maxTokens = 0,
                const std::vector<uint32_t> maxPlaceBound = std::vector<uint32_t>());

        };
    } // Reachability
} // PetriEngine

#endif // REACHABILITYRESULT_H
