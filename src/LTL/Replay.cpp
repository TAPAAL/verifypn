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
#include "LTL/Replay.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/SuccessorGenerator.h"
#include "LTL/LTLToBuchi.h"

#include <rapidxml.hpp>
#include <vector>
#include <iostream>
#include <cstring>

using namespace PetriEngine;
using namespace PetriEngine::PQL;

namespace LTL {
    bool guard_valid(const PQL::EvaluationContext &ctx, const BuchiSuccessorGenerator &buchi, bdd bdd);

    Replay::Replay(std::istream &is, const PetriEngine::PetriNet *net)
    {
        parse(is, net);
    }

    void Replay::parse(std::istream &xml, const PetriNet *net)
    {
        places.clear();
        transitions.clear();
        // preallocate reverse lookup for transition and place names. Assume this is always called with the same Petri net.
        for (int i = 0; i < net->placeNames().size(); ++i) {
            places[net->placeNames()[i]] = i;
        }
        for (int i = 0; i < net->transitionNames().size(); ++i) {
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
            std::cerr << "Error getting root node." << std::endl;
            assert(false);
            exit(1);
        }

    }

    void Replay::parseRoot(const rapidxml::xml_node<> *pNode)
    {
        if (std::strcmp(pNode->name(), "trace") != 0) {
            std::cerr << "Error: Expected trace node. Got: " << pNode->name() << std::endl;
            assert(false);
            exit(1);
        }
        for (auto it = pNode->first_node(); it; it = it->next_sibling()) {
            if (std::strcmp(it->name(), "loop") == 0) loop_idx = trace.size();
            else if (std::strcmp(it->name(), "transition") == 0 || std::strcmp(it->name(), "deadlock") == 0) {
                trace.push_back(parseTransition(it));
            } else {
                std::cerr << "Error: Expected transition or loop node. Got: " << it->name() << std::endl;
                assert(false);
                exit(1);
            }
        }
        if (loop_idx == std::numeric_limits<size_t>::max()) {
            std::cerr << "Error: Missing <loop/> statement in trace\n";
            exit(1);
        }
    }

