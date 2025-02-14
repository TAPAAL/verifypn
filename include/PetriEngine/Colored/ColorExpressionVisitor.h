/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
 *                     Peter G. Jensen <root@petergjoel.dk>
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
 * File:   ColorExpressionVisitor.h
 * Author: Peter G. Jensen
 *
 * Created on 9 February 2022, 11.55
 */

#ifndef COLOREXPRESSIONVISITOR_H
#define COLOREXPRESSIONVISITOR_H


namespace PetriEngine {

    namespace Colored {
        class Expression;
        class DotConstantExpression;
        class VariableExpression;
        class UserOperatorExpression;
        class SuccessorExpression;
        class PredecessorExpression;
        class TupleExpression;
        class LessThanExpression;
        class LessThanEqExpression;
        class EqualityExpression;
        class InequalityExpression;
        class AndExpression;
        class OrExpression;
        class AllExpression;
        class NumberOfExpression;
        class AddExpression;
        class SubtractExpression;
        class ScalarProductExpression;


        class ColorExpressionVisitor {
        public:
            virtual ~ColorExpressionVisitor() = default;

            virtual void accept(const DotConstantExpression*) = 0;
            virtual void accept(const VariableExpression*) = 0;
            virtual void accept(const UserOperatorExpression*) = 0;
            virtual void accept(const SuccessorExpression*) = 0;
            virtual void accept(const PredecessorExpression*) = 0;
            virtual void accept(const TupleExpression*) = 0;
            virtual void accept(const LessThanExpression*) = 0;
            virtual void accept(const LessThanEqExpression*) = 0;
            virtual void accept(const EqualityExpression*) = 0;
            virtual void accept(const InequalityExpression*) = 0;
            virtual void accept(const AndExpression*) = 0;
            virtual void accept(const OrExpression*) = 0;
            virtual void accept(const AllExpression*) = 0;
            virtual void accept(const NumberOfExpression*) = 0;
            virtual void accept(const AddExpression*) = 0;
            virtual void accept(const SubtractExpression*) = 0;
            virtual void accept(const ScalarProductExpression*) = 0;
        };

    }
}
#endif /* COLOREXPRESSIONVISITOR_H */

