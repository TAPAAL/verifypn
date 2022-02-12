/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
 *                     Peter G. Jensen <root@petergjoel.dk>
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
 * File:   VariableSymmetry.h
 * Author: Peter G. Jensen
 *
 * Created on 11 February 2022, 23.57
 */

#ifndef VARIABLESYMMETRY_H
#define VARIABLESYMMETRY_H

#include "Colors.h"

#include "ColoredNetStructures.h"
#include "PartitionBuilder.h"

#include <vector>
#include <set>

namespace PetriEngine {
    class ColoredPetriNetBuilder;
    namespace Colored {
        class VariableSymmetry {
            using SymVarMap = std::vector<std::vector<std::set<const Colored::Variable *>>>;
            const ColoredPetriNetBuilder& _builder;
            //transition id to vector of vectors of variables, where variable in vector are symmetric
            SymVarMap _symmetric_var_map;
            PartitionBuilder& _partition;
            void checkSymmetricVarsInArcs(const Colored::Transition &transition, const Colored::Arc &inArc, const std::set<const Colored::Variable*> &inArcVars, bool &isEligible) const;
            void checkSymmetricVarsOutArcs(const Colored::Transition &transition, const std::set<const Colored::Variable*> &inArcVars, bool &isEligible) const;
        public:
            VariableSymmetry(const ColoredPetriNetBuilder& b, PartitionBuilder& partition)
                    : _builder(b), _partition(partition) {}

            void compute();
            void printSymmetricVariables() const;
            const SymVarMap& symmetries() const { return _symmetric_var_map; }
        };
    }
}


#endif /* VARIABLESYMMETRY_H */

