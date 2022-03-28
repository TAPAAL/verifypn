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

#ifndef VERIFYPN_TRACEREPLAY_H
#define VERIFYPN_TRACEREPLAY_H

#include "PetriEngine/PetriNet.h"
#include "PetriEngine/options.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/SuccessorGenerator.h"
#include "PQL/Contexts.h"

#include <iostream>
#include <utility>
#include <rapidxml.hpp>

namespace PetriEngine {
    class TraceReplay {
    public:
        TraceReplay(std::istream &is, const PetriEngine::PetriNet *net, const options_t &options);

        struct Token {
            std::string place;
        };

        struct Transition {
            explicit Transition(shared_const_string id, int buchi) : id(std::move(id)), buchi_state(buchi) {}

            shared_const_string id;
            int buchi_state;
            std::unordered_map<uint32_t, uint32_t> tokens;
        };

        void parse(std::istream &xml, const PetriEngine::PetriNet *net);

        bool replay(const PetriEngine::PetriNet *net, const PetriEngine::PQL::Condition_ptr &cond);

        std::vector<Transition> trace;
    private:

        static const shared_const_string DEADLOCK_TRANS;
        void parseRoot(const rapidxml::xml_node<> *pNode);

        Transition parseTransition(const rapidxml::xml_node<char> *pNode);

        void parseToken(const rapidxml::xml_node<char> *pNode, std::unordered_map<uint32_t, uint32_t> &current_marking);

        size_t loop_idx = std::numeric_limits<size_t>::max();
        shared_name_index_map transitions;
        shared_name_index_map places;
        bool _play_trace(const PetriEngine::PetriNet *net, PetriEngine::SuccessorGenerator &successorGenerator);
        const options_t &options;
    };
}

#endif //VERIFYPN_TRACEREPLAY_H
