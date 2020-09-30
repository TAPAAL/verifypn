/*
 * File:   LTLValidator.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 28/09/2020
 */

#ifndef VERIFYPN_LTLVALIDATOR_H
#define VERIFYPN_LTLVALIDATOR_H

#include "PetriEngine/PQL/Visitor.h"

namespace LTL {
    class LTLValidator : public PetriEngine::PQL::Visitor {
    public:
        bool bad() const { return _bad; }

        operator bool() const { return bad(); }

    protected:
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
            std::cerr << "found ACondition" << std::endl;
        }

        void _accept(const PetriEngine::PQL::ECondition *condition) override {
            setBad();
            std::cerr << "found ECondition" << std::endl;
        }

        void _accept(const PetriEngine::PQL::NotCondition *element) override {
            (*element)[0]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::AndCondition *element) override {
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void _accept(const PetriEngine::PQL::OrCondition *element) override {
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
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
            Visitor::_accept(element);
        }

        void _accept(const PetriEngine::PQL::FireableCondition *element) override {
            Visitor::_accept(element);
        }

        void _accept(const PetriEngine::PQL::UpperBoundsCondition *element) override {
            Visitor::_accept(element);
        }

        void _accept(const PetriEngine::PQL::LivenessCondition *element) override {
            Visitor::_accept(element);
        }

        void _accept(const PetriEngine::PQL::KSafeCondition *element) override {
            Visitor::_accept(element);
        }

        void _accept(const PetriEngine::PQL::QuasiLivenessCondition *element) override {
            Visitor::_accept(element);
        }

        void _accept(const PetriEngine::PQL::StableMarkingCondition *element) override {
            Visitor::_accept(element);
        }

        void _accept(const PetriEngine::PQL::BooleanCondition *element) override {
            Visitor::_accept(element);
        }

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
