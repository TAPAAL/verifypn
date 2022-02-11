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

#ifndef COLORED_EXPRESSIONS_H
#define COLORED_EXPRESSIONS_H

#include <string>
#include <unordered_map>
#include <set>
#include <stdlib.h>
#include <iostream>
#include <cassert>
#include <memory>
#include <typeinfo>


#include "Colors.h"
#include "Multiset.h"
#include "EquivalenceVec.h"
#include "ArcIntervals.h"
#include "GuardRestrictor.h"
#include "utils/errors.h"
#include "ColorExpressionVisitor.h"
#include "CExprToString.h"

namespace PetriEngine {
    class ColoredPetriNetBuilder;

    namespace Colored {

        class WeightException : public std::exception {
        private:
            std::string _message;
        public:
            explicit WeightException(std::string message) : _message(message) {}

            const char* what() const noexcept override {
                return ("Undefined weight: " + _message).c_str();
            }
        };

        class Expression {
        public:
            Expression() {}

            virtual void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const {}

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts) const {
                uint32_t index = 0;
                getVariables(variables, varPositions, varModifierMap, includeSubtracts, index);
            }

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions) const {
                VariableModifierMap varModifierMap;
                uint32_t index = 0;
                getVariables(variables, varPositions, varModifierMap, false, index);
            }

            void getVariables(std::set<const Colored::Variable*>& variables) const {
                PositionVariableMap varPositions;
                VariableModifierMap varModifierMap;
                uint32_t index = 0;

                getVariables(variables, varPositions, varModifierMap, false, index);
            }

