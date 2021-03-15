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


#include "Colors.h"
#include "Multiset.h"
#include "ArcIntervals.h"
#include "GuardRestrictor.h"
#include "../errorcodes.h"

namespace PetriEngine {
    class ColoredPetriNetBuilder;
    
    namespace Colored {
        struct ExpressionContext {
            typedef std::unordered_map<const Colored::Variable *, const PetriEngine::Colored::Color *> BindingMap;

            BindingMap& binding;
            std::unordered_map<std::string, ColorType*>& colorTypes;
            
            const Color* findColor(const std::string& color) const {
                if (color.compare("dot") == 0)
                    return DotConstant::dotConstant(nullptr);
                for (auto& elem : colorTypes) {
                    auto col = (*elem.second)[color];
                    if (col)
                        return col;
                }
                printf("Could not find color: %s\nCANNOT_COMPUTE\n", color.c_str());
                exit(ErrorCode);
            }

            ProductType* findProductColorType(const std::vector<const ColorType*>& types) const {
                for (auto& elem : colorTypes) {
                    auto* pt = dynamic_cast<ProductType*>(elem.second);
                    if (pt && pt->containsTypes(types)) {
                        return pt;
                    }
                }
                return nullptr;
            }
        };

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
        
            virtual void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const {}

            virtual void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap) const {
                uint32_t index = 0;
                getVariables(variables, varPositions, varModifierMap, &index);
            }

