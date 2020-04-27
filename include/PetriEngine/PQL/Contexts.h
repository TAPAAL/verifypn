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
#ifndef CONTEXTS_H
#define CONTEXTS_H

#include "../PetriNet.h"
#include "../Simplification/LPCache.h"
#include "PQL.h"
#include "../NetStructures.h"

#include <string>
#include <vector>
#include <list>
#include <map>
#include <chrono>
#include <glpk.h>

namespace PetriEngine {
    namespace PQL {

        /** Context provided for context analysis */
        class AnalysisContext {
        protected:
            const unordered_map<std::string, uint32_t>& _placeNames;
            const unordered_map<std::string, uint32_t>& _transitionNames;
            const PetriNet* _net;
            std::vector<ExprError> _errors;
        public:

            /** A resolution result */
            struct ResolutionResult {
                /** Offset in relevant vector */
                int offset;
                /** True, if the resolution was successful */
                bool success;
            };

            AnalysisContext(const std::unordered_map<std::string, uint32_t>& places, const std::unordered_map<std::string, uint32_t>& tnames, const PetriNet* net)
            : _placeNames(places), _transitionNames(tnames), _net(net) {

            }
            
            virtual void setHasDeadlock(){};
            
            const PetriNet* net() const
            {
                return _net;
            }
            
            /** Resolve an identifier */
            virtual ResolutionResult resolve(const std::string& identifier, bool place = true);

            /** Report error */
            void reportError(const ExprError& error) {
                _errors.push_back(error);
            }

            /** Get list of errors */
            const std::vector<ExprError>& errors() const {
                return _errors;
            }
            auto& allPlaceNames() const { return _placeNames; }
            auto& allTransitionNames() const { return _transitionNames; }

        };

        class ColoredAnalysisContext : public AnalysisContext {
        protected:
            const std::unordered_map<std::string, std::unordered_map<uint32_t , std::string>>& _coloredPlaceNames;
            const std::unordered_map<std::string, std::vector<std::string>>& _coloredTransitionNames;

            bool _colored;

        public:
            ColoredAnalysisContext(const std::unordered_map<std::string, uint32_t>& places,
                                   const std::unordered_map<std::string, uint32_t>& tnames,
                                   const PetriNet* net,
                                   const std::unordered_map<std::string, std::unordered_map<uint32_t , std::string>>& cplaces,
                                   const std::unordered_map<std::string, std::vector<std::string>>& ctnames,
                                   bool colored)
                    : AnalysisContext(places, tnames, net),
                      _coloredPlaceNames(cplaces),
                      _coloredTransitionNames(ctnames),
                      _colored(colored)
            {}

            bool resolvePlace(const std::string& place, std::unordered_map<uint32_t,std::string>& out);

            bool resolveTransition(const std::string& transition, std::vector<std::string>& out);

            bool isColored() const {
                return _colored;
            }
            auto& allColoredPlaceNames() const { return _coloredPlaceNames; }
            auto& allColoredTransitionNames() const { return _coloredTransitionNames; }
        };

        /** Context provided for evalation */
        class EvaluationContext {
        public:

            /** Create evaluation context, this doesn't take ownership */
            EvaluationContext(const MarkVal* marking,
                    const PetriNet* net) {
                _marking = marking;
                _net = net;
            }
            
            EvaluationContext() {};

            const MarkVal* marking() const {
                return _marking;
            }
            
            void setMarking(MarkVal* marking) {
                _marking = marking;
            }

            const PetriNet* net() const {
                return _net;
            }
        private:
            const MarkVal* _marking = nullptr;
            const PetriNet* _net = nullptr;
        };

        /** Context for distance computation */
        class DistanceContext : public EvaluationContext {
        public:

            DistanceContext(const PetriNet* net,
                    const MarkVal* marking)
            : EvaluationContext(marking, net) {
                _negated = false;
            }


            void negate() {
                _negated = !_negated;
            }

            bool negated() const {
                return _negated;
            }

        private:
            bool _negated;
        };

        /** Context for condition to TAPAAL export */
        class TAPAALConditionExportContext {
        public:
            bool failed;
            std::string netName;
        };

        class SimplificationContext {
        public:

            SimplificationContext(const MarkVal* marking,
                    const PetriNet* net, uint32_t queryTimeout, uint32_t lpTimeout,
                    LPCache* cache)
                    : _queryTimeout(queryTimeout), _lpTimeout(lpTimeout) {
                _negated = false;
                _marking = marking;
                _net = net;
                _base_lp = buildBase();
                _start = std::chrono::high_resolution_clock::now();
                _cache = cache;
            }
                    
            virtual ~SimplificationContext() {
                if(_base_lp != nullptr)
                    glp_delete_prob(_base_lp);
                _base_lp = nullptr;
            }


            const MarkVal* marking() const {
                return _marking;
            }

            const PetriNet* net() const {
                return _net;
            }

            void negate() {
                _negated = !_negated;
            }

            bool negated() const {
                return _negated;
            }
            
            void setNegate(bool b){
                _negated = b;
            }
            
            double getReductionTime();
            
            bool timeout() const {
                auto end = std::chrono::high_resolution_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _start);
                return (diff.count() >= _queryTimeout);
            }
            
            uint32_t getLpTimeout() const;
            
            LPCache* cache() const
            {
                return _cache;
            }
            
            
            glp_prob* makeBaseLP() const;

        private:
            bool _negated;
            const MarkVal* _marking;
            const PetriNet* _net;
            uint32_t _queryTimeout, _lpTimeout;
            std::chrono::high_resolution_clock::time_point _start;
            LPCache* _cache;
            mutable glp_prob* _base_lp = nullptr;

            glp_prob* buildBase() const;

        };

    } // PQL
} // PetriEngine

#endif // CONTEXTS_H
