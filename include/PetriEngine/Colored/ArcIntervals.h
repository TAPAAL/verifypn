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
            std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>> _varIndexModMap;
            std::vector<Colored::intervalTuple_t> _intervalTupleVec;
            Colored::ColorFixpoint * _source;
            
            ~ArcIntervals() {_varIndexModMap.clear();}
            ArcIntervals() {
            }

            ArcIntervals(Colored::ColorFixpoint * source) : _source(source){
            }

            ArcIntervals(Colored::ColorFixpoint * source, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>> varIndexModMap) : _varIndexModMap(varIndexModMap), _source(source) {
            };

            ArcIntervals(Colored::ColorFixpoint * source, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>> varIndexModMap,  std::vector<Colored::intervalTuple_t> ranges) : _varIndexModMap(varIndexModMap), _intervalTupleVec(ranges), _source(source) {
            };

            size_t size() {
                return _intervalTupleVec.size();
            }

            Colored::intervalTuple_t& operator[] (size_t index) {
                return _intervalTupleVec[index];
            }
            
            Colored::intervalTuple_t& operator[] (int index) {
                return _intervalTupleVec[index];
            }
            
            Colored::intervalTuple_t& operator[] (uint32_t index) {
                assert(index < _intervalTupleVec.size());
                return _intervalTupleVec[index];
            }

            Colored::intervalTuple_t& back(){
                return _intervalTupleVec.back();
            }

            bool hasValidIntervals(){
                for(auto intervalTuple : _intervalTupleVec){
                    if (intervalTuple.hasValidIntervals()){
                        return true;
                    }
                }
                return false;
            }

            bool containsVariable(Colored::Variable * var){
                for (auto varModPair : _varIndexModMap){
                    if(varModPair.first == var){
                        return true;
                    }
                }
                return false;
            }

            std::set<const Colored::Variable *> getVariables(){
                std::set<const Colored::Variable *> res;
                for (auto varModPair : _varIndexModMap){
                    res.insert(varModPair.first);
                }
                return res;
            }

            void print() {
                std::cout << "[ ";
                for(auto varModifierPair : _varIndexModMap){
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