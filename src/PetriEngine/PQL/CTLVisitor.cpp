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

    void IsCTLVisitor::_accept(const ControlCondition *condition) {
        (*condition)[0]->visit(*this);
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
}