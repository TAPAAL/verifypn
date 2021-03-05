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

#ifndef VERIFYPN_LTLTOBUCHI_H
#define VERIFYPN_LTLTOBUCHI_H

#include "PetriParse/QueryParser.h"
#include "PetriEngine/PQL/QueryPrinter.h"

#include <iostream>
#include <string>

#include <spot/tl/formula.hh>

namespace LTL {
    struct AtomicProposition {
        PetriEngine::PQL::Condition_ptr expression;
        std::string text;
    };

    using APInfo = std::vector<AtomicProposition>;
    std::string toSpotFormat(const QueryItem &query);
    void toSpotFormat(const QueryItem &query, std::ostream &os);
    std::pair<spot::formula, APInfo> to_spot_formula(const PetriEngine::PQL::Condition_ptr& query);

    class BuchiSuccessorGenerator;
    BuchiSuccessorGenerator makeBuchiAutomaton(const PetriEngine::PQL::Condition_ptr &query);


    class FormulaToSpotSyntax : public PetriEngine::PQL::QueryPrinter {
    protected:
        void _accept(const PetriEngine::PQL::ACondition *condition) override;

        void _accept(const PetriEngine::PQL::ECondition *condition) override;

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

        void _accept(const PetriEngine::PQL::CompareConjunction *element) override;

    public:

        explicit FormulaToSpotSyntax(std::ostream &os = std::cout)
                : PetriEngine::PQL::QueryPrinter(os) {}

        auto begin() const {
            return std::begin(ap_info);
        }

        auto end() const {
            return std::end(ap_info);
        }

        const APInfo &apInfo() const {
            return ap_info;
        }

    private:
        APInfo ap_info;
        bool is_quoted = false;

        void make_atomic_prop(const PetriEngine::PQL::Condition_constptr &element) {
            auto cond = const_cast<PetriEngine::PQL::Condition *>(element.get())->shared_from_this();
            std::stringstream ss;
            ss << "\"";
            PetriEngine::PQL::QueryPrinter _printer{ss};
            cond->visit(_printer);
            ss << "\"";
            os << ss.str();
            ap_info.push_back(AtomicProposition{cond, ss.str().substr(1, ss.str().size() - 2)});
        }
    };

}

#endif //VERIFYPN_LTLTOBUCHI_H
