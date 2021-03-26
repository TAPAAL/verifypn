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

#ifndef VERIFYPN_REPLAY_H
#define VERIFYPN_REPLAY_H

#include "LTL/SuccessorGeneration/BuchiSuccessorGenerator.h"

#include <fstream>
#include <utility>
#include <rapidxml.hpp>
#include "../../cmake-build-debug/external/include/rapidxml.hpp"

namespace LTL {
    class Replay {
    public:
        struct Token {
            std::string place;
        };

        struct Transition {
            explicit Transition(std::string id, int buchi) : id(id), buchi_state(buchi) {}

            std::string id;
            int buchi_state;
            std::unordered_map<uint32_t, uint32_t> tokens;
        };

        void parse(std::ifstream &xml, const PetriEngine::PetriNet *net);

        bool replay(const PetriEngine::PetriNet *net, const PetriEngine::PQL::Condition_ptr &cond);

        std::vector<Transition> trace;
    private:
        void parseRoot(const rapidxml::xml_node<> *pNode);

        Transition parseTransition(const rapidxml::xml_node<char> *pNode);

        void parseToken(const rapidxml::xml_node<char> *pNode, std::unordered_map<uint32_t, uint32_t> &current_marking);

        std::unordered_map<std::string, int> transitions;
        std::unordered_map<std::string, int> places;
        bool _play_trace(const PetriEngine::PetriNet *net, PetriEngine::SuccessorGenerator &successorGenerator,
                         BuchiSuccessorGenerator &buchiGenerator, size_t init_buchi);
    };
}

#endif //VERIFYPN_REPLAY_H
