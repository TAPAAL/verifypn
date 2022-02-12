/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
 *                     Peter G. Jensen
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

/*
 * File:   ForwardFixedPoint.h
 * Author: Peter G. Jensen
 *
 * Created on 11 February 2022, 22.20
 */

#ifndef FORWARDFIXEDPOINT_H
#define FORWARDFIXEDPOINT_H

#include "ColoredNetStructures.h"
#include "Colors.h"
#include "PartitionBuilder.h"

#include <vector>
#include <unordered_map>
#include <limits>
#include <cinttypes>

namespace PetriEngine {
    class ColoredPetriNetBuilder;
    namespace Colored {

        class ForwardFixedPoint {
        public:
            using VarMap = std::vector<std::unordered_map<const Variable *, interval_vector_t>>;
            using TransitionVariableMap = std::vector<VarMap>;
        private:
            TransitionVariableMap _transition_variable_maps;
            std::vector<bool> _considered;
            ColoredPetriNetBuilder& _builder;
            bool _fixpointDone = false;
            double _fixPointCreationTime;
            size_t _max_intervals = 0;
            std::vector<std::unordered_map<uint32_t, Colored::ArcIntervals>> _arcIntervals;
            std::vector<uint32_t> _placeFixpointQueue;
            std::vector<Colored::ColorFixpoint> _placeColorFixpoints;
            PartitionBuilder& _partition;
            std::unordered_map<uint32_t, Colored::ArcIntervals> setupTransitionVars(size_t tid) const;
            void processInputArcs(const Colored::Transition& transition, uint32_t currentPlaceId, uint32_t transitionId, bool &transitionActivated, uint32_t max_intervals);
            void processOutputArcs(const Colored::Transition& transition, size_t transition_id);
            void removeInvalidVarmaps(size_t tid);
            void addTransitionVars(size_t tid);
            void getArcIntervals(const Colored::Transition& transition, bool &transitionActivated, uint32_t max_intervals, uint32_t transitionId);
            void add_place(const Colored::Place& place);
            void init();
        public:

            ForwardFixedPoint(ColoredPetriNetBuilder& b, PartitionBuilder& partition) : _builder(b), _partition(partition) {
            }

            void printPlaceTable() const;
            void compute(uint32_t maxIntervals, uint32_t maxIntervalsReduced, int32_t timeout);

            double time() const {
                return _fixPointCreationTime;
            }

            bool computed() const {
                return _fixpointDone;
            }

            auto& fixed_point() const {
                return _placeColorFixpoints;
            }

            auto max_intervals() const {
                return _max_intervals;
            }

            void set_default();

            const TransitionVariableMap& variable_map() const {
                return _transition_variable_maps;
            }
        };
    }
}

#endif /* FORWARDFIXEDPOINT_H */

