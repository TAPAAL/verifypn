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
#ifndef REACHABILITYRESULT_H
#define REACHABILITYRESULT_H

#include <vector>
#include "../PQL/PQL.h"
#include "../Structures/StateSet.h"
#include "../Reducer.h"

struct options_t;

namespace PetriEngine {
    class PetriNetBuilder;
    namespace Reachability {

        /** Result of a reachability search */

        class AbstractHandler {
        public:
            enum Result {
                /** The query was satisfied */
                Satisfied,
                /** The query cannot be satisfied */
                NotSatisfied,
                /** We're unable to say if the query can be satisfied */
                Unknown,
                /** The query should be verified using the CTL engine */
                CTL,
                /** The query should be verified using the LTL engine */
                LTL,
                /** A Synthesis query*/
                Synthesis,
                /** Just ignore */
                Ignore
            };
            virtual std::pair<Result, bool> handle(
                size_t index,
                PQL::Condition* query,
                Result result,
                const std::vector<uint32_t>* maxPlaceBound = nullptr,
                size_t expandedStates = 0,
                size_t exploredStates = 0,
                size_t discoveredStates = 0,
                int maxTokens = 0,
                Structures::StateSetInterface* stateset = nullptr, size_t lastmarking = 0, const MarkVal* initialMarking = nullptr, bool trace = true) = 0;
        };

        class ResultPrinter : public AbstractHandler {
        protected:
            PetriNetBuilder* builder;
            options_t* options;
            std::vector<std::string>& querynames;
            Reducer* reducer;

            std::string printTechniques();
            void printTrace(Structures::StateSetInterface*, size_t lastmarking);

        public:
            const std::string techniques = "TECHNIQUES COLLATERAL_PROCESSING STRUCTURAL_REDUCTION QUERY_REDUCTION SAT_SMT ";
            const std::string techniquesStateSpace = "TECHNIQUES EXPLICIT STATE_COMPRESSION";

            ResultPrinter(PetriNetBuilder* b, options_t* o, std::vector<std::string>& querynames)
            : builder(b), options(o), querynames(querynames), reducer(NULL)
            {};

            void setReducer(Reducer* r) { this->reducer = r; }

            std::pair<Result, bool> handle(
                size_t index,
                PQL::Condition* query,
                Result result,
                const std::vector<uint32_t>* maxPlaceBound = nullptr,
                size_t expandedStates = 0,
                size_t exploredStates = 0,
                size_t discoveredStates = 0,
                int maxTokens = 0,
                Structures::StateSetInterface* stateset = nullptr, size_t lastmarking = 0, const MarkVal* initialMarking = nullptr, bool trace = true) override;
        };

        // The only purpose of this class is to pretty-print the "Solved by Trace
        // abstraction refinement" after a query has been solved.
        // The entire result-printer workflow is overdue a refactor.
        class TarResultPrinter : public AbstractHandler {
        private:
            ResultPrinter& _printer;
        public:
            TarResultPrinter(ResultPrinter& printer) : _printer(printer) {}
            std::pair<Result, bool> handle(
                size_t index,
                PQL::Condition* query,
                Result result,
                const std::vector<uint32_t>* maxPlaceBound = nullptr,
                size_t expandedStates = 0,
                size_t exploredStates = 0,
                size_t discoveredStates = 0,
                int maxTokens = 0,
                Structures::StateSetInterface* stateset = nullptr, size_t lastmarking = 0, const MarkVal* initialMarking = nullptr, bool trace = true) override;
        };
    } // Reachability
} // PetriEngine

#endif // REACHABILITYRESULT_H
