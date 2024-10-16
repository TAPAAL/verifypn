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
#include "CPNMultiSet.h"
#include "GuardExpression.h"
#include "Binding.h"
#include "ColoredPetriNetMarking.h"
#include "ArcExpression.h"

namespace PetriEngine
{
    namespace ExplicitColored
    {
        struct ArcExpression
        {
            std::vector<std::shared_ptr<Variable>> variables;
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

        struct ColoredPetriNetInhibitor
        {
            int from;
            int to;
            MarkingCount_t weight;
        };

        struct ColoredPetriNetArc
        {
            int from;
            int to;
            std::shared_ptr<ColorType> colorType;
            std::unique_ptr<ArcExpression> arcExpression;
        };

        struct ColorType
        {
            uint32_t size;
            std::vector<std::shared_ptr<BaseColorType>> colorTypes;
        };

        struct BaseColorType
        {
            Color_t colors;
        };

        struct Variable
        {
            std::shared_ptr<BaseColorType> colorType;
            Variable_t id;
        };

        class ColoredPetriNet
        {
        public:
            ColoredPetriNet();
            const ColoredPetriNetMarking& initial() const {
                return _initialMarking;
            }
        private:
            std::vector<ColoredPetriNetTransition> _transitions;
            std::vector<ColoredPetriNetPlace> _places;
            std::vector<ColoredPetriNetArc> _transitionToPlaceArcs;
            std::vector<ColoredPetriNetInhibitor> _inhibitorToPlaceArcs;
            std::vector<ColoredPetriNetArc> _placeToTransitionArcs;
            std::vector<Variable> _variables;
            ColoredPetriNetMarking _initialMarking;
            uint32_t _ntransitions;
            friend class ColoredSuccessorGenerator;
        };
    }
} // PetriEngine

#endif // COLOREDPETRINET_H
