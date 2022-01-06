/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (C) 2019 Peter G. Jensen <root@petergjoel.dk>
 */

/*
 * File:   SynthConfig.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 9, 2019, 9:53 AM
 */

#ifndef SYNTHCONFIG_H
#define SYNTHCONFIG_H

#include <forward_list>
#include <cinttypes>
#include <cstddef>

namespace PetriEngine {
    namespace Synthesis {

        struct SynthConfig {
            typedef std::pair<bool, SynthConfig*> depender_t;
            typedef std::forward_list<depender_t> depends_t;

            // using uint8_t here instead of enums, packs data better
            static constexpr uint8_t UNKNOWN = 1; // no successors generated yet
            static constexpr uint8_t PROCESSED = 2; // Generated successors
            static constexpr uint8_t MAYBE = 4; // If no env strategy, then winning
            static constexpr uint8_t LOSING = 8; // env wins
            static constexpr uint8_t WINNING = 32; // ctrl surely wins
            static constexpr uint8_t PRINTED = 64; // has been printed

            uint8_t _state = UNKNOWN; // this should be at most one byte
            uint8_t _waiting = 0; // We only need on waiting once (0 = new, 1= processed/waiting, 2=back-queue)

            uint32_t _ctrl_children = 0; // It is sufficient to keep track of the counts here
            uint32_t _env_children = 0;
            // in any resonable net, these would be less than 2^32

            depends_t _dependers; // A bunch of parents
            // TODO: check if the dependers are faster by
            // 1) vector
            // 2) list
            // 3) storing as a bunch of pairs in a ptrie (def. more compact)

            bool determined() const {
                return (_state & (WINNING | LOSING)) != 0;
            }
            size_t _marking;
        };
    }
}
#endif /* SYNTHCONFIG_H */

