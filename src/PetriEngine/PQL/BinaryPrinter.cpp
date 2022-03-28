/* Copyright (C) 2011  Rasmus Tollund <rtollu18@student.aau.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHos ANY WARRANTY; withos even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PetriEngine/PQL/BinaryPrinter.h"

namespace PetriEngine { namespace PQL {
    void BinaryPrinter::_accept(const LiteralExpr *element){
        os.write("l", sizeof(char));
        int temp = element->value();
        os.write(reinterpret_cast<const char*>(&temp), sizeof(int));
    }

    void BinaryPrinter::_accept(const UnfoldedIdentifierExpr *element){
        os.write("i", sizeof(char));
        int temp = element->offset();
        os.write(reinterpret_cast<const char*>(&temp), sizeof(int));
    }

    void BinaryPrinter::_accept(const MinusExpr *element){
        auto e1 = std::make_shared<PQL::LiteralExpr>(0);
        std::vector<Expr_ptr> exprs;
        exprs.push_back(e1);
        exprs.push_back((*element)[0]);
        PQL::SubtractExpr t(std::move(exprs));
        Visitor::visit(this, t);
    }

    void BinaryPrinter::_accept(const SubtractExpr *element){
        os.write("-", sizeof(char));
        uint32_t size = element->expressions().size();
        os.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
        for(auto& e : element->expressions())
            Visitor::visit(this, e);
    }

    void BinaryPrinter::_accept(const CommutativeExpr *element){
        auto sop = element->op();
        os.write(&sop[0], sizeof(char));
        int32_t temp_constant = element->constant();
        os.write(reinterpret_cast<const char*>(&temp_constant), sizeof(int32_t));
        uint32_t size = element->places().size();
        os.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
        size = element->expressions().size();
        os.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
        for(auto& id : element->places())
            os.write(reinterpret_cast<const char*>(&id.first), sizeof(uint32_t));
        for(auto& e : element->expressions())
            Visitor::visit(this, e);
    }

    void BinaryPrinter::_accept(const SimpleQuantifierCondition *condition){
        auto path = condition->getPath();
        auto quant = condition->getQuantifier();
        os.write(reinterpret_cast<const char*>(&path), sizeof(Path));
        os.write(reinterpret_cast<const char*>(&quant), sizeof(Quantifier));
        Visitor::visit(this, condition->getCond());
    }

    void BinaryPrinter::_accept(const UntilCondition *condition){
        auto path = condition->getPath();
        auto quant = condition->getQuantifier();
        os.write(reinterpret_cast<const char*>(&path), sizeof(Path));
        os.write(reinterpret_cast<const char*>(&quant), sizeof(Quantifier));
        Visitor::visit(this, (*condition)[0]);
        Visitor::visit(this, (*condition)[1]);
    }

    void BinaryPrinter::_accept(const LogicalCondition *condition){
        auto path = condition->getPath();
        auto quant = condition->getQuantifier();
        os.write(reinterpret_cast<const char*>(&path), sizeof(Path));
        os.write(reinterpret_cast<const char*>(&quant), sizeof(Quantifier));
        uint32_t size = condition->operands();
        os.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
        for(auto& c : *condition)
            Visitor::visit(this, c);
    }

    void BinaryPrinter::_accept(const CompareConjunction *element){
        auto path = element->getPath();
        auto quant = Quantifier::COMPCONJ;
        os.write(reinterpret_cast<const char*>(&path), sizeof(Path));
        os.write(reinterpret_cast<const char*>(&quant), sizeof(Quantifier));
        bool temp = element->isNegated();
        os.write(reinterpret_cast<const char*>(&temp), sizeof(bool));
        uint32_t size = element->constraints().size();
        os.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
        for(auto& c : element->constraints())
        {
            os.write(reinterpret_cast<const char*>(&c._place), sizeof(int32_t));
            os.write(reinterpret_cast<const char*>(&c._lower), sizeof(uint32_t));
            os.write(reinterpret_cast<const char*>(&c._upper), sizeof(uint32_t));
        }
    }

    void BinaryPrinter::_accept(const CompareCondition *condition){
        auto path = condition->getPath();
        auto quant = condition->getQuantifier();
        os.write(reinterpret_cast<const char*>(&path), sizeof(Path));
        os.write(reinterpret_cast<const char*>(&quant), sizeof(Quantifier));
        std::string sop = condition->op();
        os.write(sop.data(), sop.size());
        os.write("\0", sizeof(char));
        Visitor::visit(this, (*condition)[0]);
        Visitor::visit(this, (*condition)[1]);
    }

    void BinaryPrinter::_accept(const DeadlockCondition *condition){
        auto path = condition->getPath();
        auto quant = Quantifier::DEADLOCK;
        os.write(reinterpret_cast<const char*>(&path), sizeof(Path));
        os.write(reinterpret_cast<const char*>(&quant), sizeof(Quantifier));
    }

    void BinaryPrinter::_accept(const BooleanCondition *condition){
        auto path = condition->getPath();
        auto quant = Quantifier::PN_BOOLEAN;
        os.write(reinterpret_cast<const char*>(&path), sizeof(Path));
        os.write(reinterpret_cast<const char*>(&quant), sizeof(Quantifier));
        os.write(reinterpret_cast<const char*>(&condition->value), sizeof(bool));
    }

    void BinaryPrinter::_accept(const UnfoldedUpperBoundsCondition *condition){
        auto path = condition->getPath();
        auto quant = Quantifier::UPPERBOUNDS;
        os.write(reinterpret_cast<const char*>(&path), sizeof(Path));
        os.write(reinterpret_cast<const char*>(&quant), sizeof(Quantifier));
        uint32_t size = condition->places().size();
        os.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
        double temp_max = condition->getMax();
        os.write(reinterpret_cast<const char*>(&temp_max), sizeof(double));
        double temp_offset = condition->getOffset();
        os.write(reinterpret_cast<const char*>(&temp_offset), sizeof(double));
        for(auto& b : condition->places())
        {
            os.write(reinterpret_cast<const char*>(&b._place), sizeof(uint32_t));
            os.write(reinterpret_cast<const char*>(&b._max), sizeof(double));
        }
    }

    void BinaryPrinter::_accept(const NotCondition *condition){
        auto path = condition->getPath();
        auto quant = condition->getQuantifier();
        os.write(reinterpret_cast<const char*>(&path), sizeof(Path));
        os.write(reinterpret_cast<const char*>(&quant), sizeof(Quantifier));
        Visitor::visit(this, condition->getCond());
    }

    void BinaryPrinter::_accept(const IdentifierExpr *condition) {
        Visitor::visit(this, condition->compiled());
    }

    void BinaryPrinter::_accept(const ShallowCondition *condition) {
        Visitor::visit(this, condition->getCompiled());
    }
} }
