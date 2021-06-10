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

#ifndef GuardRestrictions_H
#define GuardRestrictions_H

#include "Colors.h"
#include "Multiset.h"
#include <unordered_map>
#include <set>
#include <stdlib.h>

namespace PetriEngine {
    namespace Colored {

        class GuardRestrictor {
            public:

                GuardRestrictor();

                void restrictDiagonal(std::vector<VariableIntervalMap>& variableMap,
                            VariableModifierMap *varModifierMapL,
                            VariableModifierMap *varModifierMapR,
                            PositionVariableMap *varPositionsL,
                            PositionVariableMap *varPositionsR,
                            std::unordered_map<uint32_t, const Color*> *constantMapL,
                            std::unordered_map<uint32_t, const Color*> *constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars,
                            const Colored::Variable *var, 
                            uint32_t index, bool lessthan, bool strict);

                

                void restrictEquality(std::vector<VariableIntervalMap>& variableMap,
                            VariableModifierMap *varModifierMapL,
                            VariableModifierMap *varModifierMapR,
                            PositionVariableMap *varPositionsL,
                            PositionVariableMap *varPositionsR,
                            std::unordered_map<uint32_t, const Color*> *constantMapL,
                            std::unordered_map<uint32_t, const Color*> *constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars);

                void restrictVars(std::vector<VariableIntervalMap>& variableMap,
                            VariableModifierMap *varModifierMapL,
                            VariableModifierMap *varModifierMapR,
                            PositionVariableMap *varPositionsL,
                            PositionVariableMap *varPositionsR,
                            std::unordered_map<uint32_t, const Color*> *constantMapL,
                            std::unordered_map<uint32_t, const Color*> *constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars, 
                            bool lessthan, bool strict);

                intervalTuple_t shiftIntervals(VariableIntervalMap& varMap, 
                            std::vector<const ColorType *> *colortypes, intervalTuple_t *intervals, 
                            int32_t modifier, uint32_t ctSizeBefore) const; 

            private:
                int32_t getVarModifier(std::unordered_map<uint32_t, int32_t> *modPairMap, uint32_t index);
                interval_t getIntervalFromIds(std::vector<uint32_t> *idVec, uint32_t ctSize, int32_t modifier);
                intervalTuple_t getIntervalOverlap(std::vector<Colored::interval_t> *intervals1, std::vector<Colored::interval_t> *intervals2);

                void expandIdVec(VariableIntervalMap *varMap,
                            VariableModifierMap *mainVarModifierMap,
                            VariableModifierMap *otherVarModifierMap,
                            std::unordered_map<uint32_t, const Variable *> *varPositions,
                            std::unordered_map<uint32_t, const Color*> *constantMap,
                            const Variable *otherVar, 
                            std::vector<uint32_t> &idVec, size_t targetSize, uint32_t index);

                void expandIntervalVec(VariableIntervalMap varMap,
                            VariableModifierMap *mainVarModifierMap,
                            VariableModifierMap *otherVarModifierMap,
                            std::unordered_map<uint32_t, const Variable *> *varPositions,
                            std::unordered_map<uint32_t, const Color*> *constantMap,
                            const Variable *otherVar, 
                            std::vector<interval_t> &intervalVec, size_t targetSize, uint32_t index);

                void restrictByConstant(std::vector<VariableIntervalMap>& variableMap,
                            VariableModifierMap *mainVarModifierMap,
                            VariableModifierMap *otherVarModifierMap,
                            PositionVariableMap *varPositions,
                            std::unordered_map<uint32_t, const Color*> *constantMap,
                            const Colored::Variable *var,
                            const Colored::Variable *otherVar, 
                            uint32_t index, bool lessthan, bool strict);

                void restrictEqByConstant(std::vector<VariableIntervalMap>& variableMap,
                            VariableModifierMap *mainVarModifierMap,
                            VariableModifierMap *otherVarModifierMap,
                            PositionVariableMap *varPositions,
                            std::unordered_map<uint32_t, const Color*> *constantMap,
                            const Colored::Variable *var,
                            uint32_t index);

                void restrictEqDiagonal(std::vector<VariableIntervalMap>& variableMap,
                            VariableModifierMap *varModifierMapL,
                            VariableModifierMap *varModifierMapR,
                            PositionVariableMap *varPositionsL,
                            PositionVariableMap *varPositionsR,
                            std::unordered_map<uint32_t, const Color*> *constantMapL,
                            std::unordered_map<uint32_t, const Color*> *constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars,
                            const Colored::Variable *var, 
                            uint32_t index);    
        };
    }
}


#endif /* GuardRestrictions_H */