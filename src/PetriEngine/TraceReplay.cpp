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

#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/TraceReplay.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/SuccessorGenerator.h"
#include "LTL/LTLToBuchi.h"

#include <rapidxml.hpp>
#include <vector>
#include <iostream>
#include <cstring>


namespace PetriEngine {

    const shared_const_string TraceReplay::DEADLOCK_TRANS{std::make_shared<const_string>("##deadlock")};

    TraceReplay::TraceReplay(std::istream &is, const PetriEngine::PetriNet *net, const options_t &options)
        : options(options) {
        parse(is, net);
    }

    void TraceReplay::parse(std::istream &xml, const PetriNet *net) {
        places.clear();
        transitions.clear();
        // preallocate reverse lookup for transition and place names. Assume this is always called with the same Petri net.
        for (size_t i = 0; i < net->placeNames().size(); ++i) {
            places[net->placeNames()[i]] = i;
        }
        for (size_t i = 0; i < net->transitionNames().size(); ++i) {
            transitions[net->transitionNames()[i]] = i;
        }
        transitions[DEADLOCK_TRANS] = -1;

        // TODO can also validate transition names up front.
        rapidxml::xml_document<> doc;
        std::vector<char> buffer((std::istreambuf_iterator<char>(xml)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        doc.parse<0>(buffer.data());
        rapidxml::xml_node<> *root = doc.first_node();

        if (root) {
            parseRoot(root);
        } else {
            assert(false);
            throw base_error("Could not get root node.");
        }

    }

    void TraceReplay::parseRoot(const rapidxml::xml_node<> *pNode) {
        if (std::strcmp(pNode->name(), "trace") != 0) {
            assert(false);
            throw base_error("Expected trace node. Got: ", pNode->name());
        }
        for (auto it = pNode->first_node(); it; it = it->next_sibling()) {
            if (std::strcmp(it->name(), "loop") == 0) loop_idx = trace.size();
            else if (std::strcmp(it->name(), "transition") == 0 || std::strcmp(it->name(), "deadlock") == 0) {
                trace.push_back(parseTransition(it));
            } else {
                assert(false);
                throw base_error("Expected transition or loop node. Got: ", it->name());
            }
        }
        if (loop_idx == std::numeric_limits<size_t>::max() && options.logic == TemporalLogic::LTL) {
            throw base_error("Missing <loop/> statement in trace");
        }
    }

    TraceReplay::Transition TraceReplay::parseTransition(const rapidxml::xml_node<char> *pNode) {
        shared_const_string id;
        int buchi = -1;
        for (auto it = pNode->first_attribute(); it; it = it->next_attribute()) {
            if (std::strcmp(it->name(), "id") == 0) {
                id = std::make_shared<const_string>(it->value());
            }
            if (std::strstr(it->name(), "buchi") != nullptr) {
                // buchi field is sometimes spelled slightly differently, but will always contain 'buchi'
                buchi = std::stoi(it->value());
            }
        }
        if (strcmp(pNode->name(), "deadlock") == 0) {
            id = DEADLOCK_TRANS;
        }
        if (id->empty()) {
            assert(false);
            throw base_error("Transition has no id attribute");
        }

        Transition transition(id, buchi);

        for (auto it = pNode->first_node(); it; it = it->next_sibling()) {
            if (std::strcmp(it->name(), "token") != 0) {
                std::cerr << "Warning: Unexpected transition child. Expected: token, but got: " << it->name()
                          << std::endl;
            }
            parseToken(it, transition.tokens);
        }

        return transition;
    }

    void
    TraceReplay::parseToken(const rapidxml::xml_node<char> *pNode,
                            std::unordered_map<uint32_t, uint32_t> &current_marking) {
        for (auto it = pNode->first_attribute(); it; it = it->next_attribute()) {
            if (std::strcmp(it->name(), "place") == 0) {
                auto val = std::make_shared<const_string>(it->value());
                if (current_marking.find(places.at(val)) == current_marking.end()) {
                    current_marking[places.at(val)] = 1;
                } else {
                    ++current_marking[places.at(val)];
                }
            }
        }
    }

    bool TraceReplay::replay(const PetriEngine::PetriNet *net, const PetriEngine::PQL::Condition_ptr &cond) {
        //spot::print_dot(std::cerr, buchiGenerator.aut._buchi);
        PetriEngine::Structures::State state;
        state.setMarking(net->makeInitialMarking());
        PetriEngine::SuccessorGenerator successorGenerator(*net);

        std::cout << "Playing back trace. Length: " << trace.size() << std::endl;
        if (_play_trace(net, successorGenerator)) {
            std::cout << "Replay complete. No errors" << std::endl;
            return true;
        }
        return false;
    }

    bool
    TraceReplay::_play_trace(const PetriEngine::PetriNet *net, PetriEngine::SuccessorGenerator &successorGenerator) {
        PetriEngine::Structures::State state;
        PetriEngine::Structures::State loopstate;
        bool looping = false;
        state.setMarking(net->makeInitialMarking());
        loopstate.setMarking(net->makeInitialMarking());
        for (size_t i = 0; i < trace.size(); ++i) {
            const Transition &transition = trace[i];
            // looping part should end up at the state _before_ the <loop/> tag,
            // hence copy state from previous iteration.
            if (i == loop_idx) {
                memcpy(loopstate.marking(), state.marking(), sizeof(uint32_t) * net->numberOfPlaces());
                looping = true;
            }
            successorGenerator.prepare(&state);
            auto it = transitions.find(transition.id);
            if (it == std::end(transitions)) {
                assert(false);
                throw base_error("Unrecognized transition name ", transition.id);
            }
            int tid = it->second;

            if (tid != -1) {
                // fire transition
                if (!successorGenerator.checkPreset(tid)) {
                    std::cerr
                            << "ERROR the provided trace cannot be replayed on the provided model. \nOffending transition: "
                            << transition.id << " at index: " << i << "\n";
                    return false;
                }
                successorGenerator.consumePreset(state, tid);
                successorGenerator.producePostset(state, tid);
            } else {
                // -1 is deadlock, assert deadlocked state.
                // LTL deadlocks are unambiguously in the Petri net, since Büchi deadlocks won't generate any successor in the first place.
                assert(i == trace.size() - 1);
                if (!net->deadlocked(state.marking())) {
                    std::cerr << "ERROR: Trace claimed the net was deadlocked, but there are enabled transitions.\n";
                    return false;
                }
                return true;
            }

            if (!transition.tokens.empty()) {
                auto[finv, linv] = net->preset(transitions.at(transition.id));
                for (; finv != linv; ++finv) {
                     if (finv->inhibitor) {

                    } else {
                        auto it = transition.tokens.find(finv->place);
                        if (it == std::end(transition.tokens)) {
                            std::cerr << "ERROR: Pre-place " << net->placeNames()[finv->place] << " of transition "
                                      << transition.id << " was not mentioned in trace.\n";
                            return false;
                        }
                        if (it->second != finv->tokens) {
                            std::cerr << "ERROR: Pre-place " << net->placeNames()[finv->place] << " of transition "
                                      << transition.id << "has arc weight " << finv->tokens << " but trace consumes "
                                      << it->second << " tokens.\n";
                            return false;
                        }
                    }
                }
            }
        }

        bool err = false;
        if (looping) {
            for (size_t i = 0; i < net->numberOfPlaces(); ++i) {
                if (state.marking()[i] != loopstate.marking()[i]) {
                    if (!err) {
                        std::cerr << "ERROR end state not equal to expected loop state.\n";
                        err = true;
                    }
                    std::cerr << "  " << net->placeNames()[i] << ": expected" << loopstate.marking()[i] << ", actual "
                              << state.marking()[i] << '\n';
                }
            }
        }

        return !err;
    }
}