            virtual void visit(ColorExpressionVisitor& visitor) const = 0;

        };

        class ColorExpression : public Expression {
        public:
            ColorExpression() {}
            virtual ~ColorExpression() {}

            virtual void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const = 0;

            virtual bool getArcIntervals(Colored::ArcIntervals& arcIntervals, const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const = 0;


            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const = 0;
        };

        class DotConstantExpression : public ColorExpression {
        public:
            bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const override {
                if (arcIntervals._intervalTupleVec.empty()) {
                    //We can add all place tokens when considering the dot constant as, that must be present
                    arcIntervals._intervalTupleVec.push_back(cfp.constraints);
                }
                return !cfp.constraints.empty();
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                const Color *dotColor = &(*ColorType::dotInstance()->begin());
                constantMap[index] = dotColor;
            }


            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override{
                return ColorType::dotInstance();
            }
        };

        typedef std::shared_ptr<ColorExpression> ColorExpression_ptr;

        class VariableExpression : public ColorExpression {
        private:
            const Variable* _variable;

        public:
            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                variables.insert(_variable);
                varPositions[index] = _variable;
                if(varModifierMap.count(_variable) == 0){
                    std::vector<std::unordered_map<uint32_t, int32_t>> newVec;

                    for(auto pair : varModifierMap){
                        for(uint32_t i = 0; i < pair.second.size()-1; i++){
                            std::unordered_map<uint32_t, int32_t> emptyMap;
                            newVec.push_back(emptyMap);
                        }
                        break;
                    }
                    std::unordered_map<uint32_t, int32_t> newMap;
                    newMap[index] = 0;
                    newVec.push_back(newMap);
                    varModifierMap[_variable] = newVec;
                } else {
                    varModifierMap[_variable].back()[index] = 0;
                }
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const override {
                if (arcIntervals._intervalTupleVec.empty()){
                    //As variables does not restrict the values before the guard we include all tokens
                    arcIntervals._intervalTupleVec.push_back(cfp.constraints);
                }
                return !cfp.constraints.empty();
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
            }


            const Variable* variable() const {
                return _variable;
            }

            VariableExpression(const Variable* variable)
                    : _variable(variable) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override{
                return _variable->colorType;
            }

        };

        class UserOperatorExpression : public ColorExpression {
        private:
            const Color* _userOperator;

        public:
            bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const override {
                uint32_t colorId = _userOperator->getId() + modifier;
                while(colorId < 0){
                    colorId += _userOperator->getColorType()->size();
                }
                colorId = colorId % _userOperator->getColorType()->size();

                if(arcIntervals._intervalTupleVec.empty()){
                    Colored::interval_vector_t newIntervalTuple;
                    bool colorInFixpoint = false;
                    for (const auto& interval : cfp.constraints){
                        if(interval[index].contains(colorId)){
                            newIntervalTuple.addInterval(interval);
                            colorInFixpoint = true;
                        }
                    }
                    arcIntervals._intervalTupleVec.push_back(newIntervalTuple);
                    return colorInFixpoint;
                } else {
                    std::vector<uint32_t> intervalsToRemove;
                    for(auto& intervalTuple : arcIntervals._intervalTupleVec){
                        for (uint32_t i = 0; i < intervalTuple.size(); i++){
                            if(!intervalTuple[i][index].contains(colorId)){
                                intervalsToRemove.push_back(i);
                            }
                        }

                        for (auto i = intervalsToRemove.rbegin(); i != intervalsToRemove.rend(); ++i) {
                            intervalTuple.removeInterval(*i);
                        }
                    }
                    return !arcIntervals._intervalTupleVec[0].empty();
                }
            }

            const Color* user_operator() const {
                return _userOperator;
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                constantMap[index] = _userOperator;
            }

            UserOperatorExpression(const Color* userOperator)
                    : _userOperator(userOperator) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override {
                return _userOperator->getColorType();
            }
        };

        class SuccessorExpression : public ColorExpression {
        private:
            ColorExpression_ptr _color;

        public:
            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                //save index before evaluating nested expression to decrease all the correct modifiers
                uint32_t indexBefore = index;
                _color->getVariables(variables, varPositions, varModifierMap, includeSubtracts, index);
                for(auto& varModifierPair : varModifierMap){
                    for(auto& idModPair : varModifierPair.second.back()){
                        if(idModPair.first <= index && idModPair.first >= indexBefore){
                            idModPair.second--;
                        }
                    }
                }
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const override {
               return _color->getArcIntervals(arcIntervals, cfp, index, modifier+1);
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                _color->getConstants(constantMap, index);
                for(auto& constIndexPair : constantMap){
                    constIndexPair.second = &constIndexPair.second->operator++();
                }
            }

            const ColorExpression_ptr& child() const {
                return _color;
            }

            SuccessorExpression(ColorExpression_ptr&& color)
                    : _color(std::move(color)) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override{
                return _color->getColorType(colorTypes);
            }
        };

        class PredecessorExpression : public ColorExpression {
        private:
            ColorExpression_ptr _color;

        public:
            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                //save index before evaluating nested expression to decrease all the correct modifiers
                uint32_t indexBefore = index;
                _color->getVariables(variables, varPositions, varModifierMap, includeSubtracts, index);
                for(auto& varModifierPair : varModifierMap){
                    for(auto& idModPair : varModifierPair.second.back()){
                        if(idModPair.first <= index && idModPair.first >= indexBefore){
                            idModPair.second++;
                        }
                    }
                }
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const override {
                return _color->getArcIntervals(arcIntervals, cfp, index, modifier-1);
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                _color->getConstants(constantMap, index);
                for(auto& constIndexPair : constantMap){
                    constIndexPair.second = &constIndexPair.second->operator--();
                }
            }

            const ColorExpression_ptr& child() const {
                return _color;
            }

            PredecessorExpression(ColorExpression_ptr&& color)
                    : _color(std::move(color)) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override{
                return _color->getColorType(colorTypes);
            }
        };

        class TupleExpression : public ColorExpression {
        private:
            std::vector<ColorExpression_ptr> _colors;
            const ColorType* _colorType = nullptr;

        public:

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const override {
                for (const auto& expr : _colors) {
                    bool res = expr->getArcIntervals(arcIntervals, cfp, index, modifier);
                    if(!res){
                        return false;
                    }
                    ++index;
                }
                return true;
            }

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                for (const auto& elem : _colors) {
                    elem->getVariables(variables, varPositions, varModifierMap, includeSubtracts, index);
                    ++index;
                }
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                for (const auto& elem : _colors) {
                    elem->getConstants(constantMap, index);
                    index++;
                }
            }

            size_t size() const {
                return _colors.size();
            }

            auto begin() const {
                return _colors.begin();
            }

            auto end() const {
                return _colors.end();
            }

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            TupleExpression(std::vector<ColorExpression_ptr>&& colors, const ColorTypeMap& colorTypes)
                    : _colors(std::move(colors)) {
                _colorType = getColorType(colorTypes);
            }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override{

                std::vector<const ColorType*> types;
                if(_colorType != nullptr){
                    return _colorType;
                }

                for (const auto& color : _colors) {
                    types.push_back(color->getColorType(colorTypes));
                }

                for (auto elem : colorTypes) {
                    auto* pt = dynamic_cast<const ProductType*>(elem.second);
                    if (pt && pt->containsTypes(types)) {
                        return pt;
                    }
                }
                assert(false);
                throw base_error("COULD NOT FIND PRODUCT TYPE");
                return nullptr;
            }
        };

        class GuardExpression : public Expression {
        public:
            virtual ~GuardExpression() {};

            virtual void restrictVars(std::vector<VariableIntervalMap>& variableMap, std::set<const Colored::Variable*> &diagonalVars) const = 0;

            virtual void restrictVars(std::vector<VariableIntervalMap>& variableMap) const {
                std::set<const Colored::Variable*> diagonalVars;
                restrictVars(variableMap, diagonalVars);
            }
        };

        class CompareExpression : public GuardExpression {
        protected:
            ColorExpression_ptr _left;
            ColorExpression_ptr _right;

        public:
            CompareExpression(ColorExpression_ptr&& left, ColorExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
            size_t size() const {
                return 2;
            }

            const ColorExpression_ptr& operator[](size_t i) const {
                return (i == 0) ? _left : _right;
            }
        };

        typedef std::shared_ptr<GuardExpression> GuardExpression_ptr;

        class LessThanExpression : public CompareExpression {
        public:
            using CompareExpression::CompareExpression;

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                _left->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
                _right->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
            }

            void restrictVars(std::vector<VariableIntervalMap>& variableMap, std::set<const Colored::Variable*> &diagonalVars) const override {
                VariableModifierMap varModifierMapL;
                VariableModifierMap varModifierMapR;
                PositionVariableMap varPositionsL;
                PositionVariableMap varPositionsR;
                std::unordered_map<uint32_t, const Color*> constantMapL;
                std::unordered_map<uint32_t, const Color*> constantMapR;
                std::set<const Variable *> leftVars;
                std::set<const Variable *> rightVars;
                uint32_t index = 0;
                _left->getVariables(leftVars, varPositionsL, varModifierMapL, false);
                _right->getVariables(rightVars, varPositionsR, varModifierMapR, false);
                _left->getConstants(constantMapL, index);
                index = 0;
                _right->getConstants(constantMapR, index);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }
                Colored::GuardRestrictor guardRestrictor;
                guardRestrictor.restrictVars(variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, constantMapL, constantMapR, diagonalVars, true, true);
            }

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class LessThanEqExpression : public CompareExpression {
        public:
            using CompareExpression::CompareExpression;

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                _left->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
                _right->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
            }

            void restrictVars(std::vector<VariableIntervalMap>& variableMap, std::set<const Colored::Variable*> &diagonalVars) const override {
                VariableModifierMap varModifierMapL;
                VariableModifierMap varModifierMapR;
                PositionVariableMap varPositionsL;
                PositionVariableMap varPositionsR;
                std::unordered_map<uint32_t, const Color*> constantMapL;
                std::unordered_map<uint32_t, const Color*> constantMapR;
                std::set<const Colored::Variable *> leftVars;
                std::set<const Colored::Variable *> rightVars;
                uint32_t index = 0;
                _left->getVariables(leftVars, varPositionsL, varModifierMapL, false);
                _right->getVariables(rightVars, varPositionsR, varModifierMapR, false);
                _left->getConstants(constantMapL, index);
                index = 0;
                _right->getConstants(constantMapR, index);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }

                Colored::GuardRestrictor guardRestrictor;
                guardRestrictor.restrictVars(variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, constantMapL, constantMapR, diagonalVars, true, false);
            }

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class EqualityExpression : public CompareExpression {
        public:
            using CompareExpression::CompareExpression;

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                _left->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
                _right->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
            }


            void restrictVars(std::vector<VariableIntervalMap>& variableMap, std::set<const Colored::Variable*> &diagonalVars) const override {
                VariableModifierMap varModifierMapL;
                VariableModifierMap varModifierMapR;
                PositionVariableMap varPositionsL;
                PositionVariableMap varPositionsR;
                std::unordered_map<uint32_t, const Color*> constantMapL;
                std::unordered_map<uint32_t, const Color*> constantMapR;
                std::set<const Colored::Variable *> leftVars;
                std::set<const Colored::Variable *> rightVars;
                uint32_t index = 0;
                _left->getVariables(leftVars, varPositionsL, varModifierMapL, false);
                _right->getVariables(rightVars, varPositionsR, varModifierMapR, false);
                _left->getConstants(constantMapL, index);
                index = 0;
                _right->getConstants(constantMapR, index);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }

                Colored::GuardRestrictor guardRestrictor;
                guardRestrictor.restrictEquality(variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, constantMapL, constantMapR, diagonalVars);
            }

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class InequalityExpression : public CompareExpression {
        public:
            using CompareExpression::CompareExpression;

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                _left->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
                _right->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
            }

            void restrictVars(std::vector<VariableIntervalMap>& variableMap, std::set<const Colored::Variable*> &diagonalVars) const override {
                VariableModifierMap varModifierMapL;
                VariableModifierMap varModifierMapR;
                PositionVariableMap varPositionsL;
                PositionVariableMap varPositionsR;
                std::unordered_map<uint32_t, const Color*> constantMapL;
                std::unordered_map<uint32_t, const Color*> constantMapR;
                std::set<const Colored::Variable *> leftVars;
                std::set<const Colored::Variable *> rightVars;
                uint32_t index = 0;
                _left->getVariables(leftVars, varPositionsL, varModifierMapL, false);
                _right->getVariables(rightVars, varPositionsR, varModifierMapR, false);
                _left->getConstants(constantMapL, index);
                index = 0;
                _right->getConstants(constantMapR, index);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }
                Colored::GuardRestrictor guardRestrictor;
                guardRestrictor.restrictInEquality(variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, constantMapL, constantMapR, diagonalVars);
            }

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class LogicalExpression : public GuardExpression {
        protected:
            GuardExpression_ptr _left;
            GuardExpression_ptr _right;

        public:
            LogicalExpression(GuardExpression_ptr&& left, GuardExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
            size_t size() const {
                return 2;
            }

            const GuardExpression_ptr& operator[](size_t i) const {
                return (i == 0) ? _left : _right;
            }

        };


        class AndExpression : public LogicalExpression {
        public:
            using LogicalExpression::LogicalExpression;

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                _left->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
                _right->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
            }

            void restrictVars(std::vector<VariableIntervalMap>& variableMap, std::set<const Colored::Variable*> &diagonalVars) const override {
                _left->restrictVars(variableMap, diagonalVars);
                _right->restrictVars(variableMap, diagonalVars);
            }

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class OrExpression : public LogicalExpression {
        public:
            using LogicalExpression::LogicalExpression;

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                _left->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
                _right->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
            }

            void restrictVars(std::vector<VariableIntervalMap>& variableMap, std::set<const Colored::Variable*> &diagonalVars) const override{
                auto varMapCopy = variableMap;
                _left->restrictVars(variableMap, diagonalVars);
                _right->restrictVars(varMapCopy, diagonalVars);

                variableMap.insert(variableMap.end(), varMapCopy.begin(), varMapCopy.end());
            }

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class ArcExpression : public Expression {
        public:
            ArcExpression() {}
            virtual ~ArcExpression() {}

            virtual void getConstants(PositionColorsMap &constantMap, uint32_t &index) const = 0;

            virtual bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const = 0;

            virtual uint32_t weight() const = 0;
        };

        typedef std::shared_ptr<ArcExpression> ArcExpression_ptr;

        class AllExpression : public Expression {
        private:
            const ColorType* _sort;

        public:
            virtual ~AllExpression() {};

            void getConstants(PositionColorsMap &constantMap, uint32_t &index) const {
                for (size_t i = 0; i < _sort->size(); i++) {
                    constantMap[index].push_back(&(*_sort)[i]);
                }
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const {

                if(arcIntervals._intervalTupleVec.empty()){
                    bool colorsInFixpoint = false;
                    Colored::interval_vector_t newIntervalTuple;
                    if(cfp.constraints.getContainedColors() == _sort->size()){
                        colorsInFixpoint = true;
                        for(const auto& interval : cfp.constraints){
                            newIntervalTuple.addInterval(interval);
                        }
                    }
                    arcIntervals._intervalTupleVec.push_back(newIntervalTuple);
                    return colorsInFixpoint;
                } else {
                    std::vector<Colored::interval_vector_t> newIntervalTupleVec;
                    for(uint32_t i = 0; i < arcIntervals._intervalTupleVec.size(); i++){
                        auto& intervalTuple = arcIntervals._intervalTupleVec[i];
                        if(intervalTuple.getContainedColors() == _sort->size()){
                            newIntervalTupleVec.push_back(intervalTuple);
                        }
                    }
                    arcIntervals._intervalTupleVec = std::move(newIntervalTupleVec);
                    return !arcIntervals._intervalTupleVec.empty();
                }
            }

            size_t size() const {
                return  _sort->size();
            }

            const ColorType* sort() const {
                return _sort;
            }

            AllExpression(const ColorType* sort) : _sort(sort)
            {
                assert(sort != nullptr);
            }

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        typedef std::shared_ptr<AllExpression> AllExpression_ptr;

        class NumberOfExpression : public ArcExpression {
        private:
            uint32_t _number;
            std::vector<ColorExpression_ptr> _color;
            AllExpression_ptr _all;

        public:

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                if (_all != nullptr)
                    return;
                for (const auto& elem : _color) {
                    //TODO: can there be more than one element in a number of expression?
                    elem->getVariables(variables, varPositions, varModifierMap, includeSubtracts, index);
                }
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const override {
                if (_all != nullptr) {
                    return _all->getArcIntervals(arcIntervals, cfp, index, modifier);
                }
                for (const auto& elem : _color) {
                    if(!elem->getArcIntervals(arcIntervals, cfp, index, modifier)){
                        return false;
                    }
                }
                return true;
            }

            void getConstants(PositionColorsMap &constantMap, uint32_t &index) const override {
                if (_all != nullptr)
                    _all->getConstants(constantMap, index);
                else for (const auto& elem : _color) {
                    std::unordered_map<uint32_t, const Color*> elemMap;
                    elem->getConstants(elemMap, index);
                    for(const auto& pair : elemMap){
                        constantMap[pair.first].push_back(pair.second);
                    }
                }
            }

            uint32_t weight() const override {
                if (_all == nullptr)
                    return _number * _color.size();
                else
                    return _number * _all->size();
            }

            bool is_all() const {
                return (bool)_all;
            }

            bool is_single_color() const {
                return !is_all() && _color.size() == 1;
            }

            uint32_t number() const {
                return _number;
            }

            const ColorExpression_ptr& operator[](size_t i ) const {
                return _color[i];
            }

            size_t size() const {
                return _color.size();
            }

            auto begin() const {
                return _color.begin();
            }

            auto end() const {
                return _color.end();
            }

            const AllExpression_ptr& all() const {
                return _all;
            }

            NumberOfExpression(std::vector<ColorExpression_ptr>&& color, uint32_t number = 1)
                    : _number(number), _color(std::move(color)), _all(nullptr) {}
            NumberOfExpression(AllExpression_ptr&& all, uint32_t number = 1)
                    : _number(number), _color(), _all(std::move(all)) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        typedef std::shared_ptr<NumberOfExpression> NumberOfExpression_ptr;

        class AddExpression : public ArcExpression {
        private:
            std::vector<ArcExpression_ptr> _constituents;

        public:


            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                for (const auto& elem : _constituents) {
                    for(auto& pair : varModifierMap){
                        std::unordered_map<uint32_t, int32_t> newMap;
                        pair.second.push_back(newMap);
                    }
                    elem->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
                }
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const override {
                for (const auto& elem : _constituents) {
                    uint32_t newIndex = 0;
                    Colored::ArcIntervals newArcIntervals;
                    std::vector<uint32_t> intervalsForRemoval;
                    std::vector<Colored::interval_t> newIntervals;
                    if(!elem->getArcIntervals(newArcIntervals, cfp, newIndex, modifier)){
                        return false;
                    }

                    if(newArcIntervals._intervalTupleVec.empty()){
                        return false;
                    }

                    arcIntervals._intervalTupleVec.insert(arcIntervals._intervalTupleVec.end(), newArcIntervals._intervalTupleVec.begin(), newArcIntervals._intervalTupleVec.end());
                }
                return true;
            }

            void getConstants(PositionColorsMap &constantMap, uint32_t &index) const override {
                uint32_t indexCopy = index;
                for (const auto& elem : _constituents) {
                    uint32_t localIndex = indexCopy;
                    elem->getConstants(constantMap, localIndex);
                }
            }

            uint32_t weight() const override {
                uint32_t res = 0;
                for (const auto& expr : _constituents) {
                    res += expr->weight();
                }
                return res;
            }

            size_t size() const {
                return _constituents.size();
            }

            const ArcExpression_ptr& operator[](size_t i) const {
                return _constituents[i];
            }

            auto begin() const {
                return _constituents.begin();
            }

            auto end() const {
                return _constituents.end();
            }

            AddExpression(std::vector<ArcExpression_ptr>&& constituents)
                    : _constituents(std::move(constituents)) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class SubtractExpression : public ArcExpression {
        private:
            ArcExpression_ptr _left;
            ArcExpression_ptr _right;

        public:

            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                _left->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
                //We ignore the restrictions imposed by the subtraction for now
                if(includeSubtracts){
                    _right->getVariables(variables, varPositions, varModifierMap, includeSubtracts);
                }
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const override {
                return _left->getArcIntervals(arcIntervals, cfp, index, modifier);
                //We ignore the restrictions imposed by the subtraction for now
                //_right->getArcIntervals(arcIntervals, cfp, &rightIndex);
            }

            void getConstants(PositionColorsMap &constantMap, uint32_t &index) const override {
                uint32_t rIndex = index;
                _left->getConstants(constantMap, index);
                _right->getConstants(constantMap, rIndex);
            }

            uint32_t weight() const override {
                auto* left = dynamic_cast<NumberOfExpression*>(_left.get());
                if (!left || !left->is_all()) {
                    throw WeightException("Left constituent of subtract is not an all expression!");
                }
                auto* right = dynamic_cast<NumberOfExpression*>(_right.get());
                if (!right || !right->is_single_color()) {
                    throw WeightException("Right constituent of subtract is not a single color number of expression!");
                }

                uint32_t val = std::min(left->number(), right->number());
                return _left->weight() - val;
            }

            size_t size() const {
                return 2;
            }

            const ArcExpression_ptr& operator[](size_t i ) const {
                if(i == 0) return _left;
                else return _right;
            }


            SubtractExpression(ArcExpression_ptr&& left, ArcExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class ScalarProductExpression : public ArcExpression {
        private:
            uint32_t _scalar;
            ArcExpression_ptr _expr;

        public:
            void getVariables(std::set<const Colored::Variable*>& variables, PositionVariableMap& varPositions, VariableModifierMap& varModifierMap, bool includeSubtracts, uint32_t& index) const override {
                _expr->getVariables(variables, varPositions,varModifierMap, includeSubtracts);
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp, uint32_t& index, int32_t modifier) const override {
               return _expr ->getArcIntervals(arcIntervals, cfp, index, modifier);
            }

            void getConstants(PositionColorsMap &constantMap, uint32_t &index) const override {
                _expr->getConstants(constantMap, index);
            }

            uint32_t weight() const override {
                return _scalar * _expr->weight();
            }

            auto scalar() const {
                return _scalar;
            }

            const ArcExpression_ptr& child() const {
                return _expr;
            }

            ScalarProductExpression(ArcExpression_ptr&& expr, uint32_t scalar)
                    : _scalar(std::move(scalar)), _expr(expr) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };
    }
}

#endif /* COLORED_EXPRESSIONS_H */

