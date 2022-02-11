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
 * File:   StablePlaceFinder.h
 * Author: pgj
 *
 * Created on 11 February 2022, 21.51
 */

#ifndef STABLEPLACEFINDER_H
#define STABLEPLACEFINDER_H

#include <vector>

namespace PetriEngine {
    class ColoredPetriNetBuilder;
    namespace Colored {

        class StablePlaceFinder {
        private:
            std::vector<bool> _stable;
            const ColoredPetriNetBuilder& _builder;
        public:

            StablePlaceFinder(const ColoredPetriNetBuilder& b) : _builder(b) {
            }

            StablePlaceFinder(const ColoredPetriNetBuilder& b, const StablePlaceFinder& other)
                    : _builder(b), _stable(other._stable) {}

            void compute();

            bool operator[](size_t i) const {
                if(i >= _stable.size()) return false;
                return _stable[i];
            }
        };
    }
}


#endif /* STABLEPLACEFINDER_H */

