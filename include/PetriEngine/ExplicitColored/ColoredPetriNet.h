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
        struct ColoredPetriNetTransition
        {
            std::unique_ptr<GuardExpression> guardExpression;
            std::pair<std::map<Variable_t,std::vector<uint32_t>>, uint32_t> validVariables;
        };

        struct BaseColorType
        {
            Color_t colors;
        };

        struct ColorType
        {
            uint32_t size;
            std::vector<std::shared_ptr<BaseColorType>> basicColorTypes;
        };

        struct ColoredPetriNetPlace
        {
            std::shared_ptr<ColorType> colorType;
        };

        struct ColoredPetriNetInhibitor
        {
            ColoredPetriNetInhibitor(size_t from, size_t to, MarkingCount_t weight)
                : from(from), to(to), weight(weight){}
            uint32_t from;
            uint32_t to;
            MarkingCount_t weight;
        };

        struct ColoredPetriNetArc
        {
            uint32_t from;
            uint32_t to;
            std::shared_ptr<ColorType> colorType;
            std::unique_ptr<ArcExpression> arcExpression;
        };

        struct Variable
        {
            std::shared_ptr<BaseColorType> colorType;
        };

        class ColoredPetriNetBuilder;

        class ColoredPetriNet
        {
        public:
            ColoredPetriNet(ColoredPetriNet&&) = default;
            ColoredPetriNet& operator=(ColoredPetriNet&&) = default;
            const ColoredPetriNetMarking& initial() const {
                return _initialMarking;
            }

        private:
            friend class ColoredPetriNetBuilder;
            friend class ColoredSuccessorGenerator;
            friend class ValidVariableGenerator;
            ColoredPetriNet() = default;
            std::vector<ColoredPetriNetTransition> _transitions;
            std::vector<ColoredPetriNetPlace> _places;
            std::vector<ColoredPetriNetArc> _arcs;
            std::vector<ColoredPetriNetInhibitor> _inhibitorArcs;
            std::vector<Variable> _variables;
            ColoredPetriNetMarking _initialMarking;
            std::vector<std::pair<uint32_t,uint32_t>> _transitionArcs; //Index is transition and pair is input/output arc beginning index in _arcs
            std::vector<uint32_t> _transitionInhibitors; //Index is transition and value is beginning index in _inhibitorArcs
        };
    }
} // PetriEngine

#endif // COLOREDPETRINET_H
