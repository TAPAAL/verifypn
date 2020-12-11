/*
 * File:   LTLToBuchi.cpp
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 23/09/2020
 */

#include "LTL/LTLToBuchi.h"
#include "LTL/BuchiSuccessorGenerator.h"

#include <spot/twaalgos/translate.hh>
#include <spot/tl/parse.hh>
#include <spot/twa/bddprint.hh>
#include <sstream>
#include <spot/twaalgos/dot.hh>


namespace LTL {


    /**
     * Formula serializer to SPOT-compatible syntax.
     */


    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::NotCondition *element) {
        os << "(! ";
        (*element)[0]->visit(*this);
        os << ")";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::AndCondition *element) {
        QueryPrinter::_accept(element, "&&");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::OrCondition *element) {
        QueryPrinter::_accept(element, "||");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LessThanCondition *element) {
        make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) {
        make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::FireableCondition *element) {
        make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) {
        make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::GreaterThanCondition *element) {
        make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::GreaterThanOrEqualCondition *element) {
        make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::EqualCondition *element) {
        make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::NotEqualCondition *element) {
        make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const CompareConjunction *element) {
        make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::BooleanCondition *element) {
        os << (element->value ? "1" : "0");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LiteralExpr *element) {
        assert(false);
        std::cerr << "LiteralExpr should not be visited by Spot serializer" << std::endl;
        exit(1);
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::PlusExpr *element) {
        assert(false);
        std::cerr << "PlusExpr should not be visited by Spot serializer" << std::endl;
        exit(1);
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::MultiplyExpr *element) {
        assert(false);
        std::cerr << "MultiplyExpr should not be visited by Spot serializer" << std::endl;
        exit(1);
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::MinusExpr *element) {
        assert(false);
        std::cerr << "MinusExpr should not be visited by Spot serializer" << std::endl;
        exit(1);
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::SubtractExpr *element) {
        assert(false);
        std::cerr << "LiteralExpr should not be visited by Spot serializer" << std::endl;
        exit(1);
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::IdentifierExpr *element) {
        assert(false);
        std::cerr << "IdentifierExpr should not be visited by Spot serializer" << std::endl;
        exit(1);
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const ACondition *condition) {
        condition->operator[](0)->visit(*this);
    }

    void FormulaToSpotSyntax::_accept(const ECondition *condition) {
        condition->operator[](0)->visit(*this);
    }

    std::pair<spot::formula, APInfo> to_spot_formula(const Condition_ptr& query) {
        std::stringstream ss;
        FormulaToSpotSyntax spotConverter{ss};
        query->visit(spotConverter);
        std::string spotFormula = ss.str();
        if (spotFormula.at(0) == 'E' || spotFormula.at(0) == 'A') {
            spotFormula = spotFormula.substr(2);
        }
        auto spot_formula = spot::parse_formula(spotFormula);
#ifdef PRINTF_DEBUG
        std::cerr << "ORIG FORMULA: \n  " << ss.str() << std::endl;
        std::cerr << "SPOT FORMULA: \n  " << spotFormula << std::endl;
#endif
        return std::make_pair(spot_formula, spotConverter.apInfo());
    }

    BuchiSuccessorGenerator makeBuchiAutomaton(const Condition_ptr &query) {
        auto [formula, apinfo] = to_spot_formula(query);
        formula = spot::formula::Not(formula);
        spot::translator translator;
        translator.set_type(spot::postprocessor::BA);
        translator.set_pref(spot::postprocessor::Complete);
        spot::twa_graph_ptr automaton = translator.run(formula);
#ifdef PRINTF_DEBUG
        automaton->get_graph().dump_storage(std::cerr);
        spot::print_dot(std::cerr, automaton);
#endif
        std::unordered_map<int, AtomicProposition> ap_map;
        // bind PQL expressions to the atomic proposition IDs used by spot.
        // the resulting map can be indexed using variables mentioned on edges of the created Büchi automaton.
        for (const auto &info : apinfo) {
            int varnum = automaton->register_ap(info.text);
            ap_map[varnum] = info;
        }
#ifdef PRINTF_DEBUG
        automaton->get_dict()->dump(std::cerr);
#endif

        return BuchiSuccessorGenerator{Structures::BuchiAutomaton{std::move(automaton), std::move(ap_map)}};
    }

}
