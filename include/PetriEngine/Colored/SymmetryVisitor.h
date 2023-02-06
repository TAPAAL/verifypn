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
 * File:   SymmetryVisitor.h
 * Author: Peter G. Jensen
 *
 * Created on 10 February 2022
 */

#ifndef SYMMETRYVISITOR_H
#define SYMMETRYVISITOR_H

#include "Expressions.h"
#include <vector>

namespace PetriEngine {
    namespace Colored {

        class SymmetryVisitor : public ColorExpressionVisitor {
        private:
            std::vector<uint32_t>& _numbers;
            bool _ok = true;
        public:

            SymmetryVisitor(std::vector<uint32_t>& numbers) : _numbers(numbers) {
            }

            virtual void accept(const DotConstantExpression*)
            {
                _ok = false;
            }

            virtual void accept(const VariableExpression* e)
            {
                _ok = false;
            }

            virtual void accept(const UserOperatorExpression* e)
            {
                _ok = false;
            }

            virtual void accept(const SuccessorExpression* e)
            {
                _ok = false;
            }

            virtual void accept(const PredecessorExpression* e)
            {
                _ok = false;
            }

            virtual void accept(const TupleExpression* tup)
            {
                _ok = false;
            }

            virtual void accept(const LessThanExpression* lt)
            {
                _ok = false;
            }

            virtual void accept(const LessThanEqExpression* lte)
            {
                _ok = false;
            }

            virtual void accept(const EqualityExpression* eq)
            {
                _ok = false;
            }

            virtual void accept(const InequalityExpression* neq)
            {
                _ok = false;
            }

            virtual void accept(const AndExpression* andexpr)
            {
                _ok = false;
            }

            virtual void accept(const OrExpression* orexpr)
            {
                _ok = false;
            }

            virtual void accept(const AllExpression* all)
            {
                _ok = false;
            }

            virtual void accept(const NumberOfExpression* no)
            {
                //Not entirely sure what to do if there is more than one colorExpression, but should probably return false
                if(no->size() > 1){
                    _ok = false;
                }
                else
                {
                    _numbers.emplace_back(no->number());
                    //Maybe we need to check color expression also
                }
            }

            virtual void accept(const AddExpression* add)
            {
                for(const auto& elem : *add){
                    elem->visit(*this);
                    if(!_ok){
                        return;
                    }
                }

                if(_numbers.size() < 2){
                    _ok = false;
                    return;
                }
                //pick a number
                //every number has to be equal
                uint32_t firstNumber = _numbers[0];
                for(uint32_t number : _numbers){
                    if(firstNumber != number){
                        _ok = false;
                        return;
                    }
                }
                _ok = true;
            }

            virtual void accept(const SubtractExpression* sub)
            {
                _ok = false;
            }

            virtual void accept(const ScalarProductExpression* scalar)
            {
                _ok = false;
            }

            static inline std::pair<bool, std::vector<uint32_t>> eligible_for_symmetry(ArcExpression& e) {
                std::vector<uint32_t> numbers;
                SymmetryVisitor v(numbers);
                e.visit(v);
                return std::make_pair(v._ok, std::move(numbers));

            }
        };


    }
}


#endif /* SYMMETRYVISITOR_H */

