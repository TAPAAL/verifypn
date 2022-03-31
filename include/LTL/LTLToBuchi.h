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

#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/PQL/FormulaSize.h"
#include "LTLOptions.h"

#include <iostream>
#include <string>

#include <spot/tl/formula.hh>

namespace LTL {
    struct AtomicProposition {
        PetriEngine::PQL::Condition_ptr _expression;
        std::string _text;
    };

    using APInfo = std::vector<AtomicProposition>;

    std::pair<spot::formula, APInfo>
    to_spot_formula(const PetriEngine::PQL::Condition_ptr &query, APCompression compression);

    class BuchiSuccessorGenerator;
    namespace Structures {
        class BuchiAutomaton;
    }

    Structures::BuchiAutomaton make_buchi_automaton(
            const PetriEngine::PQL::Condition_ptr &query,
            BuchiOptimization optimization, APCompression compression);

    class FormulaToSpotSyntax : public PetriEngine::PQL::Visitor {
    protected:
        void _accept(const PetriEngine::PQL::ACondition *condition) override;

        void _accept(const PetriEngine::PQL::ECondition *condition) override;

        void _accept(const PetriEngine::PQL::NotCondition *element) override;

        void _accept(const PetriEngine::PQL::AndCondition *element) override;

        void _accept(const PetriEngine::PQL::OrCondition *element) override;

        void _accept(const PetriEngine::PQL::LessThanCondition *element) override;

        void _accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::EqualCondition *element) override;

        void _accept(const PetriEngine::PQL::NotEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) override;

        void _accept(const PetriEngine::PQL::FireableCondition *element) override;

        void _accept(const PetriEngine::PQL::BooleanCondition *element) override;

        void _accept(const PetriEngine::PQL::DeadlockCondition *element) override;

        void _accept(const PetriEngine::PQL::LiteralExpr *element) override;

        void _accept(const PetriEngine::PQL::PlusExpr *element) override;

        void _accept(const PetriEngine::PQL::MultiplyExpr *element) override;

        void _accept(const PetriEngine::PQL::MinusExpr *element) override;

        void _accept(const PetriEngine::PQL::SubtractExpr *element) override;

        void _accept(const PetriEngine::PQL::IdentifierExpr *element) override;

        void _accept(const PetriEngine::PQL::CompareConjunction *element) override;

        void _accept(const PetriEngine::PQL::EFCondition *condition) override;

        void _accept(const PetriEngine::PQL::EGCondition *condition) override;

        void _accept(const PetriEngine::PQL::AGCondition *condition)  override;

        void _accept(const PetriEngine::PQL::AFCondition *condition)  override;

        void _accept(const PetriEngine::PQL::EXCondition *condition) override;

        void _accept(const PetriEngine::PQL::AXCondition *condition) override;

        void _accept(const PetriEngine::PQL::EUCondition *condition) override;

        void _accept(const PetriEngine::PQL::AUCondition *condition) override;

        void _accept(const PetriEngine::PQL::GCondition *condition)  override;

        void _accept(const PetriEngine::PQL::FCondition *condition) override;

        void _accept(const PetriEngine::PQL::XCondition *condition) override;

        void _accept(const PetriEngine::PQL::UntilCondition *condition) override;

    public:

        explicit FormulaToSpotSyntax(APCompression compress_aps = APCompression::Choose, bool expand = true)
                : _compress(compress_aps), _expand(expand) {}


        auto begin() const
        {
            return std::begin(_ap_info);
        }

        auto end() const
        {
            return std::end(_ap_info);
        }

        const APInfo &apInfo() const
        {
            return _ap_info;
        }
        spot::formula& formula() { return _formula; }
    private:
        APInfo _ap_info;
        APCompression _compress;
        spot::formula _formula;
        const bool _expand;
        spot::formula make_atomic_prop(const PetriEngine::PQL::Condition_constptr &element);
    };

}

#endif //VERIFYPN_LTLTOBUCHI_H
