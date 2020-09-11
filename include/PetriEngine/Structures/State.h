/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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
#ifndef GENERALSTATE_H
#define GENERALSTATE_H

#include "../PetriNet.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <vector>

namespace PetriEngine {
    namespace Structures {

        /** GeneralState class for reachability searches.
         * Used in most reachability search cases */
        class State {
        public:

            MarkVal* marking() {
                return _marking;
            }

            const MarkVal* marking() const {
                return _marking;
            }

            void setMarking(MarkVal* m) {
                _marking = m;
            }
            
            State()
            {
                _marking = NULL;
            }
            
            ~State()
            {
                delete[] _marking;
            }
            
            void swap(State& other)
            {
                std::swap(_marking, other._marking);
            }
            
            void print(PetriNet& net) {
                for (uint32_t i = 0; i < net.numberOfPlaces(); i++) {
                    if(_marking[i])
                        std::cout << net.placeNames()[i] << ": " << _marking[i] << std::endl;   
                }
                std::cout << std::endl;
            }
        private:
            MarkVal* _marking;
        };

    }
}

#endif //GENERALSTATE_H
