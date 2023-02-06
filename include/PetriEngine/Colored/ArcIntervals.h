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

#ifndef ARCINTERVALS_H
#define ARCINTERVALS_H

#include "Colors.h"

namespace PetriEngine {
    namespace Colored {

        struct ArcIntervals {
            VariableModifierMap _varIndexModMap;
            std::vector<Colored::interval_vector_t> _intervalTupleVec;

            ~ArcIntervals() {_varIndexModMap.clear();}
            ArcIntervals() {
            }

            ArcIntervals(VariableModifierMap varIndexModMap)
            : _varIndexModMap(std::move(varIndexModMap)) {}

            ArcIntervals(VariableModifierMap&& varIndexModMap,  std::vector<Colored::interval_vector_t> ranges)
            : _varIndexModMap(std::move(varIndexModMap)), _intervalTupleVec(std::move(ranges)) {
            };

            void print() {
                std::cout << "[ ";
                for(auto& varModifierPair : _varIndexModMap){
                    std::cout << "(" << varModifierPair.first->name << ", " << varModifierPair.first->colorType->productSize() <<  ") ";
                }
                std::cout << "]" << std::endl;
                for(auto intervalTuple: _intervalTupleVec){
                    std::cout << "--------------------------------------------------------------------" << std::endl;
                    intervalTuple.print();
                    std::cout << "--------------------------------------------------------------------" << std::endl;
                }
            }
        };
    }
}

#endif /* INTERVALGENERATOR_H */