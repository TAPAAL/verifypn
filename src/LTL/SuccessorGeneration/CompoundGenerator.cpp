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
    : _generator(net), _hyper_traces(hyper_traces == 0 ? 1 : hyper_traces) {
    }

    void CompoundGenerator::prepare(const Structures::State* state, const successor_info_t &sucinfo) {
        _parent = state;
    }

    bool CompoundGenerator::next(PetriEngine::Structures::State &write, successor_info_t &sucinfo) {
        auto fire = [this](PetriEngine::Structures::State& write, successor_info_t& sucinfo) {
            for(size_t i = 0; i < _hyper_traces; ++i)
            {
                if(!sucinfo._enabled[i].empty())
                {
                    PetriEngine::Structures::State working(const_cast<PetriEngine::MarkVal*>(write.marking()) +
                                            (i * _generator.net().numberOfPlaces()));
                    auto t = sucinfo._enabled[i][sucinfo._enabled_it[i]];
                    _generator.consumePreset(working, t);
                    _generator.producePostset(working, t);
                    working.release();
                }
            }
        };
        std::copy(_parent->marking(), _parent->marking() + _generator.net().numberOfPlaces()*_hyper_traces, write.marking());
        if (sucinfo.fresh()) {
            // after this call, everything will be primed!
            bool any = false;
            for(size_t i = 0; i < _hyper_traces; ++i)
            {
                sucinfo._enabled.resize(_hyper_traces);
                sucinfo._enabled_it.resize(_hyper_traces, 0);
                PetriEngine::Structures::State working(const_cast<PetriEngine::MarkVal*>(_parent->marking()) +
                                                        (i * _generator.net().numberOfPlaces()));
                _generator.prepare(working);
                while(_generator.next(write))
                {
                    any = true;
                    sucinfo._enabled[i].emplace_back(_generator.fired());
                }
                working.release();
            }
            std::copy(_parent->marking(), _parent->marking() + _generator.net().numberOfPlaces(), write.marking());
            if(any)
                fire(write, sucinfo);
            return any;
        }
        else
        {
            bool any = false;
            for(size_t i = 0; i < _hyper_traces; ++i)
            {
                if(sucinfo._enabled_it[i] + 1 < sucinfo._enabled[i].size())
                {
                    ++sucinfo._enabled_it[i];
                    // reset backwards
                    for(size_t j = 0; j < i; ++j)
                        sucinfo._enabled_it[j] = 0;
                    any = true;
                    break;
                }
            }
            if(any)
                fire(write, sucinfo);
            return any;
        }
    }

}