    Replay::Transition Replay::parseTransition(const rapidxml::xml_node<char> *pNode)
    {
        std::string id;
        int buchi = -1;
        for (auto it = pNode->first_attribute(); it; it = it->next_attribute()) {
            if (std::strcmp(it->name(), "id") == 0) {
                id = std::string(it->value());
            }
            if (std::strstr(it->name(), "buchi") != nullptr) {
                // buchi field is sometimes spelled slightly differently, but will always contain 'buchi'
                buchi = std::stoi(it->value());
            }
        }
        if (strcmp(pNode->name(), "deadlock") == 0) {
            id = DEADLOCK_TRANS;
        }
        if (id.empty()) {
            std::cerr << "Error: Transition has no id attribute" << std::endl;
            assert(false);
            exit(1);
        }
        if (buchi == -1) {
            std::cerr << "Error: Missing Büchi state" << std::endl;
            assert(false);
            exit(1);
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
    Replay::parseToken(const rapidxml::xml_node<char> *pNode, std::unordered_map<uint32_t, uint32_t> &current_marking)
    {
        for (auto it = pNode->first_attribute(); it; it = it->next_attribute()) {
            if (std::strcmp(it->name(), "place") == 0) {
                std::string val{it->value()};
                if (current_marking.find(places.at(val)) == current_marking.end()) {
                    current_marking[places.at(val)] = 1;
                } else {
                    ++current_marking[places.at(val)];
                }
            }
        }
    }

    bool Replay::replay(const PetriEngine::PetriNet *net, const PetriEngine::PQL::Condition_ptr &cond)
    {
        // TODO respect command line switch
        BuchiSuccessorGenerator buchiGenerator = makeBuchiSuccessorGenerator(cond, false);
        //spot::print_dot(std::cerr, buchiGenerator.aut._buchi);
        PetriEngine::Structures::State state;
        state.setMarking(net->makeInitialMarking());
        PetriEngine::SuccessorGenerator successorGenerator(*net);

        // generate all initial Buchi states since trace may be dependent on only some initial state.
        size_t prev_buchi = buchiGenerator.initial_state_number();
        buchiGenerator.prepare(prev_buchi);
        bdd bdd;
        std::vector<size_t> init_buchi_states;
        while (buchiGenerator.next(prev_buchi, bdd)) {
            if (guard_valid(EvaluationContext{state.marking(), net}, buchiGenerator, bdd)) {
                init_buchi_states.push_back(prev_buchi);
            }
        }
        std::cout << "Playing back trace. Length: " << trace.size() << std::endl;
        for (int i = 0; i < init_buchi_states.size(); ++i) {
            std::cerr << "Replaying initial state " << i << "/" << init_buchi_states.size() << std::endl;
            if (_play_trace(net, successorGenerator, buchiGenerator, init_buchi_states[i])) {
                std::cerr << "Replay complete. No errors" << std::endl;
                return true;
            }
        }
        std::cerr << "ERROR The trace could not be played back using any valid initial Büchi state. "
                  << "See previous output for details."
                  << std::endl;
        return false;
    }

    bool Replay::_play_trace(const PetriEngine::PetriNet *net, PetriEngine::SuccessorGenerator &successorGenerator,
                             BuchiSuccessorGenerator &buchiGenerator, size_t init_buchi)
    {
        PetriEngine::Structures::State state;
        PetriEngine::Structures::State loopstate;
        size_t loop_buchi;
        bool loop_accepting = false;
        bool looping = false;
        state.setMarking(net->makeInitialMarking());
        loopstate.setMarking(net->makeInitialMarking());
        size_t prev_buchi = init_buchi;
        bdd bdd;
        bool buchi_failed = false;
        for (int i = 0; i < trace.size(); ++i) {
            const Transition &transition = trace[i];
            // looping part should end up at the state _before_ the <loop/> tag,
            // hence copy state from previous iteration.
            if (i == loop_idx) {
                memcpy(loopstate.marking(), state.marking(), sizeof(uint32_t) * net->numberOfPlaces());
                loop_buchi = prev_buchi;
                looping = true;
            }
            successorGenerator.prepare(&state);
            buchiGenerator.prepare(prev_buchi);
            auto it = transitions.find(transition.id);
            if (it == std::end(transitions)) {
                std::cerr << "Unrecognized transition name " << transition.id << std::endl;
                assert(false);
                exit(1);
            }
            int tid = it->second;

            if (tid != -1) {
                // fire transition
                if (!successorGenerator.checkPreset(tid)) {
                    std::cerr
                            << "ERROR the provided trace cannot be replayed on the provided model. Offending transition: "
                            << transition.id << " at index: " << i << "\n";
                    exit(1);
                }
                successorGenerator.consumePreset(state, tid);
                successorGenerator.producePostset(state, tid);
            }
            else {
                // NOTE Deadlock check, not yet implemented. This assumes no transition in the net is named ##deadlock.
            }

            if (!transition.tokens.empty()) {
                for (int pid = 0; pid < net->numberOfPlaces(); ++pid) {
                    if (transition.tokens.find(pid) == std::end(transition.tokens)) {
                        // If trace didn't mention the place, then it must have 0 tokens.
                        if (state.marking()[pid] > 0) {
                            std::cerr << "ERROR the playback of the trace resulted in mismatch of token counts. \n"
                                         "  Offending place " << net->placeNames()[pid] << ", expected " << 0
                                      << " tokens but got " << state.marking()[pid] << std::endl;
                            std::cerr << "  Offending transition: " << transition.id << " at index: " << i << "\n";
                            exit(1);
                        }
                    } else if (transition.tokens.at(pid) != state.marking()[pid]) {
                        // If trace did mention the place, then validate token count.
                        std::cerr << "ERROR the playback of the trace resulted in mismatch of token counts. \n"
                                     "  Offending place " << net->placeNames()[pid] << ", expected "
                                  << transition.tokens.at(pid)
                                  << " tokens but got " << state.marking()[pid] << std::endl;
                        std::cerr << "Offending transition: " << transition.id << " at index: " << i << "\n";
                        exit(1);
                    }
                }
            }
            size_t next_buchi = -1;
            const EvaluationContext &ctx = EvaluationContext{state.marking(), net};
            while (buchiGenerator.next(next_buchi, bdd)) {
                if (next_buchi == transition.buchi_state) break;
            }
            if (!guard_valid(ctx, buchiGenerator, bdd)) {
                // warning since technically we may have multiple initial buchi states,
                // and maybe only some will allow replaying the trace.
                std::cerr << "WARNING guard invalid for noted buchi state " << transition.buchi_state
                          << " at index " << i << ". Attempting to proceed anyway.\n";
                buchi_failed = true;
            }
            prev_buchi = next_buchi;
            if (looping && buchiGenerator.is_accepting(prev_buchi)) {
                loop_accepting = true;
            }

            if (i % 100000 == 0)
                std::cerr << i << "/" << trace.size() << std::endl;
        }

        bool err = false;
        for (int i = 0; i < net->numberOfPlaces(); ++i) {
            if (state.marking()[i] != loopstate.marking()[i]) {
                if (!err) {
                    std::cerr << "ERROR end state not equal to expected loop state.\n";
                    err = true;
                }
                std::cerr << "  " << net->placeNames()[i] << ": expected" << loopstate.marking()[i] << ", actual "
                          << state.marking()[i] << '\n';
            }
        }
        if (prev_buchi != loop_buchi) {
            if (!err) {
                std::cerr << "ERROR end state not equal to expected loop state.\n";
                err = true;
            }
            std::cerr << "Buchi state: expected " << loop_buchi << ", actual " << prev_buchi << std::endl;
        }
        if (err) exit(1);

        if (!loop_accepting) {
            std::cerr << "WARNING trace did not demonstrate accepting loop using initial Buchi state " << init_buchi
                      << ".\n";
            return false;
        }

        return !buchi_failed;
    }

    bool guard_valid(const PQL::EvaluationContext &ctx, const BuchiSuccessorGenerator &buchi, bdd bdd)
    {
        using namespace PetriEngine::PQL;
        // IDs 0 and 1 are false and true atoms, respectively
        // More details in buddy manual for details ( http://buddy.sourceforge.net/manual/main.html )
        while (bdd.id() > 1) {
            // find variable to test, and test it
            size_t var = bdd_var(bdd);
            Condition::Result res = buchi.getExpression(var)->evaluate(ctx);
            switch (res) {
                case Condition::RUNKNOWN:
                    std::cerr << "Unexpected unknown answer from evaluating query!\n";
                    assert(false);
                    exit(1);
                    break;
                case Condition::RFALSE:
                    bdd = bdd_low(bdd);
                    break;
                case Condition::RTRUE:
                    bdd = bdd_high(bdd);
                    break;
            }
        }
        return bdd == bddtrue;
    }

}
