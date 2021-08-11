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
#include "LTL/SuccessorGeneration/Spoolers.h"


namespace LTL {
    template<typename S, typename Spooler>
    class ReachStubProductSuccessorGenerator : public ProductSuccessorGenerator<S> {
    public:
        ReachStubProductSuccessorGenerator(const PetriEngine::PetriNet *net, const Structures::BuchiAutomaton &buchi,
                                           S *successorGen, std::unique_ptr<Spooler> &&fallbackSpooler)
                : ProductSuccessorGenerator<S>(net, buchi, successorGen), _fallback_spooler(std::move(fallbackSpooler))
        {
            //_fallback_spooler = std::make_unique<Spooler>(fallbackSpooler);
/*            if constexpr (std::is_same_v<Spooler, EnabledSpooler>) {
                _fallback_spooler = std::make_unique<Spooler>(net, static_cast<PetriEngine::SuccessorGenerator&>(this->_successor_generator));
            }
            else {
                _fallback_spooler = std::make_unique<Spooler>(net);
            }*/
            // Create the set of büchi states from which we can use reachability stubborn sets.
            calc_safe_reach_states(buchi);
            _reach = std::make_unique<SafeAutStubbornSet>(*net, _progressing_formulae);

        }

        void calc_safe_reach_states(const Structures::BuchiAutomaton &buchi) {
            assert(_reach_states.empty());
            std::vector<AtomicProposition> aps(buchi.ap_info.size());
            std::transform(std::begin(buchi.ap_info), std::end(buchi.ap_info), std::begin(aps),
                           [](const std::pair<int, AtomicProposition> &pair) { return pair.second; });
            for (unsigned state = 0; state < buchi._buchi->num_states(); ++state) {
                if (buchi._buchi->state_is_accepting(state)) continue;

                bdd retarding = bddfalse;
                bdd progressing = bddfalse;
                for (auto &e : buchi._buchi->out(state)) {
                    if (e.dst == state) {
                        retarding = e.cond;
                    }
                    else {
                        progressing |= e.cond;
                    }
                }
                bdd sink_prop = bdd_not(retarding | progressing);
                auto prog_cond = toPQL(spot::bdd_to_formula(progressing, buchi.dict), aps);
                auto ret_cond = toPQL(spot::bdd_to_formula(retarding, buchi.dict), aps);
                auto sink_cond = sink_prop == bdd_false()
                                 ? PetriEngine::PQL::BooleanCondition::FALSE_CONSTANT
                                 : std::make_shared<PetriEngine::PQL::NotCondition>(
                                std::make_shared<PetriEngine::PQL::OrCondition>(prog_cond, ret_cond)
                        );
                _reach_states.insert(std::make_pair(
                        state,
                        BuchiEdge{progressing | sink_prop,
                                  prog_cond,
                                  sink_cond}));
            }
        }

        void prepare(const LTL::Structures::ProductState *state, typename S::successor_info_t &sucinfo) override
        {
            auto suc = _reach_states.find(state->getBuchiState());
            if (suc != std::end(_reach_states) && !this->guard_valid(*state, suc->second.bddCond)) {
                //_reach->setQuery(suc->second.prog_cond.get());
                _reach->set_buchi_edge(suc->second.prog_cond, suc->second.pseudo_sink_cond);
                set_spooler(_reach.get());
            }
            else {
                set_spooler(_fallback_spooler.get());
            }
            ProductSuccessorGenerator<S>::prepare(state, sucinfo);
        }

    private:
        void set_spooler(SuccessorSpooler *spooler)
        {
            if constexpr (std::is_same_v<S, LTL::SpoolingSuccessorGenerator>)
                this->_successor_generator->setSpooler(spooler);
            else {
                assert(false);
                std::cerr << "Fatal error\n"; exit(1);
            }
        }

        struct BuchiEdge{
            bdd bddCond;
            PetriEngine::PQL::Condition_ptr prog_cond;
            PetriEngine::PQL::Condition_ptr pseudo_sink_cond;
        };

        std::unique_ptr<Spooler> _fallback_spooler;
        std::unique_ptr<LTL::SafeAutStubbornSet> _reach;
        std::unordered_map<size_t, BuchiEdge> _reach_states;
        std::vector<PetriEngine::PQL::Condition_ptr> _progressing_formulae;

    };
}

#endif //VERIFYPN_REACHSTUBPRODUCTSUCCESSORGENERATOR_H
