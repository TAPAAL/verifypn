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
 * File:   VariablesVisitor.h
 * Author: Peter G. Jensen
 *
 * Created on 11 February 2022, 11.03
 */

#ifndef VARIABLESVISITOR_H
#define VARIABLESVISITOR_H

#include "Expressions.h"
#include "Colors.h"

namespace PetriEngine {
    namespace Colored {

        class VariableVisitor : public ColorExpressionVisitor {
            uint32_t _index = 0;
            uint32_t _tupleIndex = 0;
            std::set<const Colored::Variable*>& _variables;
            std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>>& _tuples;
            PositionVariableMap& _varPositions;
            VariableModifierMap& _varModifiers;
            const bool _include_subtracts;

        public:
            VariableVisitor(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions,
                    VariableModifierMap& varModifierMap, std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>>& _tuples, bool includeSubtracts)
            : _variables(variables), _varPositions(varPositions), _varModifiers(varModifierMap), _tuples(_tuples), _include_subtracts(includeSubtracts) {

            }


            virtual void accept(const DotConstantExpression* e)
            {
            }

            virtual void accept(const VariableExpression* e)
            {
                _variables.insert(e->variable());
                _varPositions[_index] = e->variable();
                if(_tupleIndex > 0){
                    _tuples[_tupleIndex].emplace_back(e);
                }
                if(_varModifiers.count(e->variable()) == 0){
                    std::vector<std::unordered_map<uint32_t, int32_t>> newVec;

                    for(auto pair : _varModifiers){
                        for(uint32_t i = 0; i < pair.second.size()-1; i++){
                            std::unordered_map<uint32_t, int32_t> emptyMap;
                            newVec.push_back(emptyMap);
                        }
                        break;
                    }
                    std::unordered_map<uint32_t, int32_t> newMap;
                    newMap[_index] = 0;
                    newVec.push_back(newMap);
                    _varModifiers[e->variable()] = newVec;
                } else {
                    _varModifiers[e->variable()].back()[_index] = 0;
                }
            }

            virtual void accept(const UserOperatorExpression* e)
            {
                if(_tupleIndex > 0){
                    _tuples[_tupleIndex].emplace_back(e);
                }
            }

            virtual void accept(const SuccessorExpression* e)
            {
                // Relies on tuples not being nested
                if(_tupleIndex > 0){
                    _tuples[_tupleIndex].emplace_back(e);
                }
                //save index before evaluating nested expression to decrease all the correct modifiers
                auto indexBefore = _index;
                e->child()->visit(*this);
                for(auto& varModifierPair : _varModifiers){
                    for(auto& idModPair : varModifierPair.second.back()){
                        if(idModPair.first <= _index && idModPair.first >= indexBefore){
                            --idModPair.second;
                        }
                    }
                }
            }

            virtual void accept(const PredecessorExpression* e)
            {
                // Relies on tuples not being nested
                if(_tupleIndex > 0){
                    _tuples[_tupleIndex].emplace_back(e);
                }
                //save index before evaluating nested expression to decrease all the correct modifiers
                auto indexBefore = _index;
                e->child()->visit(*this);
                for(auto& varModifierPair : _varModifiers){
                    for(auto& idModPair : varModifierPair.second.back()){
                        if(idModPair.first <= _index && idModPair.first >= indexBefore){
                            ++idModPair.second;
                        }
                    }
                }
            }

            virtual void accept(const TupleExpression* tup)
            {
                _tupleIndex = _tuples.size() + 1;
                for (const auto& elem : *tup) {
                    elem->visit(*this);
                    ++_index;
                }
                _tupleIndex = 0;
            }

            virtual void accept(const LessThanExpression* e)
            {
                for(auto i : {0,1})
                {
                    _index = 0;
                    (*e)[i]->visit(*this);
                }
            }

            virtual void accept(const LessThanEqExpression* e)
            {
                for(auto i : {0,1})
                {
                    _index = 0;
                    (*e)[i]->visit(*this);
                }
            }

            virtual void accept(const EqualityExpression* e)
            {
                for(auto i : {0,1})
                {
                    _index = 0;
                    (*e)[i]->visit(*this);
                }
            }

            virtual void accept(const InequalityExpression* e)
            {
                for(auto i : {0,1})
                {
                    _index = 0;
                    (*e)[i]->visit(*this);
                }
            }

            virtual void accept(const AndExpression* e)
            {
                for(auto i : {0,1})
                {
                    _index = 0;
                    (*e)[i]->visit(*this);
                }
            }

            virtual void accept(const OrExpression* e)
            {
                for(auto i : {0,1})
                {
                    _index = 0;
                    (*e)[i]->visit(*this);
                }
            }

            virtual void accept(const AllExpression* all)
            {
            }

            virtual void accept(const NumberOfExpression* no)
            {
                if(!no->is_all())
                {
                    //TODO: can there be more than one element in a number of expression?
                    for(auto& e : *no)
                    {
                        _index = 0;
                        e->visit(*this);
                    }
                }
            }

            virtual void accept(const AddExpression* add)
            {
                for (const auto& elem : *add) {
                    for(auto& pair : _varModifiers){
                        std::unordered_map<uint32_t, int32_t> newMap;
                        pair.second.push_back(newMap);
                    }
                    _index = 0;
                    elem->visit(*this);
                }
            }

            virtual void accept(const SubtractExpression* sub)
            {
                _index = 0;
                (*sub)[0]->visit(*this);
                //We ignore the restrictions imposed by the subtraction for now
                if(_include_subtracts){
                    _index = 0;
                    (*sub)[1]->visit(*this);
                }
            }

            virtual void accept(const ScalarProductExpression* scalar)
            {
                _index = 0;
                scalar->child()->visit(*this);
            }

            static inline void get_variables(Expression& e, std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>>& tuples, bool includeSubtracts) {
                VariableVisitor v(variables, varPositions, varModifierMap, tuples, includeSubtracts);
                e.visit(v);
            }

            static inline void get_variables(Expression& e, std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts) {
                std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>> tuples;
                get_variables(e, variables, varPositions, varModifierMap, tuples, includeSubtracts);
            }

            static inline void get_variables(Expression& e, std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions) {
                VariableModifierMap varModifierMap;
                std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>> tuples;
                get_variables(e, variables, varPositions, varModifierMap, tuples, false);
            }

            static inline void get_variables(Expression& e, std::set<const Colored::Variable*>& variables) {
                PositionVariableMap varPositions;
                VariableModifierMap varModifierMap;
                std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>> tuples;

                get_variables(e, variables, varPositions, varModifierMap, tuples, false);
            }

            static inline void get_variables(Expression& e, std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>>& tuples) {
                PositionVariableMap varPositions;
                VariableModifierMap varModifierMap;

                get_variables(e, variables, varPositions, varModifierMap, tuples, false);
            }
        };
    }
}

#endif /* VARIABLESVISITOR_H */

