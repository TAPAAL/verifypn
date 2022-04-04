/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
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

#ifndef VERIFYPN_ENABLEDSPOOLER_H
#define VERIFYPN_ENABLEDSPOOLER_H

#include "LTL/SuccessorGeneration/SuccessorSpooler.h"
#include "PetriEngine/SuccessorGenerator.h"

namespace LTL {
    class EnabledSpooler : public SuccessorSpooler {
    public:
        EnabledSpooler(const PetriEngine::PetriNet& net, PetriEngine::SuccessorGenerator &sucGen)
                : _successorGenerator(sucGen)
        {
            _marking.setMarking(new PetriEngine::MarkVal[net.numberOfPlaces()]);
        }

        bool prepare(const LTL::Structures::ProductState *state) override
        {
            return _successorGenerator.prepare(state);
        }

        uint32_t next() override
        {
            // TODO don't need to actually fire the transition, merely spool to next.
            // this is a non-trivial refactor in SuccessorGenerator, but seems natural.
            if (_successorGenerator.next(_marking)) {
                return _successorGenerator.fired();
            } else {
                return NoTransition;
            }
        }

    private:
        PetriEngine::SuccessorGenerator &_successorGenerator;
        PetriEngine::Structures::State _marking;
    };
}

#endif //VERIFYPN_ENABLEDSPOOLER_H
