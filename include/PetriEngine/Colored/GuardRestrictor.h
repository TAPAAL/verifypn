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
                            const VariableModifierMap &varModifierMapL,
                            const VariableModifierMap &varModifierMapR,
                            const PositionVariableMap &varPositionsL,
                            const PositionVariableMap &varPositionsR,
                            const std::unordered_map<uint32_t, const Color*> &constantMapL,
                            const std::unordered_map<uint32_t, const Color*> &constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars,
                            const Colored::Variable *var, 
                            uint32_t index, bool lessthan, bool strict) const;

                

                void restrictEquality(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &varModifierMapL,
                            const VariableModifierMap &varModifierMapR,
                            const PositionVariableMap &varPositionsL,
                            const PositionVariableMap &varPositionsR,
                            const std::unordered_map<uint32_t, const Color*> &constantMapL,
                            const std::unordered_map<uint32_t, const Color*> &constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars) const;

                void restrictInEquality(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &varModifierMapL,
                            const VariableModifierMap &varModifierMapR,
                            const PositionVariableMap &varPositionsL,
                            const PositionVariableMap &varPositionsR,
                            const std::unordered_map<uint32_t, const Color*> &constantMapL,
                            const std::unordered_map<uint32_t, const Color*> &constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars) const;

                void restrictVars(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &varModifierMapL,
                            const VariableModifierMap &varModifierMapR,
                            const PositionVariableMap &varPositionsL,
                            const PositionVariableMap &varPositionsR,
                            const std::unordered_map<uint32_t, const Color*> &constantMapL,
                            const std::unordered_map<uint32_t, const Color*> &constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars, 
                            bool lessthan, bool strict) const;

                interval_vector_t shiftIntervals(const VariableIntervalMap& varMap, 
                            const std::vector<const ColorType *> &colortypes, interval_vector_t &intervals, 
                            int32_t modifier, uint32_t ctSizeBefore) const; 

            private:
                int32_t getVarModifier(const std::unordered_map<uint32_t, int32_t> &modPairMap, uint32_t index) const;
                interval_t getIntervalFromIds(const std::vector<uint32_t> &idVec, uint32_t ctSize, int32_t modifier) const;
                interval_vector_t getIntervalOverlap(const Colored::interval_vector_t &intervals1, const Colored::interval_vector_t &intervals2) const;

                void expandIdVec(const VariableIntervalMap &varMap,
                            const VariableModifierMap &mainVarModifierMap,
                            const VariableModifierMap &otherVarModifierMap,
                            const std::unordered_map<uint32_t, const Variable *> &varPositions,
                            const std::unordered_map<uint32_t, const Color*> &constantMap,
                            const Variable *otherVar, 
                            std::vector<uint32_t> &idVec, size_t targetSize, uint32_t index) const;

                void expandIntervalVec(const VariableIntervalMap &varMap,
                            const VariableModifierMap &mainVarModifierMap,
                            const VariableModifierMap &otherVarModifierMap,
                            const std::unordered_map<uint32_t, const Variable *> &varPositions,
                            const std::unordered_map<uint32_t, const Color*> &constantMap,
                            const Variable *otherVar, 
                            interval_vector_t &intervalVec, size_t targetSize, uint32_t index) const;

                void restrictByConstant(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &mainVarModifierMap,
                            const VariableModifierMap &otherVarModifierMap,
                            const PositionVariableMap &varPositions,
                            const std::unordered_map<uint32_t, const Color*> &constantMap,
                            const Colored::Variable *var,
                            const Colored::Variable *otherVar, 
                            uint32_t index, bool lessthan, bool strict) const;

                void restrictEqByConstant(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &mainVarModifierMap,
                            const VariableModifierMap &otherVarModifierMap,
                            const PositionVariableMap &varPositions,
                            const std::unordered_map<uint32_t, const Color*> &constantMap,
                            const Colored::Variable *var,
                            uint32_t index) const;

                void restrictEqDiagonal(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &varModifierMapL,
                            const VariableModifierMap &varModifierMapR,
                            const PositionVariableMap &varPositionsL,
                            const PositionVariableMap &varPositionsR,
                            const std::unordered_map<uint32_t, const Color*> &constantMapL,
                            const std::unordered_map<uint32_t, const Color*> &constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars,
                            const Colored::Variable *var, 
                            uint32_t index) const;    
        };
    }
}


#endif /* GuardRestrictions_H */