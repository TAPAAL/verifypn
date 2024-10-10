/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef COLOREDPETRINET_H
#define COLOREDPETRINET_H

#include <string>
#include <vector>
#include <climits>
#include <limits>
#include <memory>
#include <iostream>

#include "utils/structures/shared_string.h"
#include "AtomicTypes.h"
#include "MultiSet.h"
#include "GuardExpression.h"
#include "Binding.h"

namespace PetriEngine
{
    namespace ExplicitColored
    {
        struct ArcExpression
        {
            virtual CPNMultiSet eval(const Binding &binding) = 0;
        };

        struct ColoredPetriNetTransition
        {
            std::unique_ptr<GuardExpression> guardExpression;
        };

        struct ColoredPetriNetPlace
        {
            std::shared_ptr<ColorType> colorType;
        };

        struct ColoredPetriNetArc
        {
            int from;
            int to;
            std::unique_ptr<ArcExpression> arcExpression;
        };

        struct ColorType
        {
            std::vector<int> colors;
        };

        struct ColoredPetriNetMarking
        {
            std::vector<CPNMultiSet> markings;
        };

        class ColoredPetriNet
        {
        public:
            ColoredPetriNet();

        private:
            std::vector<ColoredPetriNetTransition> _transitions;
            std::vector<ColoredPetriNetPlace> _places;
            std::vector<ColoredPetriNetArc> _transitionToPlaceArcs;
            std::vector<ColoredPetriNetArc> _placeToTransitionArcs;
            ColoredPetriNetMarking _initialMarking;
        };
    }
} // PetriEngine

#endif // COLOREDPETRINET_H
