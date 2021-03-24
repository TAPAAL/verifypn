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

#include <rapidxml.hpp>
#include <vector>
#include <iostream>
#include <cstring>
#include <PetriEngine/Structures/State.h>
#include <PetriEngine/SuccessorGenerator.h>

#include "PetriEngine/PetriNet.h"
#include "LTL/Replay.h"
#include "../../cmake-build-debug/external/include/rapidxml.hpp"
namespace LTL {
    void Replay::parse(std::ifstream &xml) {
        rapidxml::xml_document<> doc;
        std::vector<char> buffer((std::istreambuf_iterator<char>(xml)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        doc.parse<0>(buffer.data());
        rapidxml::xml_node<> *root = doc.first_node();

        if (root) {
            parseRoot(root);
        } else {
            std::cerr << "Error getting root node." << std::endl;
            assert(false);
            exit(1);
        }
    }

    void Replay::parseRoot(const rapidxml::xml_node<> *pNode) {
        if (std::strcmp(pNode->name(), "trace") != 0) {
            std::cerr << "Error: Expected trace node. Got: " << pNode->name() << std::endl;
            assert(false); exit(1);
        }
        for (auto it = pNode->first_node(); it; it = it->next_sibling()) {
            if (std::strcmp(it->name(), "loop") == 0) continue;
            if (std::strcmp(it->name(), "deadlock") == 0) continue;
            if (std::strcmp(it->name(), "transition") != 0){
                std::cerr << "Error: Expected transition or loop node. Got: " << it->name() << std::endl;
                assert(false); exit(1);
            }
            trace.push_back(parseTransition(it));
        }
    }

    Replay::Transition Replay::parseTransition(const rapidxml::xml_node<char> *pNode) {
        std::string id;
        for (auto it = pNode->first_attribute(); it; it = it->next_attribute()) {
            if (std::strcmp(it->name(), "id") == 0) {
                id = std::string(it->value());
            }
        }
        if (id.empty()) {
            std::cerr << "Error: Transition has no id attribute" << std::endl;
            assert(false); exit(1);
        }

        Transition transition(id);

        for (auto it = pNode->first_node(); it; it = it->next_sibling()) {
            assert(std::strcmp(it->name(), "token"));
            transition.tokens.push_back(parseToken(it));
        }

        return transition;
    }

    Replay::Token Replay::parseToken(const rapidxml::xml_node<char> *pNode) {
        std::string place;
        for (auto it = pNode->first_attribute(); it; it=it->next_attribute()) {
            if (std::strcmp(it->name(), "place") == 0) {
                place = std::string(it->value());
            }
        }
        assert(!place.empty());
        return Token{place};
    }

    bool Replay::replay(const PetriEngine::PetriNet *net) {
        PetriEngine::Structures::State state;
        std::unordered_map<std::string, int> transitions;
        // speed up by an order of magnitude compared to linear look-up during replay
        for (int i = 0; i < net->transitionNames().size(); ++i) {
            transitions[net->transitionNames()[i]] = i;
        }
        state.setMarking(net->makeInitialMarking());
        PetriEngine::SuccessorGenerator successorGenerator(*net);
        std::cout << "Playing back trace. Length: " << trace.size() << std::endl;
        //for (const Transition& transition : trace) {
        int size = trace.size();
        for (int i = 0; i < size; ++i) {
            const Transition &transition = trace[i];
            successorGenerator.prepare(&state);
            auto it = transitions.find(transition.id); // std::find(std::begin(transitions), std::end(transitions), transition.id);
            if (it == std::end(transitions)) {
                std::cerr << "Unrecognized transition name " << transition.id << std::endl;
                assert(false); exit(1);
            }
            int tid = it->second;

            if (!successorGenerator.checkPreset(tid)) {
                std::cerr << "ERROR the provided trace cannot be replayed on the provided model. Offending transition: " << transition.id << " at index: " << i << "\n";
                exit(1);
            }
            successorGenerator.consumePreset(state, tid);
            successorGenerator.producePostset(state, tid);
            //std::cerr << "Fired transition: " << transition.id << std::endl;
            if (i % 100000 == 0)
                std::cerr << i << "/" << size << std::endl;
        }
        std::cerr << "Replay complete. No errors" << std::endl;
        return true;
    }
}
