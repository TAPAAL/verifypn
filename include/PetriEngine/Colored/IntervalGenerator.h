/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
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
#ifndef INTERVALGENERATOR_H
#define INTERVALGENERATOR_H

#include "ColoredNetStructures.h"

namespace PetriEngine {
    namespace Colored {
        class IntervalGenerator {
            public:
                static bool getVarIntervals(std::vector<VariableIntervalMap>& variableMaps, const std::unordered_map<uint32_t, ArcIntervals> &placeArcIntervals);
            private:

                static std::vector<interval_t> getIntervalsFromInterval(const interval_t &interval, uint32_t varPosition, int32_t varModifier, const std::vector<const ColorType*> &varColorTypes);

                static void getArcVarIntervals(interval_vector_t& varIntervals, const std::unordered_map<uint32_t, int32_t> &modIndexMap, const interval_t &interval, const std::vector<const ColorType*> &varColorTypes);

                static void populateLocalMap(const ArcIntervals &arcIntervals,
                                    const VariableIntervalMap &varMap,
                                    VariableIntervalMap &localVarMap,
                                    const interval_t &interval, bool& allVarsAssigned,  uint32_t tuplePos);

                static void fillVarMaps(std::vector<VariableIntervalMap> &variableMaps,
                                                    const ArcIntervals &arcIntervals,
                                                    const uint32_t &intervalTupleSize,
                                                    const uint32_t &tuplePos);
        };
    }
}

#endif /* INTERVALGENERATOR_H */