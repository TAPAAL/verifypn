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

#ifndef VERIFYPN_REACHSTUBPRODUCTSUCCESSORGENERATOR_H
#define VERIFYPN_REACHSTUBPRODUCTSUCCESSORGENERATOR_H

#include "LTL/SuccessorGeneration/ProductSuccessorGenerator.h"
#include "LTL/Stubborn/ReachabilityStubbornSpooler.h"
#include "LTL/SuccessorGeneration/EnabledSpooler.h"

//#define REACH_STUB_DEBUG

namespace LTL {
    template<typename S>
    class ReachStubProductSuccessorGenerator : public ProductSuccessorGenerator<S> {
    public:
        ReachStubProductSuccessorGenerator(const PetriEngine::PetriNet *net, const Structures::BuchiAutomaton &buchi,
                                           S &&successorGen)
                : ProductSuccessorGenerator<S>(net, buchi, std::move(successorGen))

        {
            _enabled = std::make_unique<EnabledSpooler>(net, static_cast<PetriEngine::SuccessorGenerator&>(this->_successor_generator));
            _reach = std::make_unique<ReachabilityStubbornSpooler>(*net);
            // Create the set of büchi states from which we can use reachability stubborn sets.
            std::vector<AtomicProposition> aps(buchi.ap_info.size());
            std::transform(std::begin(buchi.ap_info), std::end(buchi.ap_info), std::begin(aps),
                           [](const std::pair<int, AtomicProposition> &pair) { return pair.second; });
            for (unsigned state = 0; state < buchi._buchi->num_states(); ++state) {
                if (buchi._buchi->state_is_accepting(state)) continue;
                bdd retarding = bddfalse;
                bdd progressing = bddfalse;
                for (auto &e : buchi._buchi->out(state)) {
                    auto formula = spot::bdd_to_formula(e.cond, buchi.dict);
                    if (e.dst == state) {
                        retarding = e.cond;
                    } else {
                        // Remove the first disjunct  to enable disjunction of multiple progressing formulae.
                        /*if (progressing != bddfalse || !buchi._buchi->state_is_accepting(e.dst)) {
                            progressing = bddfalse;
                            break;
                        }*/
                        progressing |= e.cond;
                    }
                }
                if (progressing == bddfalse || (progressing | retarding) != bddtrue) continue;

                _reach_states.insert(std::make_pair(state, BuchiEdge{progressing, toPQL(spot::bdd_to_formula(progressing, buchi.dict), aps)}));
            }

            for (auto it = std::begin(_reach_states); it != std::end(_reach_states);) {
                auto &state = it->first;
                bool has_erased = false;
                for (auto &e: buchi._buchi->out(state)) {
                    if (e.dst != state) {
                        auto suc = e.dst;
                        // test self-loop of successor for universal satisfaction
                        for (auto &suc_edge: buchi._buchi->out(suc)) {
                            if (suc_edge.dst == suc && suc_edge.cond != bddtrue) {
                                it = _reach_states.erase(it);
                                has_erased = true;
                                break;
                            }
                        }
                    }
                    if (has_erased) break;
/*                    if (e.dst == state && e.cond != bddtrue) {
                        it = _reach_states.erase(it);
                        has_erased = true;
                        break;
                    }*/
                }
                if (!has_erased) ++it;

            }
            std::cout << "Size of _reach_states: " << _reach_states.size() << std::endl;
#ifdef REACH_STUB_DEBUG
            if (_reach_states.empty()) {
                //exit(0);
            } else {
                std::cerr << "Size of _reach_states: " << _reach_states.size() << std::endl;
            }
#endif
        }

/*
        bool prepare(const LTL::Structures::ProductState *state) override {
            //FIXME For some reason RandomNDFS is not able to call this directly on the super class.
            ProductSuccessorGenerator<S>::prepare(state);
        }
*/

        void prepare(const LTL::Structures::ProductState *state, typename S::sucinfo &sucinfo) override
        {
            if (auto suc = _reach_states.find(state->getBuchiState()); suc != std::end(_reach_states) && !this->guard_valid(*state, suc->second.bddCond)) {
#ifdef REACH_STUB_DEBUG
                if (!_reach_active) {
                    std::cout << "Found reach stub state. Switching spooler." << std::endl;
                    _reach_active = true;
                }
#endif
                _reach->set_query(suc->second.cond);
                set_spooler(_reach.get());
            }
            else {
#ifdef REACH_STUB_DEBUG
                if (_reach_active) {
                    std::cout << "Leaving reach stub state. Switching to enabled spooler." << std::endl;
                    _reach_active = false;
                }
#endif
                set_spooler(_enabled.get());
            }
            ProductSuccessorGenerator<S>::prepare(state, sucinfo);
        }

    private:
        void set_spooler(SuccessorSpooler *spooler)
        {
            if constexpr (std::is_same_v<S, LTL::SpoolingSuccessorGenerator>)
                this->_successor_generator.setSpooler(spooler);
            else {
                assert(false);
                std::cerr << "Fatal error\n"; exit(1);
            }
        }

        struct BuchiEdge{
            bdd bddCond;
            PetriEngine::PQL::Condition_ptr cond;
        };

        std::unique_ptr<EnabledSpooler> _enabled;
        std::unique_ptr<ReachabilityStubbornSpooler> _reach;
        std::unordered_map<size_t, BuchiEdge> _reach_states;
#ifdef REACH_STUB_DEBUG
        bool _reach_active = false;
#endif
    };
}

#endif //VERIFYPN_REACHSTUBPRODUCTSUCCESSORGENERATOR_H
