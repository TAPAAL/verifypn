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

#ifndef VERIFYPN_ENABLEDTRANSITIONSET_H
#define VERIFYPN_ENABLEDTRANSITIONSET_H

#include "PetriEngine/Structures/AlignedEncoder.h"
#include "PetriEngine/Structures/binarywrapper.h"

#include <ptrie/ptrie_stable.h>
#include <ptrie/ptrie_map.h>

namespace LTL::Structures {
    class EnabledTransitionSet {
    public:
        EnabledTransitionSet(size_t ntrans)
                : wrapper(ntrans), ntrans(ntrans) {}

        std::pair<bool, size_t> insert(const bool *trans)
        {
            wrapper.zero();
            for (size_t i = 0; i < ntrans; ++i) {
                wrapper.set(i, trans[i]);
            }
            auto tit = trie.insert(wrapper.raw(), wrapper.size());
            if (tit.first) {
                return std::make_pair(false, tit.second);
            } else {
                return std::make_pair(true, tit.second);
            }
        }

        void get(bool *trans, size_t id)
        {
            trie.unpack(id, wrapper.raw());
            for (size_t i = 0; i < ntrans; ++i) {
                trans[i] = wrapper.at(i);
            }
        }

    private:
        using ptrie_t = ptrie::set_stable<ptrie::uchar, 17, 128, 4>;

        binarywrapper_t wrapper;
        ptrie_t trie;
        size_t ntrans;
    };
}
#endif //VERIFYPN_ENABLEDTRANSITIONSET_H
