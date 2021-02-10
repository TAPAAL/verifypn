/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#include "LTL/Algorithm/ProductPrinter.h"

namespace LTL::ProductPrinter {
    using trans = std::pair<size_t, size_t>;
    using State = LTL::Structures::ProductState;
    template<typename SuccessorGen>
    void printProduct(ProductSuccessorGenerator<SuccessorGen> &successorGenerator, std::ostream &os, const PetriEngine::PetriNet& net, Condition_ptr formula){
        PetriEngine::Structures::StateSet states{net,0, (int)net.numberOfPlaces() + 1};
        PetriEngine::Structures::DFSQueue todo{&states};
        std::map<size_t, bool> discovered_states;
        std::vector<trans> discovered_transitions;
        Structures::ProductStateFactory factory{net, successorGenerator.initial_buchi_state()};

        State working = factory.newState();
        State curState = factory.newState();
        PetriEngine::PQL::DistanceContext ctx{&net, working.marking()};

        {
            std::vector<State> initial_states;
            successorGenerator.makeInitialState(initial_states);
            for (auto &state : initial_states) {
                auto res = states.add(state);
                assert(res.first);
                todo.push(res.second, ctx, formula);
                discovered_states.insert(std::make_pair(res.second, successorGenerator.isAccepting(state)));
            }
        }


        while (todo.pop(curState)) {
            successorGenerator.prepare(&curState);
            while (successorGenerator.next(working)){
                auto res = states.add(working);
                discovered_transitions.emplace_back(states.add(curState).second, res.second);
                if (discovered_states.count(res.second) == 0){
                    discovered_states.insert(std::make_pair(res.second, successorGenerator.isAccepting(working)));
                    todo.push(res.second, ctx, formula);
                }
            }
        }

        os << "digraph graphname {" << std::endl;
        for (auto &state : discovered_states) {
            states.decode(working, state.first);
            os << state.first << " [label=\"";
            working.printShort(net, os);
            os << working.getBuchiState() << "\"" << (state.second ? ", shape=\"doublecircle\"" : "") << "];" << std::endl;
        }
        for (auto &transition : discovered_transitions) {
            os << transition.first << " -> " << transition.second << ";" << std::endl;
        }
        os << "}" << std::endl;
    }
}