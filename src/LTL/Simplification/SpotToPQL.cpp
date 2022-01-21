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

#include "LTL/Simplification/SpotToPQL.h"
#include "LTL/LTLToBuchi.h"
#include <algorithm>
#include <map>
#include <memory>

#include <spot/tl/formula.hh>
#include <spot/tl/simplify.hh>
#include <spot/tl/unabbrev.hh>

namespace LTL {
    using namespace PetriEngine::PQL;
    PetriEngine::PQL::Condition_ptr toPQL(const spot::formula &formula, const APInfo &apinfo) {

        switch (formula.kind()) {
            case spot::op::ff:
                return BooleanCondition::FALSE_CONSTANT;
            case spot::op::tt:
                return BooleanCondition::TRUE_CONSTANT;
            case spot::op::ap: {
                auto it = std::find_if(std::begin(apinfo), std::end(apinfo),
                                       [&](const AtomicProposition &info) {
                                           auto ap = std::string_view(info.text);
                                           return ap == std::string_view(formula.ap_name());
                                       });
                if (it == std::end(apinfo)) {
                    throw base_error("Error: Expected to find ", formula.ap_name(), " in APInfo.\n");
                } else {
                    return it->expression;
                }

            }
            case spot::op::Not:
                return std::make_shared<NotCondition>(toPQL(formula[0], apinfo));
            case spot::op::X:
                return std::make_shared<XCondition>(toPQL(formula[0], apinfo));
            case spot::op::F:
                return std::make_shared<FCondition>(toPQL(formula[0], apinfo));
            case spot::op::G:
                return std::make_shared<GCondition>(toPQL(formula[0], apinfo));
            case spot::op::U:
                return std::make_shared<UntilCondition>(
                        toPQL(formula[0], apinfo), toPQL(formula[1], apinfo));
            case spot::op::Or: {
                std::vector<Condition_ptr> conds;
                std::transform(std::begin(formula), std::end(formula), std::back_insert_iterator(conds),
                               [&](auto f) { return toPQL(f, apinfo); });
                return std::make_shared<OrCondition>(conds);
            }
            case spot::op::And: {
                std::vector<Condition_ptr> conds;
                std::transform(std::begin(formula), std::end(formula), std::back_insert_iterator(conds),
                               [&](auto f) { return toPQL(f, apinfo); });
                return std::make_shared<AndCondition>(conds);
            }
            case spot::op::R:
                throw base_error("Unimplemented: R");
            case spot::op::W:
                throw base_error("Unimplemented: W");
            case spot::op::M:
                throw base_error("Unimplemented: M");
            case spot::op::eword:
                throw base_error("Unimplemented: eword");
            case spot::op::Closure:
                throw base_error("Unimplemented: Closure");
            case spot::op::NegClosure:
                throw base_error("Unimplemented: NegClosure");
            case spot::op::NegClosureMarked:
                throw base_error("Unimplemented: NegClosureMarked");
            case spot::op::Xor:
                throw base_error("Unimplemented: Xor");
            case spot::op::Implies:
                throw base_error("Unimplemented: Implies");
            case spot::op::Equiv:
                throw base_error("Unimplemented: Equiv");
            case spot::op::EConcat:
                throw base_error("Unimplemented: EConcat");
            case spot::op::EConcatMarked:
                throw base_error("Unimplemented: EConcatMarked");
            case spot::op::UConcat:
                throw base_error("Unimplemented: UConcat");
            case spot::op::OrRat:
                throw base_error("Unimplemented: OrRat");
            case spot::op::AndRat:
                throw base_error("Unimplemented: AndRat");
            case spot::op::AndNLM:
                throw base_error("Unimplemented: AndNLM");
            case spot::op::Concat:
                throw base_error("Unimplemented: Concat");
            case spot::op::Fusion:
                throw base_error("Unimplemented: Fusion");

            case spot::op::Star:
                throw base_error("Unimplemented: Star");

            case spot::op::FStar:
                throw base_error("Unimplemented: FStar");
            case spot::op::first_match:
                throw base_error("Unimplemented: first_match");
            default:
                std::stringstream ss;
                formula.dump(ss);
                throw base_error("Found unrecognized op in formula ", ss.str());
        }
    }

    PetriEngine::PQL::Condition_ptr simplify(const PetriEngine::PQL::Condition_ptr &formula, const options_t &options) {
        using namespace PetriEngine::PQL;
        if (auto e = std::dynamic_pointer_cast<ECondition>(formula); e != nullptr) {
            return std::make_shared<ECondition>(simplify((*e)[0], options));
        } else if (auto a = std::dynamic_pointer_cast<ACondition>(formula); a != nullptr) {
            return std::make_shared<ACondition>(simplify((*a)[0], options));
        }
        auto[f, apinfo] = LTL::to_spot_formula(formula, options);
        spot::tl_simplifier simplifier{static_cast<int>(options.buchiOptimization)};
        f = simplifier.simplify(f);
        // spot simplifies using unsupported operators R, W, and M, which we now remove.
        f = spot::unabbreviate(f, "RWM");
        return toPQL(f, apinfo);
    }
} // namespace LTL
