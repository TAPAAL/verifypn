/* Copyright (C) 2021  Peter G. Jensen <root@petergjoel.dk>,
 *                     Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#include "LTL/SuccessorGeneration/CompoundGenerator.h"
#include "PetriEngine/Structures/State.h"
#include "utils/errors.h"

#include <cassert>

namespace LTL {
    using namespace PetriEngine;

    CompoundGenerator::CompoundGenerator(const PetriNet& net, size_t hyper_traces)
    : _generator(net), _hyper_traces(hyper_traces) {
    }

    void CompoundGenerator::prepare(const Structures::State* state, const successor_info_t &sucinfo) {
        _parent = state;
    }

    bool CompoundGenerator::next(PetriEngine::Structures::State &write, successor_info_t &sucinfo) {
        if (sucinfo.fresh()) {
            // after this call, everything will be primed!
            sucinfo._pcounter.resize(_hyper_traces, 0);
            sucinfo._tcounter.resize(_hyper_traces, std::numeric_limits<uint32_t>::max());
            return increment(write, sucinfo, true);
        }
        else
        {
            return increment(write, sucinfo, false);
        }
    }

    bool CompoundGenerator::increment(PetriEngine::Structures::State &write, successor_info_t &sucinfo, bool initial) {
        bool has_succ = false;
        for (size_t i = 0; i < _hyper_traces; ++i) {
            PetriEngine::Structures::State working(const_cast<PetriEngine::MarkVal*>(_parent->marking()) + (i * _generator.net().numberOfPlaces()));
            _generator.prepare(working, sucinfo._pcounter[i], sucinfo._tcounter[i]);
            PetriEngine::Structures::State tmp(write.marking() + i * _generator.net().numberOfPlaces());
            bool has_next = _generator.next(tmp);
            tmp.release();
            working.release();
            if (has_next) // TODO, add specialized has_next to succgen
            {
                std::tie(sucinfo._pcounter[i], sucinfo._tcounter[i]) = _generator.state();
                has_succ = true;
                if(!initial)
                    break;
            } else {
                if (i == _hyper_traces - 1) {
                    // done w. iteration, no more!
                    if(!has_succ)
                    {
                        for (size_t j = 0; j < _hyper_traces; ++j)
                            sucinfo._pcounter[i] = _generator.net().numberOfPlaces();
                        break;
                    }
                } else {
                    sucinfo._pcounter[i] = 0; // reset counter!
                    sucinfo._tcounter[i] = std::numeric_limits<uint32_t>::max();
                }
            }
        }
        return has_succ;
    }

}

