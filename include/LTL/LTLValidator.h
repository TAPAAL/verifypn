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

#ifndef VERIFYPN_LTLVALIDATOR_H
#define VERIFYPN_LTLVALIDATOR_H

#include "PetriEngine/PQL/Visitor.h"

namespace LTL {
    using namespace PetriEngine::PQL;
    class LTLValidator : public PetriEngine::PQL::Visitor {
    public:
        bool bad() const { return _bad; }

        operator bool() const { return !bad(); }

        bool isLTL(const Condition_ptr& condition) {
            std::shared_ptr<SimpleQuantifierCondition> quantifierCondition;
            if ((quantifierCondition = std::dynamic_pointer_cast<ACondition>(condition)) != nullptr ||
                (quantifierCondition = std::dynamic_pointer_cast<ECondition>(condition)) != nullptr ){
                (*quantifierCondition)[0]->visit(*this);
            } else {
                condition->visit(*this);
            }
            return !bad();
        }

    protected:

        void _visitNary(const LogicalCondition *condition) {
            for (const auto &cond : *condition) {
                cond->visit(*this);
            }
        };

        void _accept(const PetriEngine::PQL::EFCondition *condition) override {
            setBad();
            std::cerr << "found EFCondition" << std::endl;
        }

        void _accept(const PetriEngine::PQL::EGCondition *condition) override {
            setBad();
            std::cerr << "found EGCondition" << std::endl;
        }

        void _accept(const PetriEngine::PQL::AGCondition *condition) override {
            setBad();
            std::cerr << "found AGCondition" << std::endl;
        }

        void _accept(const PetriEngine::PQL::AFCondition *condition) override {
            setBad();
            std::cerr << "found AFCondition" << std::endl;
        }

        void _accept(const PetriEngine::PQL::EXCondition *condition) override {
            setBad();
            std::cerr << "found EXCondition" << std::endl;
        }

        void _accept(const PetriEngine::PQL::AXCondition *condition) override {
            setBad();
            std::cerr << "found AXCondition" << std::endl;
        }

        void _accept(const PetriEngine::PQL::EUCondition *condition) override {
            setBad();
            std::cerr << "found EUCondition" << std::endl;
        }

        void _accept(const PetriEngine::PQL::AUCondition *condition) override {
            setBad();
            std::cerr << "found AUCondition" << std::endl;
        }

        void _accept(const PetriEngine::PQL::ACondition *condition) override {
            setBad();
        }

        void _accept(const PetriEngine::PQL::ECondition *condition) override {
            setBad();
        }

        void _accept(const PetriEngine::PQL::NotCondition *element) override {
            (*element)[0]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::AndCondition *element) override {
            _visitNary(element);
        }

        void _accept(const PetriEngine::PQL::OrCondition *element) override {
            _visitNary(element);
        }

        void _accept(const PetriEngine::PQL::LessThanCondition *element) override {
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) override {
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::GreaterThanCondition *element) override {
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::GreaterThanOrEqualCondition *element) override {
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::EqualCondition *element) override {
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::NotEqualCondition *element) override {
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::DeadlockCondition *element) override {

        }

        void _accept(const PetriEngine::PQL::CompareConjunction *element) override {

        }

        void _accept(const PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) override {

        }

        void _accept(const PetriEngine::PQL::GCondition *condition) override {
            (*condition)[0]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::FCondition *condition) override {
            (*condition)[0]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::XCondition *condition) override {
            (*condition)[0]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::UntilCondition *condition) override {
            (*condition)[0]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) override {
        }

        void _accept(const PetriEngine::PQL::FireableCondition *element) override {
        }

        void _accept(const PetriEngine::PQL::UpperBoundsCondition *element) override {
        }

        void _accept(const PetriEngine::PQL::LivenessCondition *element) override {
        }

        void _accept(const PetriEngine::PQL::KSafeCondition *element) override {
        }

        void _accept(const PetriEngine::PQL::QuasiLivenessCondition *element) override {
        }

        void _accept(const PetriEngine::PQL::StableMarkingCondition *element) override {
        }

        void _accept(const PetriEngine::PQL::BooleanCondition *element) override {}

        void _accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element) override {}

        void _accept(const PetriEngine::PQL::LiteralExpr *element) override {}

        void _accept(const PetriEngine::PQL::PlusExpr *element) override {}

        void _accept(const PetriEngine::PQL::MultiplyExpr *element) override {}

        void _accept(const PetriEngine::PQL::MinusExpr *element) override {}

        void _accept(const PetriEngine::PQL::SubtractExpr *element) override {}

        void _accept(const PetriEngine::PQL::IdentifierExpr *element) override {}

    private:
        bool _bad = false;

        void setBad() { _bad = true; }
    };
}
#endif //VERIFYPN_LTLVALIDATOR_H
