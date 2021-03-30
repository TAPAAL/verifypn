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

#ifndef VERIFYPN_REACHABILITYSTUBBORNSPOOLER_H
#define VERIFYPN_REACHABILITYSTUBBORNSPOOLER_H

#include "LTL/SuccessorGeneration/SuccessorSpooler.h"
#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"

namespace LTL {
    class ReachabilityStubbornSpooler : public SuccessorSpooler {
    public:
        ReachabilityStubbornSpooler(const PetriEngine::PetriNet &net)
                : _stubborn(net) {}

        bool prepare(const LTL::Structures::ProductState *state) override
        {
            return _stubborn.prepare(state);
        }

        uint32_t next() override
        {
            return _stubborn.next();
        }

        void reset() override
        {
            _stubborn.reset();
        }

    private:
        PetriEngine::ReachabilityStubbornSet _stubborn;
    };
}
#endif //VERIFYPN_REACHABILITYSTUBBORNSPOOLER_H
