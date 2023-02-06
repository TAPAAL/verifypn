/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
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

/*
 * File:   CExprToString.h
 * Author: Peter G. Jensen
 *
 * Created on 10 February 2022, 16.34
 */

#ifndef CEXPRTOSTRING_H
#define CEXPRTOSTRING_H

#include <string>
#include <ostream>

#include "ColorExpressionVisitor.h"

namespace PetriEngine {
    namespace Colored {

        class CExprToString : public ColorExpressionVisitor {
        public:

            CExprToString(std::ostream& out) : _out(out) {
            }

            virtual void accept(const DotConstantExpression*);

            virtual void accept(const VariableExpression* e);

            virtual void accept(const UserOperatorExpression* e);

            virtual void accept(const SuccessorExpression* e);

            virtual void accept(const PredecessorExpression* e);

            virtual void accept(const TupleExpression* tup);

            virtual void accept(const LessThanExpression* lt);

            virtual void accept(const LessThanEqExpression* lte);

            virtual void accept(const EqualityExpression* eq);

            virtual void accept(const InequalityExpression* neq);

            virtual void accept(const AndExpression* andexpr);

            virtual void accept(const OrExpression* orexpr);

            virtual void accept(const AllExpression* all);

            virtual void accept(const NumberOfExpression* no);

            virtual void accept(const AddExpression* add);

            virtual void accept(const SubtractExpression* sub);

            virtual void accept(const ScalarProductExpression* scalar);

        private:
            std::ostream& _out;
        };

        std::ostream& operator<<(std::ostream& os, const PetriEngine::Colored::Expression& x);
        std::string to_string(const PetriEngine::Colored::Expression& x);
    }
}

#endif /* CEXPRTOSTRING_H */