            virtual void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions) const {
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMap;
                uint32_t index = 0;
                getVariables(variables, varPositions, varModifierMap, &index);
            }

            virtual bool isTuple() const {
                return false;
            }

            virtual void getVariables(std::set<const Colored::Variable*>& variables) const {
                std::unordered_map<uint32_t, const Colored::Variable *> varPositions;
                std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMap;

                getVariables(variables, varPositions, varModifierMap);
            }
            
            virtual void expressionType() {
                std::cout << "Expression" << std::endl;
            }

            virtual std::string toString() const {
                return "Unsupported";
            }
        };
        
        class ColorExpression : public Expression {
        public:
            ColorExpression() {}
            virtual ~ColorExpression() {}
            
            virtual const Color* eval(ExpressionContext& context) const = 0;

            virtual void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const = 0;

            virtual bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const = 0;

            virtual ColorType* getColorType(std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const = 0;

            virtual Colored::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const {
                return Colored::intervalTuple_t();
            }

        };
        
        class DotConstantExpression : public ColorExpression {
        public:
            const Color* eval(ExpressionContext& context) const override {
                return DotConstant::dotConstant(nullptr);
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
                if (arcIntervals._intervalTupleVec.empty()) {
                    //We can add all place tokens when considering the dot constant as, that must be present
                    arcIntervals._intervalTupleVec.push_back(cfp.constraints);
                }
                return !cfp.constraints._intervals.empty();
            }

            Colored::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                Colored::interval_t interval;
                Colored::intervalTuple_t tupleInterval;
                const Color *dotColor = DotConstant::dotConstant(nullptr);
                 
                colortypes->push_back(dotColor->getColorType());
                
                interval.addRange(dotColor->getId(), dotColor->getId());
                tupleInterval.addInterval(interval);
                return tupleInterval;
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                const Color *dotColor = DotConstant::dotConstant(nullptr);
                constantMap[index] = dotColor;
            }
            ColorType* getColorType(std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                return DotConstant::dotConstant(nullptr)->getColorType();
            }

            std::string toString() const override {
                return "dot";
            }
        };

        typedef std::shared_ptr<ColorExpression> ColorExpression_ptr;
        
        class VariableExpression : public ColorExpression {
        private:
            Variable* _variable;
            
        public:
            const Color* eval(ExpressionContext& context) const override {
                return context.binding[_variable];
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                variables.insert(_variable);
                varPositions[*index] = _variable;
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
                    newMap[*index] = 0;
                    newVec.push_back(newMap);
                    varModifierMap[_variable] = newVec;
                } else {
                    varModifierMap[_variable].back()[*index] = 0;
                }                
            }

            Colored::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                Colored::intervalTuple_t varInterval;
                
                // If we see a new variable on an out arc, it gets its full interval
                if (varMap.count(_variable) == 0){
                    Colored::intervalTuple_t intervalTuple;
                    intervalTuple.addInterval(_variable->colorType->getFullInterval());
                    varMap[_variable] = intervalTuple;
                }

                for(auto interval : varMap[_variable]._intervals){
                    varInterval.addInterval(interval);
                }
                                           
                std::vector<ColorType*> varColorTypes;
                _variable->colorType->getColortypes(varColorTypes);

                for(auto ct : varColorTypes){
                    colortypes->push_back(ct);
                }

                return varInterval;
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
                if (arcIntervals._intervalTupleVec.empty()){
                    //As variables does not restrict the values before the guard we include all tokens
                    arcIntervals._intervalTupleVec.push_back(cfp.constraints);
                }
                return !cfp.constraints._intervals.empty();
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
            }

            std::string toString() const override {
                return _variable->name;
            }

            ColorType* getColorType(std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                return _variable->colorType;
            }

            VariableExpression(Variable* variable)
                    : _variable(variable) {}
        };
        
        class UserOperatorExpression : public ColorExpression {
        private:
            const Color* _userOperator;
            
        public:
            const Color* eval(ExpressionContext& context) const override {
                return _userOperator;
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
                uint32_t colorId = _userOperator->getId() + modifier;
                while(colorId < 0){
                    colorId += _userOperator->getColorType()->size();
                }
                colorId = colorId % _userOperator->getColorType()->size();
                
                if(arcIntervals._intervalTupleVec.empty()){
                    Colored::intervalTuple_t newIntervalTuple;
                    bool colorInFixpoint = false;
                    for (auto interval : cfp.constraints._intervals){
                        if(interval[*index].contains(colorId)){
                            newIntervalTuple.addInterval(interval);
                            colorInFixpoint = true;
                        }
                    }
                    arcIntervals._intervalTupleVec.push_back(newIntervalTuple);
                    return colorInFixpoint;
                } else {
                    std::vector<uint32_t> intervalsToRemove;
                    for(auto& intervalTuple : arcIntervals._intervalTupleVec){
                        for (uint32_t i = 0; i < intervalTuple._intervals.size(); i++){
                            if(!intervalTuple[i][*index].contains(colorId)){
                                intervalsToRemove.push_back(i);
                            }
                        }

                        for (auto i = intervalsToRemove.rbegin(); i != intervalsToRemove.rend(); ++i) {
                            intervalTuple.removeInterval(*i);
                        }
                    }              
                    return !arcIntervals._intervalTupleVec[0]._intervals.empty();
                }
            }

            std::string toString() const override {
                return _userOperator->toString();
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                constantMap[index] = _userOperator;
            }
            ColorType* getColorType(std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                return _userOperator->getColorType();
            }

            Colored::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                Colored::interval_t interval;
                Colored::intervalTuple_t tupleInterval;
                 
                colortypes->push_back(_userOperator->getColorType());
                
                interval.addRange(_userOperator->getId(), _userOperator->getId());
                tupleInterval.addInterval(interval);
                return tupleInterval;
            }

            UserOperatorExpression(const Color* userOperator)
                    : _userOperator(userOperator) {}
        };
        
        class UserSortExpression : public Expression {
        private:
            ColorType* _userSort;
            
        public:
            ColorType* eval(ExpressionContext& context) const {
                return _userSort;
            }

            std::string toString() const override {
                return _userSort->getName();
            }

            UserSortExpression(ColorType* userSort)
                    : _userSort(userSort) {}
        };

        typedef std::shared_ptr<UserSortExpression> UserSortExpression_ptr;
        
        class NumberConstantExpression : public Expression {
        private:
            uint32_t _number;
            
        public:
            uint32_t eval(ExpressionContext& context) const {
                return _number;
            }
            
            NumberConstantExpression(uint32_t number)
                    : _number(number) {}
        };

        typedef std::shared_ptr<NumberConstantExpression> NumberConstantExpression_ptr;
        
        class SuccessorExpression : public ColorExpression {
        private:
            ColorExpression_ptr _color;
            
        public:
            const Color* eval(ExpressionContext& context) const override {
                return &++(*_color->eval(context));
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                //save index before evaluating nested expression to decrease all the correct modifiers
                uint32_t indexBefore = *index;
                _color->getVariables(variables, varPositions, varModifierMap, index);
                for(auto& varModifierPair : varModifierMap){
                    for(auto& idModPair : varModifierPair.second.back()){
                        if(idModPair.first <= *index && idModPair.first >= indexBefore){
                            idModPair.second--;
                        } 
                    }                   
                }                
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
               return _color->getArcIntervals(arcIntervals, cfp, index, modifier+1);
            }

            Colored::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                //store the number of colortyps already in colortypes vector and use that as offset when indexing it
                auto colortypesBefore = colortypes->size();

                auto nestedInterval = _color->getOutputIntervals(varMap, colortypes);
                Colored::GuardRestrictor guardRestrictor;
                return guardRestrictor.shiftIntervals(colortypes, &nestedInterval, 1, colortypesBefore);
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                _color->getConstants(constantMap, index);
                for(auto& constIndexPair : constantMap){
                    constIndexPair.second = &constIndexPair.second->operator++();
                }
            }

            std::string toString() const override {
                return _color->toString() + "++";
            }

            ColorType* getColorType(std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                return _color->getColorType(colorTypes);
            }

            SuccessorExpression(ColorExpression_ptr&& color)
                    : _color(std::move(color)) {}
        };
        
        class PredecessorExpression : public ColorExpression {
        private:
            ColorExpression_ptr _color;
            
        public:
            const Color* eval(ExpressionContext& context) const override {
                return &--(*_color->eval(context));
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                //save index before evaluating nested expression to decrease all the correct modifiers
                uint32_t indexBefore = *index;
                _color->getVariables(variables, varPositions, varModifierMap, index);
                for(auto& varModifierPair : varModifierMap){
                    for(auto& idModPair : varModifierPair.second.back()){
                        if(idModPair.first <= *index && idModPair.first >= indexBefore){
                            idModPair.second++;
                        } 
                    }                   
                } 
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
                return _color->getArcIntervals(arcIntervals, cfp, index, modifier-1);
            }
            
            Colored::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                //store the number of colortyps already in colortypes vector and use that as offset when indexing it
                auto colortypesBefore = colortypes->size();

                auto nestedInterval = _color->getOutputIntervals(varMap, colortypes);
                Colored::GuardRestrictor guardRestrictor;
                return guardRestrictor.shiftIntervals(colortypes, &nestedInterval, -1, colortypesBefore);
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                _color->getConstants(constantMap, index);
                for(auto& constIndexPair : constantMap){
                    constIndexPair.second = &constIndexPair.second->operator--();
                }
            }

            std::string toString() const override {
                return _color->toString() + "--";
            }

            ColorType* getColorType(std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                return _color->getColorType(colorTypes);
            }

            PredecessorExpression(ColorExpression_ptr&& color)
                    : _color(std::move(color)) {}
        };
        
        class TupleExpression : public ColorExpression {
        private:
            std::vector<ColorExpression_ptr> _colors;
            ColorType* _colorType;
            
        public:
            const Color* eval(ExpressionContext& context) const override {
                std::vector<const Color*> colors;
                std::vector<const ColorType*> types;
                for (auto color : _colors) {
                    colors.push_back(color->eval(context));
                    types.push_back(colors.back()->getColorType());
                }
                ProductType* pt = context.findProductColorType(types);

                const Color* col = pt->getColor(colors);
                assert(col != nullptr);
                return col;
            }

            bool isTuple() const override {
                return true;
            }

            Colored::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                Colored::intervalTuple_t intervals;
                Colored::intervalTuple_t intervalHolder;                

                for(auto colorExp : _colors) {
                    Colored::intervalTuple_t intervalHolder;
                    auto nested_intervals = colorExp->getOutputIntervals(varMap, colortypes);

                    if(intervals._intervals.empty()){
                        intervals = nested_intervals;
                    } else {                        
                        for(auto nested_interval : nested_intervals._intervals){
                            Colored::intervalTuple_t newIntervals;
                            for(auto interval : intervals._intervals){
                                for(auto nestedRange : nested_interval._ranges) {
                                    interval.addRange(nestedRange);    
                                }
                                newIntervals.addInterval(interval);
                            }
                            for(auto newInterval : newIntervals._intervals){
                                intervalHolder.addInterval(newInterval);
                            }
                        }
                        intervals = intervalHolder;
                    }                  
                }
                return intervals;
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
                for (auto expr : _colors) {
                    bool res = expr->getArcIntervals(arcIntervals, cfp, index, modifier);
                    if(!res){
                        return false;
                    }
                    (*index)++;
                }
                return true;
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                for (auto elem : _colors) {
                    elem->getVariables(variables, varPositions, varModifierMap, index);
                    (*index)++;
                }
            }

            ColorType* getColorType(std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                std::vector<const ColorType*> types;
                for (auto color : _colors) {
                    types.push_back(color->getColorType(colorTypes));
                }
                for (auto& elem : colorTypes) {
                    auto* pt = dynamic_cast<ProductType*>(elem.second);
                    if (pt && pt->containsTypes(types)) {
                        return pt;
                    }
                }
                std::cout << "COULD NOT FIND PRODUCT TYPE" << std::endl;
                return nullptr;
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                for (auto elem : _colors) {
                    elem->getConstants(constantMap, index);
                    index++;
                }
            }

            std::string toString() const override {
                std::string res = "(" + _colors[0]->toString();
                for (uint32_t i = 1; i < _colors.size(); ++i) {
                    res += "," + _colors[i]->toString();
                }
                res += ")";
                return res;
            }

            void setColorType(ColorType* ct){
                _colorType = ct;
            }

            TupleExpression(std::vector<ColorExpression_ptr>&& colors)
                    : _colors(std::move(colors)) {}
        };
        
        class GuardExpression : public Expression {
        public:
            GuardExpression() {}
            virtual ~GuardExpression() {}
            
            virtual bool eval(ExpressionContext& context) const = 0;

            virtual void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap) const = 0;
        };

        typedef std::shared_ptr<GuardExpression> GuardExpression_ptr;
        
        class LessThanExpression : public GuardExpression {
        private:
            ColorExpression_ptr _left;
            ColorExpression_ptr _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) < _right->eval(context);
            }

            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap) const override {
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapL;
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapR;
                std::unordered_map<uint32_t, const Colored::Variable *> varPositionsL;
                std::unordered_map<uint32_t, const Colored::Variable *> varPositionsR;
                std::unordered_map<uint32_t, const Color*> constantMapL;
                std::unordered_map<uint32_t, const Color*> constantMapR;
                std::set<const Variable *> leftVars;
                std::set<const Variable *> rightVars;
                uint32_t index = 0;
                _left->getVariables(leftVars, varPositionsL, varModifierMapL);
                _right->getVariables(rightVars, varPositionsR, varModifierMapR);
                _left->getConstants(constantMapL, index);
                index = 0;
                _right->getConstants(constantMapR, index);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }
                Colored::GuardRestrictor guardRestrictor;
                guardRestrictor.restrictDiagonal(&variableMap, &varModifierMapL, &varModifierMapR, &varPositionsL, &varPositionsR, &constantMapL, &constantMapR, true, true);
            }
            
            std::string toString() const override {
                std::string res = _left->toString() + " < " + _right->toString();
                return res;
            }

            
            LessThanExpression(ColorExpression_ptr&& left, ColorExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
        };
        
        class GreaterThanExpression : public GuardExpression {
        private:
            ColorExpression_ptr _left;
            ColorExpression_ptr _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) > _right->eval(context);
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap) const override {
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapL;
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapR;
                std::unordered_map<uint32_t, const Colored::Variable *> varPositionsL;
                std::unordered_map<uint32_t, const Colored::Variable *> varPositionsR;
                std::unordered_map<uint32_t, const Color*> constantMapL;
                std::unordered_map<uint32_t, const Color*> constantMapR;
                std::set<const Colored::Variable *> leftVars;
                std::set<const Colored::Variable *> rightVars;
                uint32_t index = 0;
                _left->getVariables(leftVars, varPositionsL, varModifierMapL);
                _right->getVariables(rightVars, varPositionsR, varModifierMapR);
                _left->getConstants(constantMapL, index);
                index = 0;
                _right->getConstants(constantMapR, index);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }

                Colored::GuardRestrictor guardRestrictor;
                guardRestrictor.restrictDiagonal(&variableMap, &varModifierMapL, &varModifierMapR, &varPositionsL, &varPositionsR, &constantMapL, &constantMapR, false, true);
            }

            std::string toString() const override {
                std::string res = _left->toString() + " > " + _right->toString();
                return res;
            }
            
            GreaterThanExpression(ColorExpression_ptr&& left, ColorExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
        };
        
        class LessThanEqExpression : public GuardExpression {
        private:
            ColorExpression_ptr _left;
            ColorExpression_ptr _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) <= _right->eval(context);
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap) const override {
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapL;
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapR;
                std::unordered_map<uint32_t, const Colored::Variable *> varPositionsL;
                std::unordered_map<uint32_t, const Colored::Variable *> varPositionsR;
                std::unordered_map<uint32_t, const Color*> constantMapL;
                std::unordered_map<uint32_t, const Color*> constantMapR;
                std::set<const Colored::Variable *> leftVars;
                std::set<const Colored::Variable *> rightVars;
                uint32_t index = 0;
                _left->getVariables(leftVars, varPositionsL, varModifierMapL);
                _right->getVariables(rightVars, varPositionsR, varModifierMapR);
                _left->getConstants(constantMapL, index);
                index = 0;
                _right->getConstants(constantMapR, index);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }

                Colored::GuardRestrictor guardRestrictor;
                guardRestrictor.restrictDiagonal(&variableMap, &varModifierMapL, &varModifierMapR, &varPositionsL, &varPositionsR, &constantMapL, &constantMapR, true, false); 
            }

            std::string toString() const override {
                std::string res = _left->toString() + " <= " + _right->toString();
                return res;
            }

            
            LessThanEqExpression(ColorExpression_ptr&& left, ColorExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
        };
        
        class GreaterThanEqExpression : public GuardExpression {
        private:
            ColorExpression_ptr _left;
            ColorExpression_ptr _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) >= _right->eval(context);
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap) const override {
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapL;
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapR;
                std::unordered_map<uint32_t, const Colored::Variable *> varPositionsL;
                std::unordered_map<uint32_t, const Colored::Variable *> varPositionsR;
                std::unordered_map<uint32_t, const Color*> constantMapL;
                std::unordered_map<uint32_t, const Color*> constantMapR;
                std::set<const Colored::Variable *> leftVars;
                std::set<const Colored::Variable *> rightVars;
                uint32_t index = 0;
                _left->getVariables(leftVars, varPositionsL, varModifierMapL);
                _right->getVariables(rightVars, varPositionsR, varModifierMapR);
                _left->getConstants(constantMapL, index);
                index = 0;
                _right->getConstants(constantMapR, index);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }

                Colored::GuardRestrictor guardRestrictor;
                guardRestrictor.restrictDiagonal(&variableMap, &varModifierMapL, &varModifierMapR, &varPositionsL, &varPositionsR, &constantMapL, &constantMapR, false, false);                
            }
            
            std::string toString() const override {
                std::string res = _left->toString() + " >= " + _right->toString();
                return res;
            }

            GreaterThanEqExpression(ColorExpression_ptr&& left, ColorExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
        };
        
        class EqualityExpression : public GuardExpression {
        private:
            ColorExpression_ptr _left;
            ColorExpression_ptr _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) == _right->eval(context);
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }
            

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap) const override {
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapL;
                std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapR;
                std::unordered_map<uint32_t, const Colored::Variable *> varPositionsL;
                std::unordered_map<uint32_t, const Colored::Variable *> varPositionsR;
                std::unordered_map<uint32_t, const Color*> constantMapL;
                std::unordered_map<uint32_t, const Color*> constantMapR;
                std::set<const Colored::Variable *> leftVars;
                std::set<const Colored::Variable *> rightVars;
                uint32_t index = 0;
                _left->getVariables(leftVars, varPositionsL, varModifierMapL);
                _right->getVariables(rightVars, varPositionsR, varModifierMapR);
                _left->getConstants(constantMapL, index);
                index = 0;
                _right->getConstants(constantMapR, index);

                if(leftVars.empty() && rightVars.empty()){
                    return;
                }
                
                Colored::GuardRestrictor guardRestrictor;
                guardRestrictor.restrictEquality(&variableMap, &varModifierMapL, &varModifierMapR, &varPositionsL, &varPositionsR, &constantMapL, &constantMapR);
            }

            std::string toString() const override {
                std::string res = _left->toString() + " == " + _right->toString();
                return res;
            }

            EqualityExpression(ColorExpression_ptr&& left, ColorExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
        };
        
        class InequalityExpression : public GuardExpression {
        private:
            ColorExpression_ptr _left;
            ColorExpression_ptr _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) != _right->eval(context);
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap) const override {
                
            }

            std::string toString() const override {
                std::string res = _left->toString() + " != " + _right->toString();
                return res;
            }
            
            InequalityExpression(ColorExpression_ptr&& left, ColorExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
        };
        
        class NotExpression : public GuardExpression {
        private:
            GuardExpression_ptr _expr;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return !_expr->eval(context);
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _expr->getVariables(variables, varPositions, varModifierMap, index);
            }

            std::string toString() const override {
                std::string res = "!" + _expr->toString();
                return res;
            }

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap) const override {
                std::set<const Colored::Variable *> variables;
                _expr->getVariables(variables);
                //TODO: invert the var intervals here instead of using the full intervals

                for(auto var : variables){
                    auto fullInterval = var->colorType->getFullInterval();
                    Colored::intervalTuple_t fullTuple;
                    fullTuple.addInterval(fullInterval);
                    for(auto& varMap : variableMap){
                        varMap[var] = fullTuple;
                    }                    
                }
            }
            
            NotExpression(GuardExpression_ptr&& expr) : _expr(std::move(expr)) {}
        };
        
        class AndExpression : public GuardExpression {
        private:
            GuardExpression_ptr _left;
            GuardExpression_ptr _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) && _right->eval(context);
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap) const override {
                _left->restrictVars(variableMap);
                _right->restrictVars(variableMap);
            }

            std::string toString() const override {
                std::string res = _left->toString() + " && " + _right->toString();
                return res;
            }

            AndExpression(GuardExpression_ptr&& left, GuardExpression_ptr&& right)
                    : _left(left), _right(right) {}
        };
        
        class OrExpression : public GuardExpression {
        private:
            GuardExpression_ptr _left;
            GuardExpression_ptr _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) || _right->eval(context);
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap) const override{
                auto varMapCopy = variableMap;
                _left->restrictVars(variableMap);
                _right->restrictVars(varMapCopy);

                for(uint i = 0; i < variableMap.size(); i++){
                    for(auto& varPair : varMapCopy[i]){
                        for(auto& interval : varPair.second._intervals){
                            variableMap[i][varPair.first].addInterval(std::move(interval));
                        }
                    }
                }
            }

            std::string toString() const override {
                std::string res = _left->toString() + " || " + _right->toString();
                return res;
            }

            OrExpression(GuardExpression_ptr&& left, GuardExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
        };
        
        class ArcExpression : public Expression {
        public:
            ArcExpression() {}
            virtual ~ArcExpression() {}
            
            virtual Multiset eval(ExpressionContext& context) const = 0;

            virtual void expressionType() override {
                std::cout << "ArcExpression" << std::endl;
            }
            virtual void getConstants(std::unordered_map<uint32_t, std::vector<const Color*>> &constantMap, uint32_t &index) const = 0;

            virtual bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const = 0;

            virtual uint32_t weight() const = 0;

            virtual Colored::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>>& varMapVec) const {
                std::vector<const Colored::ColorType *> colortypes;
                
                return getOutputIntervals(varMapVec, &colortypes);
            }

            virtual Colored::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const {
                return Colored::intervalTuple_t();
            }
        };

        typedef std::shared_ptr<ArcExpression> ArcExpression_ptr;
        
        class AllExpression : public Expression {
        private:
            ColorType* _sort;
            
        public:
            virtual ~AllExpression() {};
            std::vector<const Color*> eval(ExpressionContext& context) const {
                std::vector<const Color*> colors;
                assert(_sort != nullptr);
                for (size_t i = 0; i < _sort->size(); i++) {
                    colors.push_back(&(*_sort)[i]);
                }
                return colors;
            }

            void getConstants(std::unordered_map<uint32_t, std::vector<const Color*>> &constantMap, uint32_t &index) const {
                for (size_t i = 0; i < _sort->size(); i++) {
                    constantMap[index].push_back(&(*_sort)[i]);
                }
            }

            Colored::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const {
                Colored::intervalTuple_t newIntervalTuple;
                newIntervalTuple.addInterval(_sort->getFullInterval());
                return newIntervalTuple;
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const {
                
                if(arcIntervals._intervalTupleVec.empty()){
                    bool colorsInFixpoint = false;
                    Colored::intervalTuple_t newIntervalTuple;
                    cfp.constraints.simplify();
                    if(cfp.constraints.getContainedColors() == _sort->size()){
                        colorsInFixpoint = true;
                        for(auto interval : cfp.constraints._intervals){                            
                            newIntervalTuple.addInterval(interval);
                        }
                    }                    
                    arcIntervals._intervalTupleVec.push_back(newIntervalTuple);
                    return colorsInFixpoint;
                } else {
                    std::vector<Colored::intervalTuple_t> newIntervalTupleVec;
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

            std::string toString() const override {
                return _sort->getName() + ".all";
            }

            AllExpression(ColorType* sort) : _sort(sort) 
            {
                assert(sort != nullptr);
            }
        };

        typedef std::shared_ptr<AllExpression> AllExpression_ptr;
        
        class NumberOfExpression : public ArcExpression {
        private:
            uint32_t _number;
            std::vector<ColorExpression_ptr> _color;
            AllExpression_ptr _all;
            
        public:
            Multiset eval(ExpressionContext& context) const override {
                std::vector<const Color*> colors;
                if (!_color.empty()) {
                    for (auto elem : _color) {
                        colors.push_back(elem->eval(context));
                    }
                } else if (_all != nullptr) {
                    colors = _all->eval(context);
                }
                std::vector<std::pair<const Color*,uint32_t>> col;
                for (auto elem : colors) {
                    col.push_back(std::make_pair(elem, _number));
                }
                return Multiset(col);
            }

            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                if (_all != nullptr)
                    return;
                for (auto elem : _color) {
                    //TODO: can there be more than one element in a number of expression?
                    elem->getVariables(variables, varPositions, varModifierMap, index);
                    //(*index)++;
                }
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
                if (_all != nullptr) {
                    return _all->getArcIntervals(arcIntervals, cfp, index, modifier);
                }
                uint32_t i = 0;
                for (auto elem : _color) {
                    (*index) += i;
                    if(!elem->getArcIntervals(arcIntervals, cfp, index, modifier)){
                        return false;
                    }
                    i++;
                }
                return true;
            }

            Colored::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const override {
                Colored::intervalTuple_t intervals;
                if (_all == nullptr) {
                    for (auto elem : _color) {
                        for(auto& varMap : varMapVec){
                            auto nestedIntervals = elem->getOutputIntervals(varMap, colortypes);

                            if (intervals._intervals.empty()) {
                                intervals = nestedIntervals;
                            } else {
                                for(auto interval : nestedIntervals._intervals) {
                                    intervals.addInterval(interval);
                                }
                            }
                        }                        
                    }
                } else {
                    return _all->getOutputIntervals(varMapVec, colortypes);
                }
                return intervals;
            }

            void getConstants(std::unordered_map<uint32_t, std::vector<const Color*>> &constantMap, uint32_t &index) const override {
                if (_all != nullptr)
                    _all->getConstants(constantMap, index);
                else for (auto elem : _color) {
                    std::unordered_map<uint32_t, const Color*> elemMap;
                    elem->getConstants(elemMap, index);
                    for(auto pair : elemMap){
                        constantMap[pair.first].push_back(pair.second);
                    }
                    index++;//not sure if index should be increased here, but no number expression have multiple elements
                }
            }

            uint32_t weight() const override {
                if (_all == nullptr)
                    return _number * _color.size();
                else
                    return _number * _all->size();
            }

            bool isAll() const {
                return (bool)_all;
            }

            bool isSingleColor() const {
                return !isAll() && _color.size() == 1;
            }

            uint32_t number() const {
                return _number;
            }

            std::string toString() const override {
                if (isAll())
                    return std::to_string(_number) + "'(" + _all->toString() + ")";
                std::string res = std::to_string(_number) + "'(" + _color[0]->toString() + ")";
                for (uint32_t i = 1; i < _color.size(); ++i) {
                    res += " + ";
                    res += std::to_string(_number) + "'(" + _color[i]->toString() + ")";
                }
                return res;
            }

            NumberOfExpression(std::vector<ColorExpression_ptr>&& color, uint32_t number = 1)
                    : _number(number), _color(std::move(color)), _all(nullptr) {}
            NumberOfExpression(AllExpression_ptr&& all, uint32_t number = 1)
                    : _number(number), _color(), _all(std::move(all)) {}
        };

        typedef std::shared_ptr<NumberOfExpression> NumberOfExpression_ptr;
        
        class AddExpression : public ArcExpression {
        private:
            std::vector<ArcExpression_ptr> _constituents;
            
        public:
            Multiset eval(ExpressionContext& context) const override {
                Multiset ms;
                for (auto expr : _constituents) {
                    ms += expr->eval(context);
                }
                return ms;
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                for (auto elem : _constituents) {
                    for(auto& pair : varModifierMap){
                        std::unordered_map<uint32_t, int32_t> newMap;
                        pair.second.push_back(newMap);
                    }
                    elem->getVariables(variables, varPositions, varModifierMap);
                }
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
                for (auto elem : _constituents) {
                    uint32_t newIndex = 0;
                    Colored::ArcIntervals newArcIntervals;
                    std::vector<uint32_t> intervalsForRemoval;
                    std::vector<Colored::interval_t> newIntervals;
                    if(!elem->getArcIntervals(newArcIntervals, cfp, &newIndex, modifier)){
                        return false;
                    }

                    if(newArcIntervals._intervalTupleVec.empty()){
                        return false;
                    }

                    arcIntervals._intervalTupleVec.insert(arcIntervals._intervalTupleVec.end(), newArcIntervals._intervalTupleVec.begin(), newArcIntervals._intervalTupleVec.end());
                }
                return true;
            }            

            Colored::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const override {
                Colored::intervalTuple_t intervals;
                
                for (auto elem : _constituents) {
                    auto nestedIntervals = elem->getOutputIntervals(varMapVec, colortypes);

                    if (intervals._intervals.empty()) {
                        intervals = nestedIntervals;
                    } else {
                        for(auto interval : nestedIntervals._intervals) {
                            intervals.addInterval(interval);
                        }
                    }
                }
                return intervals;
            }

            void getConstants(std::unordered_map<uint32_t, std::vector<const Color*>> &constantMap, uint32_t &index) const override {
                uint32_t indexCopy = index;
                for (auto elem : _constituents) {
                    uint32_t localIndex = indexCopy;
                    elem->getConstants(constantMap, localIndex);
                }
            }

            uint32_t weight() const override {
                uint32_t res = 0;
                for (auto expr : _constituents) {
                    res += expr->weight();
                }
                return res;
            }

            std::string toString() const override {
                std::string res = _constituents[0]->toString();
                for (uint32_t i = 1; i < _constituents.size(); ++i) {
                    res += " + " + _constituents[i]->toString();
                }
                return res;
            }

            AddExpression(std::vector<ArcExpression_ptr>&& constituents)
                    : _constituents(std::move(constituents)) {}
        };
        
        class SubtractExpression : public ArcExpression {
        private:
            ArcExpression_ptr _left;
            ArcExpression_ptr _right;
            
        public:
            Multiset eval(ExpressionContext& context) const override {
                return _left->eval(context) - _right->eval(context);
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                //We ignore the restrictions imposed by the subtraction for now
                //_right->getVariables(variables, varPositions, varModifierMap);
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
                return _left->getArcIntervals(arcIntervals, cfp, index, modifier);
                //We ignore the restrictions imposed by the subtraction for now
                //_right->getArcIntervals(arcIntervals, cfp, &rightIndex);
            }

            Colored::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const override {
                //We could maybe reduce the intervals slightly by checking if the upper or lower bound is being subtracted
                auto leftIntervals = _left->getOutputIntervals(varMapVec, colortypes);

                return leftIntervals;
            }   

            void getConstants(std::unordered_map<uint32_t, std::vector<const Color*>> &constantMap, uint32_t &index) const override {
                uint32_t rIndex = index;
                _left->getConstants(constantMap, index);
                _right->getConstants(constantMap, rIndex);
            }

            uint32_t weight() const override {
                auto* left = dynamic_cast<NumberOfExpression*>(_left.get());
                if (!left || !left->isAll()) {
                    throw WeightException("Left constituent of subtract is not an all expression!");
                }
                auto* right = dynamic_cast<NumberOfExpression*>(_right.get());
                if (!right || !right->isSingleColor()) {
                    throw WeightException("Right constituent of subtract is not a single color number of expression!");
                }

                uint32_t val = std::min(left->number(), right->number());
                return _left->weight() - val;
            }

            std::string toString() const override {
                return _left->toString() + " - " + _right->toString();
            }

            SubtractExpression(ArcExpression_ptr&& left, ArcExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
        };
        
        class ScalarProductExpression : public ArcExpression {
        private:
            uint32_t _scalar;
            ArcExpression_ptr _expr;
            
        public:
            Multiset eval(ExpressionContext& context) const override {
                return _expr->eval(context) * _scalar;
            }
            
            void getVariables(std::set<const Colored::Variable*>& variables, std::unordered_map<uint32_t, const Colored::Variable *>& varPositions, std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _expr->getVariables(variables, varPositions,varModifierMap);
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
               return _expr ->getArcIntervals(arcIntervals, cfp, index, modifier);
            }

            Colored::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const override {
                return _expr->getOutputIntervals(varMapVec, colortypes);
            }

            void getConstants(std::unordered_map<uint32_t, std::vector<const Color*>> &constantMap, uint32_t &index) const override {
                _expr->getConstants(constantMap, index);
            }

            uint32_t weight() const override {
                return _scalar * _expr->weight();
            }

            std::string toString() const override {
                return std::to_string(_scalar) + " * " + _expr->toString();
            }

            ScalarProductExpression(ArcExpression_ptr&& expr, uint32_t scalar)
                    : _scalar(std::move(scalar)), _expr(expr) {}
        };
    }
}

#endif /* COLORED_EXPRESSIONS_H */

