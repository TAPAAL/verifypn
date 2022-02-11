/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
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

