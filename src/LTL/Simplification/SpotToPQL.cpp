/*
 * File:   SpotToPQL.cpp
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 12/11/2020
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
    using std::make_shared;

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
                    std::cerr << "Error: Expected to find " << formula.ap_name() << " in APInfo.\n";
                    exit(EXIT_FAILURE);
                } else {
                    return it->expression;
                }

            }
            case spot::op::Not:
                return make_shared<NotCondition>(toPQL(formula[0], apinfo));
            case spot::op::X:
                return make_shared<XCondition>(toPQL(formula[0], apinfo));
            case spot::op::F:
                return make_shared<FCondition>(toPQL(formula[0], apinfo));
            case spot::op::G:
                return make_shared<GCondition>(toPQL(formula[0], apinfo));
            case spot::op::U:
                return make_shared<UntilCondition>(
                        toPQL(formula[0], apinfo), toPQL(formula[1], apinfo));
            case spot::op::Or: {
                std::vector<Condition_ptr> conds;
                std::transform(std::begin(formula), std::end(formula), std::back_insert_iterator(conds),
                               [&](auto f) { return toPQL(f, apinfo); });
                return make_shared<OrCondition>(conds);
            }
            case spot::op::And: {
                std::vector<Condition_ptr> conds;
                std::transform(std::begin(formula), std::end(formula), std::back_insert_iterator(conds),
                               [&](auto f) { return toPQL(f, apinfo); });
                return make_shared<AndCondition>(conds);
            }
            case spot::op::R:
                std::cerr << "Unimplemented: R" << std::endl;
                exit(0);
            case spot::op::W:
                std::cerr << "Unimplemented: W" << std::endl;
                exit(0);
            case spot::op::M:
                std::cerr << "Unimplemented: M" << std::endl;
                exit(0);
            case spot::op::eword:
                std::cerr << "Unimplemented: eword" << std::endl;
                exit(0);
            case spot::op::Closure:
                std::cerr << "Unimplemented: Closure" << std::endl;
                exit(0);
            case spot::op::NegClosure:
                std::cerr << "Unimplemented: NegClosure" << std::endl;
                exit(0);
            case spot::op::NegClosureMarked:
                std::cerr << "Unimplemented: NegClosureMarked" << std::endl;
                exit(0);
            case spot::op::Xor:
                std::cerr << "Unimplemented: Xor" << std::endl;
                exit(0);
            case spot::op::Implies:
                std::cerr << "Unimplemented: Implies" << std::endl;
                exit(0);
            case spot::op::Equiv:
                std::cerr << "Unimplemented: Equiv" << std::endl;
                exit(0);
            case spot::op::EConcat:
                std::cerr << "Unimplemented: EConcat" << std::endl;
                exit(0);
            case spot::op::EConcatMarked:
                std::cerr << "Unimplemented: EConcatMarked" << std::endl;
                exit(0);
            case spot::op::UConcat:
                std::cerr << "Unimplemented: UConcat" << std::endl;
                exit(0);
            case spot::op::OrRat:
                std::cerr << "Unimplemented: OrRat" << std::endl;
                exit(0);
            case spot::op::AndRat:
                std::cerr << "Unimplemented: AndRat" << std::endl;
                exit(0);
            case spot::op::AndNLM:
                std::cerr << "Unimplemented: AndNLM" << std::endl;
                exit(0);
            case spot::op::Concat:
                std::cerr << "Unimplemented: Concat" << std::endl;
                exit(0);
            case spot::op::Fusion:
                std::cerr << "Unimplemented: Fusion" << std::endl;
                exit(0);

            case spot::op::Star:
                std::cerr << "Unimplemented: Star" << std::endl;
                exit(0);

            case spot::op::FStar:
                std::cerr << "Unimplemented: FStar" << std::endl;
                exit(0);
            case spot::op::first_match:
                std::cerr << "Unimplemented: first_match" << std::endl;
                exit(0);
            default:
                std::cerr << "Found unrecognized op in formula ";
                formula.dump(std::cerr) << std::endl;
                exit(0);
        }
    }

    PetriEngine::PQL::Condition_ptr simplify(const PetriEngine::PQL::Condition_ptr& formula) {
        if (auto e = dynamic_pointer_cast<ECondition>(formula); e != nullptr) {
            return std::make_shared<ECondition>(simplify((*e)[0]));
        } else if (auto a = dynamic_pointer_cast<ACondition>(formula); a != nullptr) {
            return std::make_shared<ACondition>(simplify((*a)[0]));
        }
        auto[f, apinfo] = LTL::to_spot_formula(formula);
        spot::tl_simplifier simplifier{3};
        f = simplifier.simplify(f);
        f = spot::unabbreviate(f, "RWM");
        return toPQL(f, apinfo);
    }
} // namespace LTL
