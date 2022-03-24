/* Copyright (C) 2022  Peter G. Jensen <root@petergjoel.dk>,
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

/*
 * File:   CompoundGenerator.h
 * Author: Peter G. Jensen
 * Based on ResumingSeuccessorGenerator (authors
 *
 * Created on 21 March 2022, 16.54
 */

#ifndef COMPOUNDGENERATOR_H
#define COMPOUNDGENERATOR_H

#include "PetriEngine/SuccessorGenerator.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "utils/errors.h"

#include <memory>

namespace LTL {

    class CompoundGenerator {
    public:

        struct successor_info_t {
            size_t _last_state;
            
            bool has_prev_state() const {
                return _last_state != NoLastState;
            }

            std::vector<uint32_t> transition() const {
                std::vector<uint32_t> compound(_enabled.size(), std::numeric_limits<uint32_t>::max());
                for(size_t id = 0; id < _enabled.size(); ++id)
                {
                    if(!_enabled[id].empty())
                        compound[id] = _enabled[id][_enabled_it[id]];
                }
                return compound;
            }

            [[nodiscard]] bool fresh() const {
                return _enabled_it.empty();
            }
            size_t _buchi_state;

        private:
            // wasting of memory, but good enough for now
            std::vector<std::vector<uint32_t>> _enabled;
            std::vector<uint32_t> _enabled_it;
            successor_info_t() {
                _buchi_state = NoBuchiState;
                _last_state = NoLastState;
            }
            static constexpr auto NoPCounter = 0;
            static constexpr auto NoTCounter = std::numeric_limits<uint32_t>::max();
            static constexpr auto NoBuchiState = std::numeric_limits<size_t>::max();
            static constexpr auto NoLastState = std::numeric_limits<size_t>::max();
            friend class CompoundGenerator;
        };
    public:

        CompoundGenerator(const PetriEngine::PetriNet& net, size_t hyper_traces);

        CompoundGenerator(const PetriEngine::PetriNet& net, size_t hyper_traces,
                const std::shared_ptr<PetriEngine::PQL::Condition> &query);

        ~CompoundGenerator() = default;

        size_t state_size() const {
            return _generator.net().numberOfPlaces() * _hyper_traces;
        }

        void initialize(PetriEngine::MarkVal* marking) const {
            for(size_t i = 0; i < _hyper_traces; ++i)
            {
                std::copy(  _generator.net().initial(),
                            _generator.net().initial() + _generator.net().numberOfPlaces(),
                            marking + _generator.net().numberOfPlaces()*i);
            }
        }

        void prepare(const PetriEngine::Structures::State *state, const successor_info_t &sucinfo);

        bool next(PetriEngine::Structures::State &write, successor_info_t &sucinfo);

        auto* getParent() const {
            return _parent->marking();
        }

        uint32_t fired() const {
            throw base_error("Traces not yet supported by Hyper-LTL");
        }

        auto initial_suc_info() {
            return successor_info_t();
        }

    private:
        const size_t _hyper_traces;
        PetriEngine::SuccessorGenerator _generator;
        const PetriEngine::Structures::State* _parent;

    };
}

#endif /* COMPOUNDGENERATOR_H */

