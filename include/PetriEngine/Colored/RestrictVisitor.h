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
 * File:   RestrictVisitor.h
 * Author: Peter G. Jensen
 *
 * Created on 11 February 2022, 11.28
 */

#ifndef RESTRICTVISITOR_H
#define RESTRICTVISITOR_H

#include "Expressions.h"
#include "ConstantVisitor.h"
#include "VariableVisitor.h"


namespace PetriEngine {
    namespace Colored {

        class RestrictVisitor : public ColorExpressionVisitor {
        private:
            std::vector<VariableIntervalMap>& _variableMap;
            std::set<const Colored::Variable*>& _diagonalVars;
        public:

            RestrictVisitor(std::vector<VariableIntervalMap>& variableMap, std::set<const Colored::Variable*> &diagonalVars)
            : _variableMap(variableMap), _diagonalVars(diagonalVars) {
            }

            virtual void accept(const DotConstantExpression*) {
            }

            virtual void accept(const VariableExpression* e) {
            }

            virtual void accept(const UserOperatorExpression* e) {
            }

            virtual void accept(const SuccessorExpression* e) {
            }

            virtual void accept(const PredecessorExpression* e) {
            }

            virtual void accept(const TupleExpression* tup) {
            }

            virtual void accept(const LessThanExpression* e) {
                VariableModifierMap varModifierMapL;
                VariableModifierMap varModifierMapR;
                PositionVariableMap varPositionsL;
                PositionVariableMap varPositionsR;
                std::set<const Variable *> leftVars;
                std::set<const Variable *> rightVars;
                VariableVisitor::get_variables(*(*e)[0], leftVars, varPositionsL, varModifierMapL, false);
                VariableVisitor::get_variables(*(*e)[1], rightVars, varPositionsR, varModifierMapR, false);
                auto constantMapL = ConstantVisitor::get_constants(*(*e)[0]);
                auto constantMapR = ConstantVisitor::get_constants(*(*e)[1]);

                if (leftVars.empty() && rightVars.empty()) {
                    return;
                }
                GuardRestrictor::restrictVars(_variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, constantMapL, constantMapR, _diagonalVars, true, true);
            }

            virtual void accept(const LessThanEqExpression* e) {
                VariableModifierMap varModifierMapL;
                VariableModifierMap varModifierMapR;
                PositionVariableMap varPositionsL;
                PositionVariableMap varPositionsR;
                std::set<const Colored::Variable *> leftVars;
                std::set<const Colored::Variable *> rightVars;
                VariableVisitor::get_variables(*(*e)[0], leftVars, varPositionsL, varModifierMapL, false);
                VariableVisitor::get_variables(*(*e)[1], rightVars, varPositionsR, varModifierMapR, false);
                auto constantMapL = ConstantVisitor::get_constants(*(*e)[0]);
                auto constantMapR = ConstantVisitor::get_constants(*(*e)[1]);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }

                GuardRestrictor::restrictVars(_variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, constantMapL, constantMapR, _diagonalVars, true, false);
            }

            virtual void accept(const EqualityExpression* e) {
                VariableModifierMap varModifierMapL;
                VariableModifierMap varModifierMapR;
                PositionVariableMap varPositionsL;
                PositionVariableMap varPositionsR;
                std::set<const Colored::Variable *> leftVars;
                std::set<const Colored::Variable *> rightVars;
                VariableVisitor::get_variables(*(*e)[0], leftVars, varPositionsL, varModifierMapL, false);
                VariableVisitor::get_variables(*(*e)[1], rightVars, varPositionsR, varModifierMapR, false);
                auto constantMapL = ConstantVisitor::get_constants(*(*e)[0]);
                auto constantMapR = ConstantVisitor::get_constants(*(*e)[1]);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }

                Colored::GuardRestrictor::restrictEquality(_variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, constantMapL, constantMapR, _diagonalVars);
            }

            virtual void accept(const InequalityExpression* e) {
                VariableModifierMap varModifierMapL;
                VariableModifierMap varModifierMapR;
                PositionVariableMap varPositionsL;
                PositionVariableMap varPositionsR;
                std::set<const Colored::Variable *> leftVars;
                std::set<const Colored::Variable *> rightVars;
                VariableVisitor::get_variables(*(*e)[0], leftVars, varPositionsL, varModifierMapL, false);
                VariableVisitor::get_variables(*(*e)[1], rightVars, varPositionsR, varModifierMapR, false);
                auto constantMapL = ConstantVisitor::get_constants(*(*e)[0]);
                auto constantMapR = ConstantVisitor::get_constants(*(*e)[1]);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }
                Colored::GuardRestrictor::restrictInEquality(_variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, constantMapL, constantMapR, _diagonalVars);
            }

            virtual void accept(const AndExpression* e) {
                (*e)[0]->visit(*this);
                (*e)[1]->visit(*this);
            }

            virtual void accept(const OrExpression* e) {
                auto varMapCopy = _variableMap;
                (*e)[0]->visit(*this);
                restrict(*(*e)[1], varMapCopy, _diagonalVars);
                _variableMap.insert(_variableMap.end(), varMapCopy.begin(), varMapCopy.end());
            }

            virtual void accept(const AllExpression* all) {
            }

            virtual void accept(const NumberOfExpression* no) {
            }

            virtual void accept(const AddExpression* add) {
            }

            virtual void accept(const SubtractExpression* sub) {
            }

            virtual void accept(const ScalarProductExpression* scalar) {
            }

            static inline void restrict(const Expression& e, std::vector<VariableIntervalMap>& variableMap) {
                std::set<const Colored::Variable*> diagonalVars;
                restrict(e, variableMap, diagonalVars);
            }

            static inline void restrict(const Expression& e, std::vector<VariableIntervalMap>& variableMap, std::set<const Colored::Variable*>& diagonalVars) {
                RestrictVisitor v(variableMap, diagonalVars);
                e.visit(v);
            }
        };
    }
}

#endif /* RESTRICTVISITOR_H */

