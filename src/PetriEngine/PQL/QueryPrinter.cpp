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

#include "PetriEngine/PQL/QueryPrinter.h"

namespace PetriEngine {
    namespace PQL {
        void QueryPrinter::_accept(const NotCondition *element) {
            os << "(not ";
            element->operator[](0)->visit(*this);
            os << ")";
        }


        void QueryPrinter::_accept(const LogicalCondition *element, const std::string &op) {
            os << "(";
            (*element)[0]->visit(*this);
            for (size_t i = 1; i < element->operands(); ++i) {
                os << " " << op << " ";
                (*element)[i]->visit(*this);
            }
            os << ")";
        }

        void QueryPrinter::_accept(const CompareCondition *element, const std::string &op) {
            os << "(";
            element->getExpr1()->visit(*this);
            os << " " << op << " ";
            element->getExpr2()->visit(*this);
            os << ")";
        }

        void QueryPrinter::_accept(const AndCondition *element) {
            _accept(element, "and");
        }

        void QueryPrinter::_accept(const OrCondition *element) {
            _accept(element, "or");
        }

        void QueryPrinter::_accept(const LessThanCondition *element) {
            _accept(element, "<");
        }

        void QueryPrinter::_accept(const LessThanOrEqualCondition *element) {
            _accept(element, "<=");
        }

        void QueryPrinter::_accept(const EqualCondition *element) {
            _accept(element, "==");
        }

        void QueryPrinter::_accept(const NotEqualCondition *element) {
            _accept(element, "!=");
        }

        void QueryPrinter::_accept(const DeadlockCondition *element) {
            os << "deadlock";
        }

        void QueryPrinter::_accept(const CompareConjunction *element) {
            os << "(";
            if (element->isNegated()) os << "not";
            bool first = true;
            for (const auto &cons : *element) {
                if (!first) os << " and ";
                if (cons._lower != 0)
                    os << "(" << cons._lower << " <= " << cons._name << ")";
                if (cons._lower != 0 && cons._upper != std::numeric_limits<uint32_t>::max())
                    os << " and ";
                if (cons._upper != std::numeric_limits<uint32_t>::max())
                    os << "(" << cons._upper << " >= " << cons._name << ")";
                first = false;
            }
            os << ")";
        }

        void QueryPrinter::_accept(const UnfoldedUpperBoundsCondition *element) {
            os << "bounds (";
            auto places = element->places();
            for (size_t i = 0; i < places.size(); ++i) {
                if (i != 0) os << ", ";
                os << places[i]._name;
            }
            os << ")";
        }

        void QueryPrinter::_accept(const EFCondition *condition) {
            os << "EF ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const EGCondition *condition) {
            os << "EG ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const AGCondition *condition) {
            os << "AG ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const AFCondition *condition) {
            os << "AF ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const EXCondition *condition) {
            os << "AF ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const AXCondition *condition) {
            os << "AX ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const EUCondition *condition) {
            os << "E ";
            _accept((UntilCondition*) condition);
        }

        void QueryPrinter::_accept(const AUCondition *condition) {
            os << "A ";
            _accept((UntilCondition*) condition);
        }

        void QueryPrinter::_accept(const ACondition *condition) {
            os << "A ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const ECondition *condition) {
            os << "E ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const GCondition *condition) {
            os << "G ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const FCondition *condition) {
            os << "F ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const XCondition *condition) {
            os << "X ";
            (*condition)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const UntilCondition *condition) {
            os << "(";
            condition->getCond1()->visit(*this);
            os << " U ";
            condition->getCond2()->visit(*this);
            os << ")";
        }

        void QueryPrinter::_accept(const UnfoldedFireableCondition *element) {
            os << "is-fireable(" << element->getName() << ")";
        }

        void QueryPrinter::_accept(const FireableCondition *element) {
            os << "is-fireable(" << element->getName() << ")";

        }

        void QueryPrinter::_accept(const UpperBoundsCondition *element) {
            os << "bounds (";
            auto places = element->getPlaces();
            for(size_t i = 0; i < places.size(); ++i)
            {
                if(i != 0) os << ", ";
                os << places[i];
            }
            os << ")";
        }

        void QueryPrinter::_accept(const LivenessCondition *element) {
            const Condition_ptr& cond = element->getCompiled();
            if (cond) {
                cond->visit(*this);
            }
            else {
                os << "liveness";
            }
        }

        void QueryPrinter::_accept(const KSafeCondition *element) {
            if (element->getCompiled()) {
                element->getCompiled()->visit(*this);
            }
            else {
                os << "k-safe(";
                element->getBound()->visit(*this);
                os << ")";
            }
        }

        void QueryPrinter::_accept(const QuasiLivenessCondition *element) {
            const Condition_ptr& cond = element->getCompiled();
            if (cond) {
                cond->visit(*this);
            }
            else {
                os << "liveness";
            }
        }

        void QueryPrinter::_accept(const StableMarkingCondition *element) {
            const Condition_ptr& cond = element->getCompiled();
            if (cond) {
                cond->visit(*this);
            }
            else {
                os << "stable-marking";
            }
        }

        void QueryPrinter::_accept(const BooleanCondition *element) {
            os << (element->value ? "true" : "false");
        }

        void QueryPrinter::_accept(const UnfoldedIdentifierExpr *element) {
            os << element->name();
        }

        void QueryPrinter::_accept(const LiteralExpr *element) {
            os << element->value();
        }

        void QueryPrinter::_accept(const CommutativeExpr *element, const std::string &op) {
            os << "(" << element->constant();
            for (const auto &id: element->places()) {
                os << " " << op << " " << id.second;
            }
            for (const auto &expr : element->expressions()) {
                os << " " << op << " ";
                expr->visit(*this);
            }
            os << ")";
        }

        void QueryPrinter::_accept(const NaryExpr *element, const std::string &op) {
            os << "(";
            (*element)[0]->visit(*this);
            for(size_t i = 1; i < element->operands(); ++i)
            {
                os << " " << op << " ";
                (*element)[i]->visit(*this);
            }
            os << ")";
        }

        void QueryPrinter::_accept(const PlusExpr *element) {
            _accept((const CommutativeExpr *)element, "+");
        }

        void QueryPrinter::_accept(const MultiplyExpr *element) {
            _accept((const CommutativeExpr*)element, "*");
        }

        void QueryPrinter::_accept(const MinusExpr *element) {
            os << "-";
            (*element)[0]->visit(*this);
        }

        void QueryPrinter::_accept(const SubtractExpr *element) {
            _accept((const NaryExpr *)element, "-");
        }

        void QueryPrinter::_accept(const IdentifierExpr *element) {
            os << element->name();
        }
    }
}