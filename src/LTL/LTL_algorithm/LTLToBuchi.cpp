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
    class FormulaToSpotSyntax : public PetriEngine::PQL::QueryPrinter {
    protected:
        void _accept(const PetriEngine::PQL::NotCondition *element) override;

        void _accept(const PetriEngine::PQL::AndCondition *element) override;

        void _accept(const PetriEngine::PQL::OrCondition *element) override;

        void _accept(const PetriEngine::PQL::LessThanCondition *element) override;

        void _accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::GreaterThanCondition *element) override;

        void _accept(const PetriEngine::PQL::GreaterThanOrEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::EqualCondition *element) override;

        void _accept(const PetriEngine::PQL::NotEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) override;

        void _accept(const PetriEngine::PQL::FireableCondition *element) override;

        void _accept(const PetriEngine::PQL::BooleanCondition *element) override;

        void _accept(const PetriEngine::PQL::LiteralExpr *element) override;

        void _accept(const PetriEngine::PQL::PlusExpr *element) override;

        void _accept(const PetriEngine::PQL::MultiplyExpr *element) override;

        void _accept(const PetriEngine::PQL::MinusExpr *element) override;

        void _accept(const PetriEngine::PQL::SubtractExpr *element) override;

        void _accept(const PetriEngine::PQL::IdentifierExpr *element) override;

    public:

        explicit FormulaToSpotSyntax(std::ostream &os = std::cout)
                : PetriEngine::PQL::QueryPrinter(os) {}

        auto begin() const {
            return std::begin(ap_info);
        }

        auto end() const {
            return std::end(ap_info);
        }

        const APInfo &getAPInfo() const {
            return ap_info;
        }

    private:
        APInfo ap_info;
        bool is_quoted = false;

        void make_atomic_prop(const Condition_constptr &element) {
            auto cond = const_cast<Condition *>(element.get())->shared_from_this();
            std::stringstream ss;
            ss << "\"";
            QueryPrinter _printer{ss};
            cond->visit(_printer);
            ss << "\"";
            os << ss.str();
            ap_info.push_back(AtomicProposition{cond, ss.str().substr(1, ss.str().size() - 2)});
        }
    };


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

    BuchiSuccessorGenerator makeBuchiAutomaton(const Condition_ptr &query) {
        std::stringstream ss;
        FormulaToSpotSyntax spotConverter{ss};
        query->visit(spotConverter);
        const std::string spotFormula = "!(" + ss.str() + ")";
#ifdef PRINTF_DEBUG
        std::cerr << "ORIG FORMULA: \n  " << ss.str() << std::endl;
        std::cerr << "SPOT FORMULA: \n  " << spotFormula << std::endl;
#endif
        spot::formula formula = spot::parse_formula(spotFormula);
        //spot::bdd_dict_ptr bdd = spot::make_bdd_dict();
        spot::translator translator;
        translator.set_pref(spot::postprocessor::Complete
                            | spot::postprocessor::SBAcc);
        spot::twa_graph_ptr automaton = translator.run(formula);
#ifdef PRINTF_DEBUG
        automaton->get_graph().dump_storage(std::cerr);
        spot::print_dot(std::cerr, automaton);
        bdd->dump(std::cerr);
#endif
        std::unordered_map<int, AtomicProposition> ap_map;
        // bind PQL expressions to the atomic proposition IDs used by spot.
        // the resulting map can be indexed using variables mentioned on edges of the created Büchi automaton.
        for (const auto &apinfo : spotConverter) {
            int varnum = automaton->register_ap(apinfo.text);
            ap_map[varnum] = apinfo;
        }

        return BuchiSuccessorGenerator{Structures::BuchiAutomaton{std::move(automaton), std::move(ap_map)}};
    }

}