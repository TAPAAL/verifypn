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

#include "PQL.h"

#include "../PetriNet.h"
#include "../Simplification/LPCache.h"
#include "../NetStructures.h"
#include "utils/structures/shared_string.h"

#include "utils/errors.h"

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
            const shared_name_index_map& _placeNames;
            const shared_name_index_map& _transitionNames;
            const PetriNet* _net = nullptr;
            std::unordered_map<std::string, uint32_t> _trace_names;
        public:

            /** A resolution result */
            struct ResolutionResult {
                /** Offset in relevant vector */
                int offset;
                /** True, if the resolution was successful */
                bool success;
            };

            AnalysisContext(const shared_name_index_map& places, const shared_name_index_map& tnames, const PetriNet* net)
            : _placeNames(places), _transitionNames(tnames), _net(net) {

            }

            virtual void setHasDeadlock(){};

            const PetriNet* net() const
            {
                return _net;
            }

            /** Resolve an identifier */
            virtual ResolutionResult resolve(const shared_const_string& identifier, bool place = true);

            uint32_t resolve_trace_name(const std::string& s, bool create);

            auto& allPlaceNames() const { return _placeNames; }
            auto& allTransitionNames() const { return _transitionNames; }

        };

        class ColoredAnalysisContext : public AnalysisContext {
        protected:
            const shared_place_color_map& _coloredPlaceNames;
            const shared_name_name_map& _coloredTransitionNames;

            bool _colored;

        public:
            ColoredAnalysisContext(const shared_name_index_map& places,
                                   const shared_name_index_map& tnames,
                                   const PetriNet* net,
                                   const shared_place_color_map& cplaces,
                                   const shared_name_name_map& ctnames,
                                   bool colored)
                    : AnalysisContext(places, tnames, net),
                      _coloredPlaceNames(cplaces),
                      _coloredTransitionNames(ctnames),
                      _colored(colored)
            {}

            bool resolvePlace(const shared_const_string& place, std::function<void(const shared_const_string&)>&& fn);

            bool resolveTransition(const shared_const_string& transition, std::function<void(const shared_const_string)>&& fn);

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
                return &_marking[_offset];
            }

            void setMarking(MarkVal* marking) {
                _marking = marking;
            }

            const PetriNet* net() const {
                return _net;
            }

            void set_offset(size_t i) {
                _offset = i;
            }

        private:
            const MarkVal* _marking = nullptr;
            const PetriNet* _net = nullptr;
            size_t _offset = 0;
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
                    Simplification::LPCache* cache, uint32_t potencyTimeout = 0)
                    : _queryTimeout(queryTimeout), _lpTimeout(lpTimeout),
                    _potencyTimeout(potencyTimeout) {
                _negated = false;
                _marking = marking;
                _net = net;
                _base_lp = buildBase();
                _start = std::chrono::high_resolution_clock::now();
                _cache = cache;
                _markingOutOfBounds = false;
                for(size_t i = 0; i < net->numberOfPlaces(); ++i) {
                    if (marking[i] >  std::numeric_limits<int32_t>::max()) { //too many tokens exceeding int32_t limits, LP solver will give wrong results
                        _markingOutOfBounds = true;
                    }
                }
            }

            virtual ~SimplificationContext() {
                if(_base_lp != nullptr)
                    glp_delete_prob(_base_lp);
                _base_lp = nullptr;
            }


            const MarkVal* marking() const {
                return _marking;
            }

            bool markingOutOfBounds() const {
                return _markingOutOfBounds;
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

            bool potencyTimeout() const {
                auto end = std::chrono::high_resolution_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _start);
                return (diff.count() >= _potencyTimeout);
            }

            uint32_t getLpTimeout() const;
            uint32_t getPotencyTimeout() const;

            Simplification::LPCache* cache() const
            {
                return _cache;
            }

            glp_prob* makeBaseLP() const;

        private:
            bool _negated;
            const MarkVal* _marking;
            bool _markingOutOfBounds;
            const PetriNet* _net;
            uint32_t _queryTimeout, _lpTimeout, _potencyTimeout;
            mutable glp_prob* _base_lp = nullptr;
            std::chrono::high_resolution_clock::time_point _start;
            Simplification::LPCache* _cache;

            glp_prob* buildBase() const;
        };

    } // PQL
} // PetriEngine

#endif // CONTEXTS_H
