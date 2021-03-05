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
#include "PetriEngine/PQL/CTLVisitor.h"

#include <memory>

namespace PetriEngine::PQL {
    void IsCTLVisitor::_accept(const NotCondition *element) {
        (*element)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const LogicalCondition *element) {
        for (size_t i = 0; i < element->operands(); i++){
            (*element)[i]->visit(*this);
            if (_cur_type != CTLSyntaxType::BOOLEAN){
                isCTL = false;
                break;
            }
        }
    }

    void IsCTLVisitor::_accept(const AndCondition *element) {
        _accept((const LogicalCondition *) element);
    }

    void IsCTLVisitor::_accept(const OrCondition *element) {
        _accept((const LogicalCondition *) element);
    }

    void IsCTLVisitor::_accept(const CompareCondition *element) {
        //We are an atom. No need to check children as they are the same as CTL*
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const LessThanCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const LessThanOrEqualCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const GreaterThanCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const GreaterThanOrEqualCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const EqualCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const NotEqualCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const DeadlockCondition *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const CompareConjunction *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const UnfoldedUpperBoundsCondition *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const EFCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const EGCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const AGCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const AFCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const EXCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const AXCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const EUCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const AUCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const ACondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::PATH)
            isCTL = false;
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const ECondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::PATH)
            isCTL = false;
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const GCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
        _cur_type = CTLSyntaxType::PATH;
    }

    void IsCTLVisitor::_accept(const FCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
        _cur_type = CTLSyntaxType::PATH;
    }

    void IsCTLVisitor::_accept(const XCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
        _cur_type = CTLSyntaxType::PATH;
    }

    void IsCTLVisitor::_accept(const UntilCondition *condition) {
        (*condition)[0]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
        (*condition)[1]->visit(*this);
        if (_cur_type != CTLSyntaxType::BOOLEAN)
            isCTL = false;
        _cur_type = CTLSyntaxType::PATH;
    }

    void IsCTLVisitor::_accept(const UnfoldedFireableCondition *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const FireableCondition *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const UpperBoundsCondition *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const LivenessCondition *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const KSafeCondition *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const QuasiLivenessCondition *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const StableMarkingCondition *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const BooleanCondition *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const UnfoldedIdentifierExpr *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const LiteralExpr *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const PlusExpr *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const MultiplyExpr *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const MinusExpr *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const SubtractExpr *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const IdentifierExpr *element) {
        _cur_type = CTLSyntaxType::BOOLEAN;
    }


    void AsCTL::_accept(const NotCondition *element) {
        (*element)[0]->visit(*this);
        _ctl_query = std::make_shared<NotCondition>(_ctl_query);
    }

    template<typename T>
    void AsCTL::_acceptNary(const T *element) {
        std::vector<Condition_ptr> children;
        for (auto operand : *element){
            operand->visit(*this);
            children.push_back(_ctl_query);
        }
        _ctl_query = std::make_shared<T>(children);
    }

    void AsCTL::_accept(const AndCondition *element) {
        AsCTL::_acceptNary(element);
    }

    void AsCTL::_accept(const OrCondition *element) {
        AsCTL::_acceptNary(element);
    }

    template<typename T>
    std::shared_ptr<T> AsCTL::copy_compare_condition(const T *element) {
        // we copy of sharedptr for now, but this is not safe!
        // copy_narry_expr needs fixing.
        return std::make_shared<T>((*element)[0], (*element)[1]);
    }

    void AsCTL::_accept(const LessThanCondition *element) {
        _ctl_query = copy_compare_condition(element);
    }

    void AsCTL::_accept(const LessThanOrEqualCondition *element) {
        _ctl_query = copy_compare_condition(element);
    }

    void AsCTL::_accept(const GreaterThanCondition *element) {
        _ctl_query = copy_compare_condition(element);
    }

    void AsCTL::_accept(const GreaterThanOrEqualCondition *element) {
        _ctl_query = copy_compare_condition(element);
    }

    void AsCTL::_accept(const EqualCondition *element) {
        _ctl_query = copy_compare_condition(element);
    }

    void AsCTL::_accept(const NotEqualCondition *element) {
        _ctl_query = copy_compare_condition(element);
    }

    void AsCTL::_accept(const DeadlockCondition *element) {
        _ctl_query = std::make_shared<DeadlockCondition>();
    }

    void AsCTL::_accept(const CompareConjunction *element) {
        _ctl_query = std::make_shared<CompareConjunction>(*element);
    }

    void AsCTL::_accept(const UnfoldedUpperBoundsCondition *element) {
        _ctl_query = std::make_shared<UnfoldedUpperBoundsCondition>(*element);
    }

    void AsCTL::_accept(const EFCondition *condition) {
        (*condition)[0]->visit(*this);
        _ctl_query = std::make_shared<EFCondition>(_ctl_query);
    }

    void AsCTL::_accept(const EGCondition *condition) {
        (*condition)[0]->visit(*this);
        _ctl_query = std::make_shared<EGCondition>(_ctl_query);
    }

    void AsCTL::_accept(const AGCondition *condition) {
        (*condition)[0]->visit(*this);
        _ctl_query = std::make_shared<AGCondition>(_ctl_query);
    }

    void AsCTL::_accept(const AFCondition *condition) {
        (*condition)[0]->visit(*this);
        _ctl_query = std::make_shared<AFCondition>(_ctl_query);
    }

    void AsCTL::_accept(const EXCondition *condition) {
        (*condition)[0]->visit(*this);
        _ctl_query = std::make_shared<EXCondition>(_ctl_query);
    }

    void AsCTL::_accept(const AXCondition *condition) {
        (*condition)[0]->visit(*this);
        _ctl_query = std::make_shared<AXCondition>(_ctl_query);
    }

    void AsCTL::_accept(const EUCondition *condition) {
        (*condition)[0]->visit(*this);
        auto first = _ctl_query;
        (*condition)[1]->visit(*this);
        _ctl_query = std::make_shared<EUCondition>(first, _ctl_query);
    }

    void AsCTL::_accept(const AUCondition *condition) {
        (*condition)[0]->visit(*this);
        auto first = _ctl_query;
        (*condition)[1]->visit(*this);
        _ctl_query = std::make_shared<AUCondition>(first, _ctl_query);
    }

    void AsCTL::_accept(const ACondition *condition) {
        auto child = dynamic_cast<QuantifierCondition*>((*condition)[0].get());
        switch (child->getPath()) {
            case Path::G:
                (*child)[0]->visit(*this);
                _ctl_query = std::make_shared<AGCondition>(_ctl_query);
                break;
            case Path::X:
                (*child)[0]->visit(*this);
                _ctl_query = std::make_shared<AXCondition>(_ctl_query);
                break;
            case Path::F:
                (*child)[0]->visit(*this);
                _ctl_query = std::make_shared<AFCondition>(_ctl_query);
                break;
            case Path::U: {
                (*child)[0]->visit(*this);
                auto first = _ctl_query;
                (*child)[1]->visit(*this);
                _ctl_query = std::make_shared<AUCondition>(first, _ctl_query);
                break;
            }
            case Path::pError:
                assert(false);
                _ctl_query = nullptr;
                break;
        }
    }

    void AsCTL::_accept(const ECondition *condition) {
        auto child = dynamic_cast<QuantifierCondition*>((*condition)[0].get());
        switch (child->getPath()) {
            case Path::G:
                (*child)[0]->visit(*this);
                _ctl_query = std::make_shared<EGCondition>(_ctl_query);
                break;
            case Path::X:
                (*child)[0]->visit(*this);
                _ctl_query = std::make_shared<EXCondition>(_ctl_query);
                break;
            case Path::F:
                (*child)[0]->visit(*this);
                _ctl_query = std::make_shared<EFCondition>(_ctl_query);
                break;
            case Path::U: {
                (*child)[0]->visit(*this);
                auto first = _ctl_query;
                (*child)[1]->visit(*this);
                _ctl_query = std::make_shared<EUCondition>(first, _ctl_query);
                break;
            }
            case Path::pError:
                assert(false);
                _ctl_query = nullptr;
                break;
        }
    }

    void AsCTL::_accept(const GCondition *condition) {
        std::cerr << "Direct call to path quantifier in AsCTL GCondition" << std::endl;
        assert(false);
        _ctl_query = nullptr;
    }

    void AsCTL::_accept(const FCondition *condition) {
        std::cerr << "Direct call to path quantifier in AsCTL FCondition" << std::endl;
        assert(false);
        _ctl_query = nullptr;
    }

    void AsCTL::_accept(const XCondition *condition) {
        std::cerr << "Direct call to path quantifier in AsCTL XCondition" << std::endl;
        assert(false);
        _ctl_query = nullptr;
    }

    void AsCTL::_accept(const UntilCondition *condition) {
        std::cerr << "Direct call to path quantifier in AsCTL UntilCondition" << std::endl;
        assert(false);
        _ctl_query = nullptr;
    }

    void AsCTL::_accept(const UnfoldedFireableCondition *element) {
        _ctl_query = std::make_shared<UnfoldedFireableCondition>(*element);
    }

    template<typename T>
    Condition_ptr copy_condition(const T* el)
    {
        return std::make_shared<T>(*el);
    }

    void AsCTL::_accept(const FireableCondition *element) {
        _ctl_query = copy_condition(element);
    }

    void AsCTL::_accept(const UpperBoundsCondition *element) {
        _ctl_query = copy_condition(element);
    }

    void AsCTL::_accept(const LivenessCondition *element) {
        _ctl_query = copy_condition(element);
    }

    void AsCTL::_accept(const KSafeCondition *element) {
        _ctl_query = copy_condition(element);
    }

    void AsCTL::_accept(const QuasiLivenessCondition *element) {
        _ctl_query = copy_condition(element);
    }

    void AsCTL::_accept(const StableMarkingCondition *element) {
        _ctl_query = copy_condition(element);
    }

    void AsCTL::_accept(const BooleanCondition *element) {
        _ctl_query = element->value ? BooleanCondition::TRUE_CONSTANT : BooleanCondition::FALSE_CONSTANT;
    }

    template<typename T>
    Expr_ptr AsCTL::copy_narry_expr(const T* el)
    {
        assert(false);
        // TODO: fix
        return nullptr;
    }

    void AsCTL::_accept(const PlusExpr *element) {
        _expression = copy_narry_expr(element);
    }

    void AsCTL::_accept(const MultiplyExpr *element) {
        _expression = copy_narry_expr(element);
    }

    void AsCTL::_accept(const SubtractExpr *element) {
        _expression = copy_narry_expr(element);
    }

    void AsCTL::_accept(const MinusExpr *element) {
        (*element)[0]->visit(*this);
        _expression = std::make_shared<MinusExpr>(_expression);
    }

    void AsCTL::_accept(const LiteralExpr *element) {
        _expression = std::make_shared<LiteralExpr>(element->value());
    }

    void AsCTL::_accept(const IdentifierExpr *element) {
        _expression = std::make_shared<IdentifierExpr>(*element);
    }

    void AsCTL::_accept(const UnfoldedIdentifierExpr *element) {
        _expression = std::make_shared<UnfoldedIdentifierExpr>(*element);
    }

}