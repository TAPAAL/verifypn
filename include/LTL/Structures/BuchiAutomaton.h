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

#ifndef VERIFYPN_BUCHIAUTOMATON_H
#define VERIFYPN_BUCHIAUTOMATON_H

#include "LTL/LTLToBuchi.h"
#include "LTL/LTLOptions.h"
#include "PetriEngine/PQL/Evaluation.h"

#include <spot/twa/twagraph.hh>
#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/neverclaim.hh>

#include <unordered_map>

namespace LTL { namespace Structures {
    class BuchiAutomaton {
    private:
        spot::twa_graph_ptr _buchi = nullptr;
        std::unordered_map<int, AtomicProposition> _ap_info;

    public:
        BuchiAutomaton(spot::twa_graph_ptr buchi, std::unordered_map<int, AtomicProposition> apInfo)
                : _buchi(std::move(buchi)), _ap_info(std::move(apInfo)) {
        }

        BuchiAutomaton() {};

        BuchiAutomaton(BuchiAutomaton&&) = default;
        BuchiAutomaton& operator=(BuchiAutomaton&& other) = default;

        BuchiAutomaton(const BuchiAutomaton&) = default;
        BuchiAutomaton& operator=(const BuchiAutomaton&) = default;

        const spot::twa_graph& buchi() const {
            return *_buchi;
        }

        const spot::twa_graph_ptr& buchi_ptr() const {
            return _buchi;
        }

        const std::unordered_map<int, AtomicProposition>& ap_info() const {
            return _ap_info;
        }

        void output_buchi(std::ostream& os, BuchiOutType type)
        {
            switch (type) {
                case BuchiOutType::Dot:
                    spot::print_dot(os, _buchi);
                    break;
                case BuchiOutType::HOA:
                    spot::print_hoa(os, _buchi, "s");
                    break;
                case BuchiOutType::Spin:
                    spot::print_never_claim(os, _buchi);
                    break;
            }
        }

        /**
         * Evaluate binary decision diagram (BDD) representation of transition guard in given state.
         */
        bool guard_valid(PetriEngine::PQL::EvaluationContext &ctx, bdd bdd) const
        {
            // IDs 0 and 1 are false and true atoms, respectively
            // More details in buddy manual ( http://buddy.sourceforge.net/manual/main.html )
            while (bdd.id() > 1) {
                // find variable to test, and test it
                size_t var = bdd_var(bdd);
                using PetriEngine::PQL::Condition;
                Condition::Result res = PetriEngine::PQL::evaluate(_ap_info.at(var)._expression.get(), ctx);
                switch (res) {
                    case Condition::RUNKNOWN:
                        assert(false);
                        throw base_error("Unexpected unknown answer from evaluating query!");
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
    };
} }

#endif //VERIFYPN_BUCHIAUTOMATON_H
