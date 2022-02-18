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

        using successor_info_t = typename S::successor_info_t;
        static constexpr auto initial_suc_info() { return S::initial_suc_info(); }

        ReachStubProductSuccessorGenerator(const PetriEngine::PetriNet& net, const Structures::BuchiAutomaton &buchi,
                                           S& successorGen, std::unique_ptr<Spooler> &&fallbackSpooler)
                : ProductSuccessorGenerator<S>(net, buchi, successorGen), _fallback_spooler(std::move(fallbackSpooler))
        {
            calc_safe_reach_states(buchi);
            _reach = std::make_unique<SafeAutStubbornSet>(net, _progressing_formulae);
        }

        void calc_safe_reach_states(const Structures::BuchiAutomaton &buchi) {
            assert(_reach_states.empty());
            std::vector<AtomicProposition> aps;
            aps.reserve(buchi.ap_info().size());
            for(auto& [id, ap] : buchi.ap_info())
                aps.emplace_back(ap);
            for (unsigned state = 0; state < buchi.buchi().num_states(); ++state) {
                //if (buchi._buchi->state_is_accepting(state)) continue;

                bdd retarding = bddfalse;
                bdd progressing = bddfalse;
                for (auto &e : buchi.buchi().out(state)) {
                    if (e.dst == state) {
                        retarding = e.cond;
                    }
                    else {
                        progressing |= e.cond;
                    }
                }
                bdd sink_prop = bdd_not(retarding | progressing);
                auto prog_cond = toPQL(spot::bdd_to_formula(progressing, buchi.buchi().get_dict()), aps);
                auto ret_cond = toPQL(spot::bdd_to_formula(retarding, buchi.buchi().get_dict()), aps);
                auto sink_cond = sink_prop == bdd_false()
                                 ? PetriEngine::PQL::BooleanCondition::FALSE_CONSTANT
                                 : std::make_shared<PetriEngine::PQL::NotCondition>(
                                std::make_shared<PetriEngine::PQL::OrCondition>(prog_cond, ret_cond)
                        );
                _reach_states.insert(std::make_pair(
                        state,
                        buchi_edge_t{progressing | sink_prop,
                                  ret_cond,
                                  prog_cond,
                                  sink_cond}));
            }
        }

        void prepare(const LTL::Structures::ProductState *state, typename S::successor_info_t &sucinfo) override
        {
            auto suc = _reach_states.find(state->get_buchi_state());
            // assert valid since all states are reducible when considering
            // the sink progressing formula and key transitions in accepting states.
            assert(suc != std::end(_reach_states));
            if (suc != std::end(_reach_states) && !this->guard_valid(*state, suc->second._bddCond)) {
                _reach->set_buchi_conds(suc->second._ret_cond, suc->second._prog_cond, suc->second._pseudo_sink_cond);
                set_spooler(*_reach);
            }
            else {
                set_spooler(*_fallback_spooler);
            }
            ProductSuccessorGenerator<S>::prepare(state, sucinfo);
        }

    private:
        void set_spooler(SuccessorSpooler& spooler)
        {
            if constexpr (std::is_same_v<S, LTL::SpoolingSuccessorGenerator>)
                this->_successor_generator.set_spooler(spooler);
            else {
                assert(false);
                throw base_error("Fatal error");
            }
        }

        struct buchi_edge_t{
            bdd _bddCond;
            PetriEngine::PQL::Condition_ptr _ret_cond;
            PetriEngine::PQL::Condition_ptr _prog_cond;
            PetriEngine::PQL::Condition_ptr _pseudo_sink_cond;
        };

        std::unique_ptr<Spooler> _fallback_spooler;
        std::unique_ptr<LTL::SafeAutStubbornSet> _reach;
        std::unordered_map<size_t, buchi_edge_t> _reach_states;
        std::vector<PetriEngine::PQL::Condition_ptr> _progressing_formulae;

    };
}

#endif //VERIFYPN_REACHSTUBPRODUCTSUCCESSORGENERATOR_H
