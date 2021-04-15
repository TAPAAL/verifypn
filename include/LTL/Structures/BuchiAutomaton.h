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
#include "LTL/AlgorithmTypes.h"
#include <spot/twa/twagraph.hh>
#include <unordered_map>

#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/neverclaim.hh>

namespace LTL::Structures {
    struct BuchiAutomaton {
        BuchiAutomaton(spot::twa_graph_ptr buchi, std::unordered_map<int, AtomicProposition> apInfo)
                : _buchi(std::move(buchi)), ap_info(std::move(apInfo)) {
            dict = _buchi->get_dict();
        }

        spot::twa_graph_ptr _buchi;
        const std::unordered_map<int, AtomicProposition> ap_info;
        spot::bdd_dict_ptr dict;


        void output_buchi(const std::string& file, BuchiOutType type)
        {
            std::ofstream fs(file);
            switch (type) {
                case BuchiOutType::Dot:
                    spot::print_dot(fs, _buchi);
                    break;
                case BuchiOutType::HOA:
                    spot::print_hoa(fs, _buchi, "s");
                    break;
                case BuchiOutType::Spin:
                    spot::print_never_claim(fs, _buchi);
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
                Condition::Result res = ap_info.at(var).expression->evaluate(ctx);
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
    };
}

#endif //VERIFYPN_BUCHIAUTOMATON_H
