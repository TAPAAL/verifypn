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

#include "LTL/LTLToBuchi.h"
#include "LTL/Structures/BuchiAutomaton.h"
#include "PetriEngine/PQL/QueryPrinter.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
#include "PetriEngine/PQL/FormulaSize.h"

#include <spot/twaalgos/translate.hh>
#include <spot/tl/parse.hh>
#include <spot/twa/bddprint.hh>
#include <sstream>
#include <spot/twaalgos/dot.hh>

using namespace PetriEngine::PQL;

namespace LTL {


    bool is_spot_too_many_children(const std::runtime_error& err)
    {
        // I wish spot would have better exceptions.
        // I also wish that we would too.
        return std::strcmp(err.what(), "too many children for formula") == 0;
    }

    /**
     * Formula serializer to SPOT-compatible syntax.
     */
    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::NotCondition *element) {
        Visitor::visit(this, (*element)[0]);
        _formula = spot::formula::Not(_formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::AndCondition *element) {
        std::vector<spot::formula> temp_ops;
        std::vector<Condition_ptr> non_temp;
        for(auto& e : *element)
        {
            if(isTemporal(e))
            {
                Visitor::visit(this, e);
                temp_ops.emplace_back(_formula);
            }
            else non_temp.emplace_back(e);
        }
        if(!non_temp.empty())
        {
            if(_expand)
            {
                std::vector<spot::formula> non_temp_ops;
                for(auto& e : non_temp)
                {
                    Visitor::visit(this, e);
                    non_temp_ops.emplace_back(_formula);
                }
                temp_ops.emplace_back(spot::formula::And(non_temp_ops));
            }
            else
            {
                temp_ops.emplace_back(make_atomic_prop(std::make_shared<AndCondition>(non_temp)));
            }
        }
        _formula = spot::formula::And(std::move(temp_ops));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::OrCondition *element) {
        std::vector<spot::formula> temp_ops;
        std::vector<Condition_ptr> non_temp;
        for(auto& e : *element)
        {
            if(isTemporal(e))
            {
                Visitor::visit(this, e);
                temp_ops.emplace_back(_formula);
            }
            else non_temp.emplace_back(e);
        }
        if(!non_temp.empty())
        {
            if(_expand)
            {
                std::vector<spot::formula> non_temp_ops;
                for(auto& e : non_temp)
                {
                    Visitor::visit(this, e);
                    non_temp_ops.emplace_back(_formula);
                }
                temp_ops.emplace_back(spot::formula::Or(non_temp_ops));
            }
            else
            {
                temp_ops.emplace_back(make_atomic_prop(std::make_shared<OrCondition>(non_temp)));
            }
        }
        _formula = spot::formula::Or(std::move(temp_ops));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::XCondition *element) {
        Visitor::visit(this, (*element)[0]);
        _formula = spot::formula::X(_formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::EXCondition *element) {
        Visitor::visit(this, (*element)[0]);
        _formula = spot::formula::X(_formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::AXCondition *element) {
        Visitor::visit(this, (*element)[0]);
        _formula = spot::formula::X(_formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::FCondition *element) {
        Visitor::visit(this, (*element)[0]);
        _formula = spot::formula::F(_formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::EFCondition *element) {
        Visitor::visit(this, (*element)[0]);
        _formula = spot::formula::F(_formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::AFCondition *element) {
        Visitor::visit(this, (*element)[0]);
        _formula = spot::formula::F(_formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::GCondition *element) {
        Visitor::visit(this, (*element)[0]);
        _formula = spot::formula::G(_formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::EGCondition *element) {
        Visitor::visit(this, (*element)[0]);
        _formula = spot::formula::G(_formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::AGCondition *element) {
        Visitor::visit(this, (*element)[0]);
        _formula = spot::formula::G(_formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::UntilCondition *element) {
        Visitor::visit(this, (*element)[0]);
        auto lhs = _formula;
        Visitor::visit(this, (*element)[1]);
        _formula = spot::formula::U(lhs, _formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::AUCondition *element) {
        Visitor::visit(this, (*element)[0]);
        auto lhs = _formula;
        Visitor::visit(this, (*element)[1]);
        _formula = spot::formula::U(lhs, _formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::EUCondition *element) {
        Visitor::visit(this, (*element)[0]);
        auto lhs = _formula;
        Visitor::visit(this, (*element)[1]);
        _formula = spot::formula::U(lhs, _formula);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LessThanCondition *element) {
        _formula = make_atomic_prop(std::make_shared<LessThanCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) {
        _formula = make_atomic_prop(std::make_shared<UnfoldedFireableCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::FireableCondition *element) {
        _formula = make_atomic_prop(std::make_shared<FireableCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) {
        _formula = make_atomic_prop(std::make_shared<LessThanOrEqualCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::EqualCondition *element) {
        _formula = make_atomic_prop(std::make_shared<EqualCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::NotEqualCondition *element) {
        _formula = make_atomic_prop(std::make_shared<NotEqualCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::CompareConjunction *element) {
        _formula = make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::DeadlockCondition *element) {
        _formula = make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::BooleanCondition *element) {
        _formula = (element->value ? spot::formula::tt() : spot::formula::ff());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LiteralExpr *element) {
        assert(false);
        throw base_error("LiteralExpr should not be visited by Spot serializer");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::PlusExpr *element) {
        assert(false);
        throw base_error("PlusExpr should not be visited by Spot serializer");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::MultiplyExpr *element) {
        assert(false);
        throw base_error("MultiplyExpr should not be visited by Spot serializer");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::MinusExpr *element) {
        assert(false);
        throw base_error("MinusExpr should not be visited by Spot serializer");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::SubtractExpr *element) {
        assert(false);
        throw base_error("LiteralExpr should not be visited by Spot serializer");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::IdentifierExpr *element) {
        assert(false);
        throw base_error("IdentifierExpr should not be visited by Spot serializer");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::ACondition *condition) {
        Visitor::visit(this, (*condition)[0]);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::ECondition *condition) {
        Visitor::visit(this, (*condition)[0]);
    }

    spot::formula FormulaToSpotSyntax::make_atomic_prop(const PetriEngine::PQL::Condition_constptr &element) {
        auto cond =
                const_cast<PetriEngine::PQL::Condition *>(element.get())->shared_from_this();
        std::stringstream ss;
        bool choice = _compress == APCompression::Choose && PetriEngine::PQL::formulaSize(element) > 250;
        if (_compress == APCompression::Full || choice) {
            // FIXME Very naive; this completely removes APs being in multiple places in the query,
            // leading to some query not being answered as is. The net gain is large in the firebaility category,
            // but ideally it would be possible to make a smarter approach that looks at previously stored APs
            // and efficiently checks for repeat APs such that we can reuse APs.
            ss << _ap_info.size();
        } else {
            PetriEngine::PQL::QueryPrinter _printer{ss};
            Visitor::visit(_printer, cond);
        }
        _ap_info.push_back(AtomicProposition{cond, ss.str()});
        return spot::formula::ap(_ap_info.back()._text);
    }

    std::pair<spot::formula, APInfo>
    to_spot_formula (const PetriEngine::PQL::Condition_ptr &query, APCompression compression) {
        FormulaToSpotSyntax spotConverter{compression, formulaSize(query) < 100};
        Visitor::visit(spotConverter, query);
        auto spot_formula = spotConverter.formula();
        return std::make_pair(spot_formula, spotConverter.apInfo());
    }

    Structures::BuchiAutomaton make_buchi_automaton(const PetriEngine::PQL::Condition_ptr &query, BuchiOptimization optimization, APCompression compression) {
        auto [formula, apinfo] = to_spot_formula(query, compression);
        formula = spot::formula::Not(formula);
        spot::translator translator;
        // Ask for Büchi acceptance (rather than generalized Büchi) and medium optimizations
        // (default is high which causes many worst case BDD constructions i.e. exponential blow-up)
        translator.set_type(spot::postprocessor::BA);
        spot::postprocessor::optimization_level level;
        switch(optimization) {
            case BuchiOptimization::Low:
                level = spot::postprocessor::Low;
                break;
            case BuchiOptimization::Medium:
                level = spot::postprocessor::Medium;
                break;
            case BuchiOptimization::High:
                level = spot::postprocessor::High;
                break;
        }
        translator.set_level(level);
        spot::twa_graph_ptr automaton = translator.run(formula);
        std::unordered_map<int, AtomicProposition> ap_map;
        // bind PQL expressions to the atomic proposition IDs used by spot.
        // the resulting map can be indexed using variables mentioned on edges of the created Büchi automaton.
        for (const auto &info : apinfo) {
            int varnum = automaton->register_ap(info._text);
            ap_map[varnum] = info;
        }

        return Structures::BuchiAutomaton{std::move(automaton), std::move(ap_map)};
    }

}
