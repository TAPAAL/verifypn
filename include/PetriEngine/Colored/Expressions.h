/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Expressions.h
 * Author: andreas
 *
 * Created on February 19, 2018, 7:00 PM
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
#include "Patterns.h"
#include "Multiset.h"
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
                    return DotConstant::dotConstant();
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

            virtual Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const {
                return Reachability::intervalTuple_t();
            }

        };
        
        class DotConstantExpression : public ColorExpression {
        public:
            const Color* eval(ExpressionContext& context) const override {
                return DotConstant::dotConstant();
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const override {
                if (arcIntervals._intervalTupleVec.empty()) {
                    //We can add all place tokens when considering the dot constant as, that must be present
                    arcIntervals._intervalTupleVec.push_back(cfp.constraints);
                }
                return !cfp.constraints._intervals.empty();
            }

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                Reachability::interval_t interval;
                Reachability::intervalTuple_t tupleInterval;
                const Color *dotColor = DotConstant::dotConstant();
                 
                colortypes->push_back(dotColor->getColorType());
                
                interval.addRange(dotColor->getId(), dotColor->getId());
                tupleInterval.addInterval(interval);
                return tupleInterval;
            }

            void getConstants(std::unordered_map<uint32_t, const Color*> &constantMap, uint32_t &index) const override {
                const Color *dotColor = DotConstant::dotConstant();
                constantMap[index] = dotColor;
            }
            ColorType* getColorType(std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                return DotConstant::dotConstant()->getColorType();
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

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                Reachability::intervalTuple_t varInterval;
                
                if (varMap.count(_variable) == 0){
                    std::cout << "Could not find intervals for: " << _variable->name << std::endl;
                    Reachability::intervalTuple_t intervalTuple;
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
                    Reachability::intervalTuple_t newIntervalTuple;
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

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                Reachability::interval_t interval;
                Reachability::intervalTuple_t tupleInterval;
                 
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
            //TODO: pattern?
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
        //TODO: pattern?
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

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                //store the number of colortyps already in colortypes vector and use that as offset when indexing it
                auto colortypesBefore = colortypes->size();

                auto nestedInterval = _color->getOutputIntervals(varMap, colortypes);
                Reachability::intervalTuple_t newIntervals;

                for(uint32_t i = 0;  i < nestedInterval.size(); i++) {
                    Reachability::interval_t newInterval;
                    std::vector<Reachability::interval_t> tempIntervals;
                    auto interval = &nestedInterval[i];
                    for(uint32_t j = 0; j < interval->_ranges.size(); j++) {
                        auto& range = interval->operator[](j);
                        size_t ctSize = colortypes->operator[](j+ colortypesBefore)->size();
                        
                        int32_t lower_val = ctSize +(range._lower +1);
                        int32_t upper_val = ctSize +(range._upper +1);
                        range._lower = lower_val % ctSize;
                        range._upper = upper_val % ctSize;
                        
                        if(range._upper+1 == range._lower){
                            if(tempIntervals.empty()){
                                newInterval.addRange(0, ctSize-1);
                                tempIntervals.push_back(newInterval);
                            } else {
                                for (auto& tempInterval : tempIntervals){ 
                                    tempInterval.addRange(0, ctSize-1);
                                }
                            }
                        } else if(range._upper < range._lower ){
                            
                            if(tempIntervals.empty()){
                                auto intervalCopy = newInterval;
                                newInterval.addRange(range._lower, ctSize-1);
                                intervalCopy.addRange(0,range._upper);
                                tempIntervals.push_back(newInterval);
                                tempIntervals.push_back(intervalCopy);
                            } else {
                                std::vector<Reachability::interval_t> newTempIntervals;
                                for(auto tempInterval : tempIntervals){
                                    auto intervalCopy = tempInterval;
                                    tempInterval.addRange(range._lower, ctSize-1);
                                    intervalCopy.addRange(0,range._upper);
                                    newTempIntervals.push_back(intervalCopy);
                                    newTempIntervals.push_back(tempInterval);
                                }
                                tempIntervals = newTempIntervals;
                            }                            
                        } else {
                            if(tempIntervals.empty()){
                                newInterval.addRange(range);
                                tempIntervals.push_back(newInterval);
                            } else {
                                for (auto& tempInterval : tempIntervals){ 
                                    tempInterval.addRange(range);
                                }
                            }
                        }
                    }

                    for(auto tempInterval : tempIntervals){
                        newIntervals.addInterval(tempInterval);
                    }                   
                }

                return newIntervals;
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
            
            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                //store the number of colortyps already in colortypes vector and use that as offset when indexing it
                auto colortypesBefore = colortypes->size();

                auto nestedInterval = _color->getOutputIntervals(varMap, colortypes);
                Reachability::intervalTuple_t newIntervals;

                for(uint32_t i = 0;  i < nestedInterval.size(); i++) {
                    Reachability::interval_t newInterval;
                    std::vector<Reachability::interval_t> tempIntervals;

                    auto interval = &nestedInterval[i];
                    for(uint32_t j = 0; j < interval->_ranges.size(); j++) {
                        auto& range = interval->operator[](j);
                        auto ctSize = colortypes->operator[](j + colortypesBefore)->size()-1;
                        
                        int32_t lower_val = ctSize +(range._lower -1);
                        int32_t upper_val = ctSize +(range._upper -1);
                        range._lower = lower_val % ctSize;
                        range._upper = upper_val % ctSize;

                        if(range._upper+1 == range._lower ){
                            if(tempIntervals.empty()){
                                newInterval.addRange(0, ctSize-1);
                                tempIntervals.push_back(newInterval);
                            } else {
                                for(auto& tempInterval : tempIntervals){
                                    tempInterval.addRange(0, ctSize-1);
                                }
                            }
                        } else if(range._upper < range._lower ){
                            if(tempIntervals.empty()){
                                auto intervalCopy = newInterval;
                                newInterval.addRange(range._lower, ctSize-1);
                                intervalCopy.addRange(0,range._upper);
                                tempIntervals.push_back(newInterval);
                                tempIntervals.push_back(intervalCopy);

                            } else {
                                std::vector<Reachability::interval_t> newTempIntervals;
                                for (auto tempInterval : tempIntervals){
                                    auto intervalCopy = tempInterval;
                                    tempInterval.addRange(range._lower, ctSize-1);
                                    intervalCopy.addRange(0, range._upper);
                                    newTempIntervals.push_back(tempInterval);
                                    newTempIntervals.push_back(intervalCopy);
                                }
                                tempIntervals = newTempIntervals;
                            }                            
                        } else {                            
                            if(tempIntervals.empty()){
                                newInterval.addRange(range);
                                tempIntervals.push_back(newInterval);
                            } else {
                                for(auto& tempInterval : tempIntervals){
                                    tempInterval.addRange(range);
                                }
                            }                   
                        }
                    }

                    for(auto tempInterval : tempIntervals){
                        newIntervals.addInterval(tempInterval);
                    }                    
                }
                return newIntervals;
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

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes) const override {
                Reachability::intervalTuple_t intervals;
                Reachability::intervalTuple_t intervalHolder;
                

                for(auto colorExp : _colors) {
                    Reachability::intervalTuple_t intervalHolder;
                    auto nested_intervals = colorExp->getOutputIntervals(varMap, colortypes);

                    if(intervals._intervals.empty()){
                        intervals = nested_intervals;
                    } else {                        
                        for(auto nested_interval : nested_intervals._intervals){
                            Reachability::intervalTuple_t newIntervals;
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

            virtual void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMap) const = 0;
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

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMap) const override {
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

                for(auto& varMap : variableMap){
                    for(auto varPositionPair : varPositionsL){
                        uint32_t index = varPositionPair.first;
                        if(varPositionsR.count(index)){
                            if(varMap.count(varPositionPair.second) == 0){
                                std::cout << "Unable to find left var " << varPositionPair.second->name << std::endl;
                            }
                            if(varMap.count(varPositionsR[index]) == 0){
                                std::cout << "Unable to find right var " << varPositionsR[index]->name << std::endl;
                            }
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            auto rightTupleInterval = &varMap[varPositionsR[index]];
                            int32_t leftVarModifier;
                            int32_t rightVarModifier;
                            for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                if(idModPair.first == index){
                                    leftVarModifier = idModPair.second;
                                    break;
                                }
                            }

                            for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                if(idModPair.first == index){
                                    rightVarModifier = idModPair.second;
                                    break;
                                }
                            }
                            auto leftIds = leftTupleInterval->getLowerIds(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                            auto rightIds = rightTupleInterval->getUpperIds(rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());

                            //comparing vars of same size
                            if(varPositionPair.second->colorType->productSize() == varPositionsR[index]->colorType->productSize()){
                                leftTupleInterval->constrainUpper(rightIds, true);
                                rightTupleInterval->constrainLower(leftIds, true);
                            } else if(varPositionPair.second->colorType->productSize() > varPositionsR[index]->colorType->productSize()){
                                std::vector<uint32_t> leftLowerVec(leftIds.begin(), leftIds.begin() + rightTupleInterval->tupleSize());
                                

                                auto idVec = rightIds;
                                index += varPositionsR[index]->colorType->productSize();
                                while(idVec.size() < leftTupleInterval->tupleSize()){
                                    if(varPositionsR.count(index)){
                                        auto rightTupleInterval = &varMap[varPositionsR[index]];
                                        int32_t rightVarMod;
                                        for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                            if(idModPair.first == index){
                                                rightVarMod = idModPair.second;
                                                break;
                                            }
                                        }
                                        auto ids = rightTupleInterval->getUpperIds(rightVarMod, varPositionsR[index]->colorType->getConstituentsSizes());
                                        idVec.insert(idVec.end(), ids.begin(), ids.end());
                                        index += varPositionsR[index]->colorType->productSize();
                                    } else {
                                        auto oldSize = idVec.size();
                                        constantMapR[index]->getTupleId(&idVec); 
                                        int32_t leftVarMod;
                                        for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                            if(idModPair.first == index){
                                                leftVarMod = idModPair.second;
                                                break;
                                            }
                                        }

                                        for(auto& id : idVec){
                                            id = (varPositionPair.second->colorType->size()+(id + leftVarMod)) & varPositionPair.second->colorType->size();
                                        }                                   
                                        index+= idVec.size() - oldSize;
                                    }
                                }

                                leftTupleInterval->constrainUpper(idVec, true);
                                rightTupleInterval->constrainLower(leftLowerVec, true);
                            } else {
                                std::vector<uint32_t> rightUpperVec(rightIds.begin(), rightIds.begin() + leftTupleInterval->tupleSize());
                                
                                auto idVec = leftIds;
                                auto oldIndex = index;
                                index += varPositionsL[index]->colorType->productSize();
                                while(idVec.size() < rightTupleInterval->tupleSize()){
                                    if(varPositionsL.count(index)){
                                        auto leftTupleInterval = &varMap[varPositionsL[index]];
                                        int32_t leftVarMod;
                                        for(auto idModPair : varModifierMapL[varPositionsL[index]].back()){
                                            if(idModPair.first == index){
                                                leftVarMod = idModPair.second;
                                                break;
                                            }
                                        }
                                        auto ids = leftTupleInterval->getLowerIds(leftVarMod, varPositionsL[index]->colorType->getConstituentsSizes());
                                        idVec.insert(idVec.end(), ids.begin(), ids.end());
                                        index += varPositionsL[index]->colorType->productSize();
                                    } else {
                                        auto oldSize = idVec.size();
                                        constantMapL[index]->getTupleId(&idVec);
                                        int32_t rightVarMod;
                                        for(auto idModPair : varModifierMapR[varPositionsR[oldIndex]].back()){
                                            if(idModPair.first == index){
                                                rightVarMod = idModPair.second;
                                                break;
                                            }
                                        }

                                        for(auto& id : idVec){
                                            int32_t val = varPositionsR[oldIndex]->colorType->size() + (id + rightVarMod);
                                            id = val % varPositionsR[oldIndex]->colorType->size();
                                        }
                                        index+= idVec.size() - oldSize;
                                    }
                                }

                                leftTupleInterval->constrainUpper(rightUpperVec, true);
                                rightTupleInterval->constrainLower(idVec, true);
                            }
                        } else {
                            auto rightColor = constantMapR[index];
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            rightColor->getTupleId(&idVec);

                            index += idVec.size();
                            while(idVec.size() < leftTupleInterval->tupleSize()){
                                if(varPositionsR.count(index)){
                                    auto rightTupleInterval = &varMap[varPositionsR[index]];
                                    int32_t rightVarMod;
                                    for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                        if(idModPair.first == index){
                                            rightVarMod = idModPair.second;
                                            break;
                                        }
                                    }
                                    auto ids = rightTupleInterval->getUpperIds(rightVarMod, varPositionsR[index]->colorType->getConstituentsSizes());
                                    idVec.insert(idVec.end(), ids.begin(), ids.end());
                                    index += varPositionsR[index]->colorType->productSize();
                                } else {
                                    auto oldSize = idVec.size();
                                    constantMapR[index]->getTupleId(&idVec);
                                    int32_t leftVarMod;
                                    for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                        if(idModPair.first == index){
                                            leftVarMod = idModPair.second;
                                            break;
                                        }
                                    }

                                    for(auto& id : idVec){
                                        int32_t val = varPositionPair.second->colorType->size() + (id + leftVarMod);
                                        id = val % varPositionPair.second->colorType->size();
                                    }
                                    index+= idVec.size() - oldSize;
                                }
                            }
                            leftTupleInterval->constrainUpper(idVec, true);
                        }
                    }

                    for(auto varPositionPair : varPositionsR){
                        uint32_t index = varPositionPair.first;

                        if(constantMapL.count(index)){
                            auto leftColor = constantMapL[index];
                            auto rightTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            leftColor->getTupleId(&idVec);

                            index += idVec.size();
                            while(idVec.size() < rightTupleInterval->tupleSize()){
                                if(varPositionsL.count(index)){
                                    auto leftTupleInterval = &varMap[varPositionsL[index]];
                                    int32_t leftVarMod;
                                    for(auto idModPair : varModifierMapL[varPositionsL[index]].back()){
                                        if(idModPair.first == index){
                                            leftVarMod = idModPair.second;
                                            break;
                                        }
                                    }
                                    auto ids = leftTupleInterval->getLowerIds(leftVarMod, varPositionsL[index]->colorType->getConstituentsSizes());
                                    idVec.insert(idVec.end(), ids.begin(), ids.end());
                                    index += varPositionsL[index]->colorType->productSize();
                                } else {
                                    auto oldSize = idVec.size();
                                    constantMapL[index]->getTupleId(&idVec);
                                    int32_t rightVarMod;
                                    for(auto idModPair : varModifierMapR[varPositionPair.second].back()){
                                        if(idModPair.first == index){
                                            rightVarMod = idModPair.second;
                                            break;
                                        }
                                    }

                                    for(auto& id : idVec){
                                        int32_t val = varPositionPair.second->colorType->size() + (id + rightVarMod);
                                        id = val % varPositionPair.second->colorType->size();
                                    }
                                    index+= idVec.size() - oldSize;
                                }
                            }
                            rightTupleInterval->constrainUpper(idVec, true);
                        }
                    }
                }
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

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMap) const override {
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

                for(auto& varMap : variableMap){
                    for(auto varPositionPair : varPositionsL){
                        uint32_t index = varPositionPair.first;
                        if(varPositionsR.count(index)){
                            if(varMap.count(varPositionPair.second) == 0){
                                std::cout << "Unable to find left var " << varPositionPair.second->name << std::endl;
                            }
                            if(varMap.count(varPositionsR[index]) == 0){
                                std::cout << "Unable to find right var " << varPositionsR[index]->name << std::endl;
                            }
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            auto rightTupleInterval = &varMap[varPositionsR[index]];
                            int32_t leftVarModifier;
                            int32_t rightVarModifier;
                            for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                if(idModPair.first == index){
                                    leftVarModifier = idModPair.second;
                                    break;
                                }
                            }

                            for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                if(idModPair.first == index){
                                    rightVarModifier = idModPair.second;
                                    break;
                                }
                            }

                            auto leftIds = leftTupleInterval->getUpperIds(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                            auto rightIds = rightTupleInterval->getLowerIds(rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());
                            //comparing vars of same size
                            if(varPositionPair.second->colorType->productSize() == varPositionsR[index]->colorType->productSize()){                     
                                leftTupleInterval->constrainLower(rightIds, true);
                                rightTupleInterval->constrainUpper(leftIds, true);
                            } else if(varPositionPair.second->colorType->productSize() > varPositionsR[index]->colorType->productSize()){
                                std::vector<uint32_t> leftUpperVec(leftIds.begin(), leftIds.begin() + rightTupleInterval->tupleSize());
                                

                                auto idVec = rightIds;
                                index += varPositionsR[index]->colorType->productSize();
                                while(idVec.size() < leftTupleInterval->tupleSize()){
                                    if(varPositionsR.count(index)){
                                        auto rightTupleInterval = &varMap[varPositionsR[index]];
                                        int32_t rightVarMod;
                                        for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                            if(idModPair.first == index){
                                                rightVarMod = idModPair.second;
                                                break;
                                            }
                                        }
                                        auto ids = rightTupleInterval->getLowerIds(rightVarMod, varPositionsR[index]->colorType->getConstituentsSizes());
                                        idVec.insert(idVec.end(), ids.begin(), ids.end());
                                        index += varPositionsR[index]->colorType->productSize();
                                    } else {
                                        auto oldSize = idVec.size();
                                        constantMapR[index]->getTupleId(&idVec); 
                                        int32_t leftVarMod;
                                        for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                            if(idModPair.first == index){
                                                leftVarMod = idModPair.second;
                                                break;
                                            }
                                        }

                                        for(auto& id : idVec){
                                            int32_t val = varPositionPair.second->colorType->size() + (id + leftVarMod);
                                            id = val % varPositionPair.second->colorType->size();
                                        }                                   
                                        index+= idVec.size() - oldSize;
                                    }
                                }

                                leftTupleInterval->constrainLower(idVec, true);
                                rightTupleInterval->constrainUpper(leftUpperVec, true);
                            } else {
                                std::vector<uint32_t> rightLowerVec(rightIds.begin(), rightIds.begin() + leftTupleInterval->tupleSize());
                                
                                auto idVec = leftIds;
                                auto oldIndex = index;
                                index += varPositionsL[index]->colorType->productSize();
                                while(idVec.size() < rightTupleInterval->tupleSize()){
                                    if(varPositionsL.count(index)){
                                        auto leftTupleInterval = &varMap[varPositionsL[index]];
                                        int32_t leftVarMod;
                                        for(auto idModPair : varModifierMapL[varPositionsL[index]].back()){
                                            if(idModPair.first == index){
                                                leftVarMod = idModPair.second;
                                                break;
                                            }
                                        }
                                        auto ids = leftTupleInterval->getUpperIds(leftVarMod, varPositionsL[index]->colorType->getConstituentsSizes());
                                        idVec.insert(idVec.end(), ids.begin(), ids.end());
                                        index += varPositionsL[index]->colorType->productSize();
                                    } else {
                                        auto oldSize = idVec.size();
                                        constantMapL[index]->getTupleId(&idVec);
                                        int32_t rightVarMod;
                                        for(auto idModPair : varModifierMapR[varPositionsR[oldIndex]].back()){
                                            if(idModPair.first == index){
                                                rightVarMod = idModPair.second;
                                                break;
                                            }
                                        }

                                        for(auto& id : idVec){
                                            int32_t val = varPositionsR[oldIndex]->colorType->size() + (id + rightVarMod);
                                            id = val % varPositionsR[oldIndex]->colorType->size();
                                        }
                                        index+= idVec.size() - oldSize;
                                    }
                                }

                                leftTupleInterval->constrainLower(rightLowerVec, true);
                                rightTupleInterval->constrainUpper(idVec, true);
                            }
                        } else {
                            auto rightColor = constantMapR[index];
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            rightColor->getTupleId(&idVec);

                            index += idVec.size();
                            while(idVec.size() < leftTupleInterval->tupleSize()){
                                if(varPositionsR.count(index)){
                                    auto rightTupleInterval = &varMap[varPositionsR[index]];
                                    int32_t rightVarMod;
                                    for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                        if(idModPair.first == index){
                                            rightVarMod = idModPair.second;
                                            break;
                                        }
                                    }
                                    auto ids = rightTupleInterval->getLowerIds(rightVarMod, varPositionsR[index]->colorType->getConstituentsSizes());
                                    idVec.insert(idVec.end(), ids.begin(), ids.end());
                                    index += varPositionsR[index]->colorType->productSize();
                                } else {
                                    auto oldSize = idVec.size();
                                    constantMapR[index]->getTupleId(&idVec);
                                    int32_t leftVarMod;
                                    for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                        if(idModPair.first == index){
                                            leftVarMod = idModPair.second;
                                            break;
                                        }
                                    }

                                    for(auto& id : idVec){
                                        int32_t val = varPositionPair.second->colorType->size() + (id + leftVarMod);
                                        id = val % varPositionPair.second->colorType->size();
                                    }
                                    index+= idVec.size() - oldSize;
                                }
                            }
                            leftTupleInterval->constrainLower(idVec, true);
                        }
                    }

                    for(auto varPositionPair : varPositionsR){
                        uint32_t index = varPositionPair.first;

                        if(constantMapL.count(index)){
                            auto leftColor = constantMapL[index];
                            auto rightTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            leftColor->getTupleId(&idVec);

                            index += idVec.size();
                            while(idVec.size() < rightTupleInterval->tupleSize()){
                                if(varPositionsL.count(index)){
                                    auto leftTupleInterval = &varMap[varPositionsL[index]];
                                    int32_t leftVarMod;
                                    for(auto idModPair : varModifierMapL[varPositionsL[index]].back()){
                                        if(idModPair.first == index){
                                            leftVarMod = idModPair.second;
                                            break;
                                        }
                                    }
                                    auto ids = leftTupleInterval->getUpperIds(leftVarMod, varPositionsL[index]->colorType->getConstituentsSizes());
                                    idVec.insert(idVec.end(), ids.begin(), ids.end());
                                    index += varPositionsL[index]->colorType->productSize();
                                } else {
                                    auto oldSize = idVec.size();
                                    constantMapL[index]->getTupleId(&idVec);
                                    int32_t rightVarMod;
                                    for(auto idModPair : varModifierMapR[varPositionPair.second].back()){
                                        if(idModPair.first == index){
                                            rightVarMod = idModPair.second;
                                            break;
                                        }
                                    }

                                    for(auto& id : idVec){
                                        int32_t val = varPositionPair.second->colorType->size() + (id + rightVarMod);
                                        id = val % varPositionPair.second->colorType->size();
                                    }
                                    index+= idVec.size() - oldSize;
                                }
                            }
                            rightTupleInterval->constrainUpper(idVec, true);
                        }
                    }
                }

                
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

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMap) const override {
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

                for(auto& varMap : variableMap){
                    for(auto varPositionPair : varPositionsL){
                        uint32_t index = varPositionPair.first;
                        if(varPositionsR.count(index)){
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            auto rightTupleInterval = &varMap[varPositionsR[index]];
                            int32_t leftVarModifier;
                            int32_t rightVarModifier;
                            for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                if(idModPair.first == index){
                                    leftVarModifier = idModPair.second;
                                    break;
                                }
                            }

                            for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                if(idModPair.first == index){
                                    rightVarModifier = idModPair.second;
                                    break;
                                }
                            }


                            auto leftIds = leftTupleInterval->getLowerIds(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                            auto rightIds = rightTupleInterval->getUpperIds(rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());
                            //comparing vars of same size
                            if(varPositionPair.second->colorType->productSize() == varPositionsR[index]->colorType->productSize()){                        
                                leftTupleInterval->constrainUpper(rightIds, false);
                                rightTupleInterval->constrainLower(leftIds, false);
                            } else if(varPositionPair.second->colorType->productSize() > varPositionsR[index]->colorType->productSize()){
                                std::vector<uint32_t> leftLowerVec(leftIds.begin(), leftIds.begin() + rightTupleInterval->tupleSize());
                                
                                auto idVec = rightIds;
                                index += varPositionsR[index]->colorType->productSize();
                                while(idVec.size() < leftTupleInterval->tupleSize()){
                                    if(varPositionsR.count(index)){
                                        auto rightTupleInterval = &varMap[varPositionsR[index]];
                                        int32_t rightVarMod;
                                        for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                            if(idModPair.first == index){
                                                rightVarMod = idModPair.second;
                                                break;
                                            }
                                        }
                                        auto ids = rightTupleInterval->getUpperIds(rightVarMod, varPositionsR[index]->colorType->getConstituentsSizes());
                                        idVec.insert(idVec.end(), ids.begin(), ids.end());
                                        index += varPositionsR[index]->colorType->productSize();
                                    } else {
                                        auto oldSize = idVec.size();
                                        constantMapR[index]->getTupleId(&idVec);  
                                        int32_t leftVarMod;
                                        for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                            if(idModPair.first == index){
                                                leftVarMod = idModPair.second;
                                                break;
                                            }
                                        }

                                        for(auto& id : idVec){
                                            int32_t val = varPositionPair.second->colorType->size() + (id + leftVarMod);
                                            id = val % varPositionPair.second->colorType->size();
                                        }                                  
                                        index+= idVec.size() - oldSize;
                                    }
                                }

                                leftTupleInterval->constrainUpper(idVec, false);
                                rightTupleInterval->constrainLower(leftLowerVec, false);
                            } else {
                                std::vector<uint32_t> rightUpperVec(rightIds.begin(), rightIds.begin() + leftTupleInterval->tupleSize());
                                

                                auto idVec = leftIds;
                                auto oldIndex = index;
                                index += varPositionsL[index]->colorType->productSize();
                                while(idVec.size() < rightTupleInterval->tupleSize()){
                                    if(varPositionsL.count(index)){
                                        auto leftTupleInterval = &varMap[varPositionsL[index]];
                                        int32_t leftVarMod;
                                        for(auto idModPair : varModifierMapL[varPositionsL[index]].back()){
                                            if(idModPair.first == index){
                                                leftVarMod = idModPair.second;
                                                break;
                                            }
                                        }
                                        auto ids = leftTupleInterval->getLowerIds(leftVarMod, varPositionsL[index]->colorType->getConstituentsSizes());
                                        idVec.insert(idVec.end(), ids.begin(), ids.end());
                                        index += varPositionsL[index]->colorType->productSize();
                                    } else {
                                        auto oldSize = idVec.size();
                                        constantMapL[index]->getTupleId(&idVec);
                                        int32_t rightVarMod;
                                        for(auto idModPair : varModifierMapR[varPositionsR[oldIndex]].back()){
                                            if(idModPair.first == index){
                                                rightVarMod = idModPair.second;
                                                break;
                                            }
                                        }

                                        for(auto& id : idVec){
                                            int32_t val = varPositionsR[oldIndex]->colorType->size() + (id + rightVarMod);
                                            id = val % varPositionsR[oldIndex]->colorType->size();
                                        }
                                        index+= idVec.size() - oldSize;
                                    }
                                }

                                leftTupleInterval->constrainUpper(rightUpperVec, false);
                                rightTupleInterval->constrainLower(idVec, false);
                            }
                        } else {
                            auto rightColor = constantMapR[index];
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            rightColor->getTupleId(&idVec);

                            index += idVec.size();
                            while(idVec.size() < leftTupleInterval->tupleSize()){
                                if(varPositionsR.count(index)){
                                    auto rightTupleInterval = &varMap[varPositionsR[index]];
                                    int32_t rightVarMod;
                                    for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                        if(idModPair.first == index){
                                            rightVarMod = idModPair.second;
                                            break;
                                        }
                                    }
                                    auto ids = rightTupleInterval->getUpperIds(rightVarMod, varPositionsR[index]->colorType->getConstituentsSizes());
                                    idVec.insert(idVec.end(), ids.begin(), ids.end());
                                    index += varPositionsR[index]->colorType->productSize();
                                } else {
                                    auto oldSize = idVec.size();
                                    constantMapR[index]->getTupleId(&idVec);
                                    int32_t leftVarMod;
                                    for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                        if(idModPair.first == index){
                                            leftVarMod = idModPair.second;
                                            break;
                                        }
                                    }

                                    for(auto& id : idVec){
                                        int32_t val = varPositionPair.second->colorType->size() + (id + leftVarMod);
                                        id = val % varPositionPair.second->colorType->size();
                                    }
                                    index+= idVec.size() - oldSize;
                                }
                            }
                            leftTupleInterval->constrainUpper(idVec, false);
                        }
                    }

                    for(auto varPositionPair : varPositionsR){
                        uint32_t index = varPositionPair.first;

                        if(constantMapL.count(index)){
                            auto leftColor = constantMapL[index];
                            auto rightTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            leftColor->getTupleId(&idVec);

                            index += idVec.size();
                            while(idVec.size() < rightTupleInterval->tupleSize()){
                                if(varPositionsL.count(index)){
                                    auto leftTupleInterval = &varMap[varPositionsL[index]];
                                    int32_t leftVarMod;
                                    for(auto idModPair : varModifierMapL[varPositionsL[index]].back()){
                                        if(idModPair.first == index){
                                            leftVarMod = idModPair.second;
                                            break;
                                        }
                                    }
                                    auto ids = leftTupleInterval->getLowerIds(leftVarMod, varPositionsL[index]->colorType->getConstituentsSizes());
                                    idVec.insert(idVec.end(), ids.begin(), ids.end());
                                    index += varPositionsL[index]->colorType->productSize();
                                } else {
                                    auto oldSize = idVec.size();
                                    constantMapL[index]->getTupleId(&idVec);
                                    int32_t rightVarMod;
                                    for(auto idModPair : varModifierMapR[varPositionPair.second].back()){
                                        if(idModPair.first == index){
                                            rightVarMod = idModPair.second;
                                            break;
                                        }
                                    }

                                    for(auto& id : idVec){
                                        int32_t val = varPositionPair.second->colorType->size() + (id + rightVarMod);
                                        id = val % varPositionPair.second->colorType->size();
                                    }
                                    index+= idVec.size() - oldSize;
                                }
                            }
                            rightTupleInterval->constrainUpper(idVec, false);
                        }
                    }
                }                
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

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMap) const override {
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

                for(auto& varMap : variableMap){
                    for(auto varPositionPair : varPositionsL){
                        uint32_t index = varPositionPair.first;
                        if(varPositionsR.count(index)){
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            auto rightTupleInterval = &varMap[varPositionsR[index]];
                            int32_t leftVarModifier;
                            int32_t rightVarModifier;
                            for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                if(idModPair.first == index){
                                    leftVarModifier = idModPair.second;
                                    break;
                                }
                            }

                            for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                if(idModPair.first == index){
                                    rightVarModifier = idModPair.second;
                                    break;
                                }
                            }

                            auto leftIds = leftTupleInterval->getUpperIds(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                            auto rightIds = rightTupleInterval->getLowerIds(rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());
                            //comparing vars of same size
                            if(varPositionPair.second->colorType->productSize() == varPositionsR[index]->colorType->productSize()){                        
                                leftTupleInterval->constrainLower(rightIds, false);
                                rightTupleInterval->constrainUpper(leftIds, false);
                            } else if(varPositionPair.second->colorType->productSize() > varPositionsR[index]->colorType->productSize()){
                                std::vector<uint32_t> leftUpperVec(leftIds.begin(), leftIds.begin() + rightTupleInterval->tupleSize());
                                

                                auto idVec = rightIds;
                                index += varPositionsR[index]->colorType->productSize();
                                while(idVec.size() < leftTupleInterval->tupleSize()){
                                    if(varPositionsR.count(index)){
                                        auto rightTupleInterval = &varMap[varPositionsR[index]];
                                        int32_t rightVarMod;
                                        for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                            if(idModPair.first == index){
                                                rightVarMod = idModPair.second;
                                                break;
                                            }
                                        }
                                        auto ids = rightTupleInterval->getLowerIds(rightVarMod, varPositionsR[index]->colorType->getConstituentsSizes());
                                        idVec.insert(idVec.end(), ids.begin(), ids.end());
                                        index += varPositionsR[index]->colorType->productSize();
                                    } else {
                                        auto oldSize = idVec.size();
                                        constantMapR[index]->getTupleId(&idVec);
                                        int32_t leftVarMod;
                                        for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                            if(idModPair.first == index){
                                                leftVarMod = idModPair.second;
                                                break;
                                            }
                                        }

                                        for(auto& id : idVec){
                                            int32_t val = varPositionPair.second->colorType->size() + (id + leftVarMod);
                                            id = val % varPositionPair.second->colorType->size();
                                        }                                    
                                        index+= idVec.size() - oldSize;
                                    }
                                }

                                leftTupleInterval->constrainLower(idVec, false);
                                rightTupleInterval->constrainUpper(leftUpperVec, false);
                            } else {
                                std::vector<uint32_t> rightLowerVec(rightIds.begin(), rightIds.begin() + leftTupleInterval->tupleSize());
                                

                                auto idVec = leftIds;
                                auto oldIndex = index;
                                index += varPositionsL[index]->colorType->productSize();
                                while(idVec.size() < rightTupleInterval->tupleSize()){
                                    if(varPositionsL.count(index)){
                                        auto leftTupleInterval = &varMap[varPositionsL[index]];
                                        int32_t leftVarMod;
                                        for(auto idModPair : varModifierMapL[varPositionsL[index]].back()){
                                            if(idModPair.first == index){
                                                leftVarMod = idModPair.second;
                                                break;
                                            }
                                        }
                                        auto ids = leftTupleInterval->getUpperIds(leftVarMod, varPositionsL[index]->colorType->getConstituentsSizes());
                                        idVec.insert(idVec.end(), ids.begin(), ids.end());
                                        index += varPositionsL[index]->colorType->productSize();
                                    } else {
                                        auto oldSize = idVec.size();
                                        constantMapL[index]->getTupleId(&idVec);
                                        int32_t rightVarMod;
                                        for(auto idModPair : varModifierMapR[varPositionsR[oldIndex]].back()){
                                            if(idModPair.first == index){
                                                rightVarMod = idModPair.second;
                                                break;
                                            }
                                        }

                                        for(auto& id : idVec){
                                            int32_t val = varPositionsR[oldIndex]->colorType->size() + (id + rightVarMod);
                                            id = val % varPositionsR[oldIndex]->colorType->size();
                                        }
                                        index+= idVec.size() - oldSize;
                                    }
                                }

                                leftTupleInterval->constrainLower(rightLowerVec, false);
                                rightTupleInterval->constrainUpper(idVec, false);
                            }
                        } else {
                            auto rightColor = constantMapR[index];
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            rightColor->getTupleId(&idVec);
                            index += idVec.size();
                            while(idVec.size() < leftTupleInterval->tupleSize()){
                                if(varPositionsR.count(index)){
                                    auto rightTupleInterval = &varMap[varPositionsR[index]];
                                    int32_t rightVarMod;
                                    for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                        if(idModPair.first == index){
                                            rightVarMod = idModPair.second;
                                            break;
                                        }
                                    }
                                    auto ids = rightTupleInterval->getLowerIds(rightVarMod, varPositionsR[index]->colorType->getConstituentsSizes());
                                    idVec.insert(idVec.end(), ids.begin(), ids.end());
                                    index += varPositionsR[index]->colorType->productSize();
                                } else {
                                    auto oldSize = idVec.size();
                                    constantMapR[index]->getTupleId(&idVec);
                                    int32_t leftVarMod;
                                    for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                        if(idModPair.first == index){
                                            leftVarMod = idModPair.second;
                                            break;
                                        }
                                    }

                                    for(auto& id : idVec){
                                        int32_t val = varPositionPair.second->colorType->size() + (id + leftVarMod);
                                        id = val % varPositionPair.second->colorType->size();
                                    }
                                    
                                    index+= idVec.size() - oldSize;
                                }
                            }
                            leftTupleInterval->constrainLower(idVec, false);
                        }
                    }

                    for(auto varPositionPair : varPositionsR){
                        uint32_t index = varPositionPair.first;

                        if(constantMapL.count(index)){
                            auto leftColor = constantMapL[index];
                            auto rightTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            leftColor->getTupleId(&idVec);

                            index += idVec.size();
                            while(idVec.size() < rightTupleInterval->tupleSize()){
                                if(varPositionsL.count(index)){
                                    auto leftTupleInterval = &varMap[varPositionsL[index]];
                                    int32_t leftVarMod;
                                    for(auto idModPair : varModifierMapL[varPositionsL[index]].back()){
                                        if(idModPair.first == index){
                                            leftVarMod = idModPair.second;
                                            break;
                                        }
                                    }
                                    auto ids = leftTupleInterval->getUpperIds(leftVarMod, varPositionsL[index]->colorType->getConstituentsSizes());
                                    idVec.insert(idVec.end(), ids.begin(), ids.end());
                                    index += varPositionsL[index]->colorType->productSize();
                                } else {
                                    auto oldSize = idVec.size();
                                    constantMapL[index]->getTupleId(&idVec);
                                    int32_t rightVarMod;
                                    for(auto idModPair : varModifierMapR[varPositionPair.second].back()){
                                        if(idModPair.first == index){
                                            rightVarMod = idModPair.second;
                                            break;
                                        }
                                    }

                                    for(auto& id : idVec){
                                        int32_t val = varPositionPair.second->colorType->size() + (id + rightVarMod);
                                        id = val % varPositionPair.second->colorType->size();
                                    }
                                    index+= idVec.size() - oldSize;
                                }
                            }
                            rightTupleInterval->constrainUpper(idVec, false);
                        }
                    }
                }

                
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

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMap) const override {
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

                for(auto& varMap : variableMap){
                    for(auto varPositionPair : varPositionsL){
                        uint32_t index = varPositionPair.first;
                        if(varPositionsR.count(index)){
                            auto leftTupleIntervalVal = varMap[varPositionPair.second];
                            auto rightTupleIntervalVal = varMap[varPositionsR[index]];
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            auto rightTupleInterval = &varMap[varPositionsR[index]];
                            int32_t leftVarModifier;
                            int32_t rightVarModifier;
                            for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                if(idModPair.first == index){
                                    leftVarModifier = idModPair.second;
                                    break;
                                }
                            }

                            for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                if(idModPair.first == index){
                                    rightVarModifier = idModPair.second;
                                    break;
                                }
                            }

                            leftTupleIntervalVal.applyModifier(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                            rightTupleIntervalVal.applyModifier(rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());
                            //comparing vars of same size
                            if(varPositionPair.second->colorType->productSize() == varPositionsR[index]->colorType->productSize()){
                                Reachability::intervalTuple_t newIntervalTuple;
                                for(auto leftInterval : leftTupleIntervalVal._intervals){
                                    for(auto rightInterval : rightTupleIntervalVal._intervals){
                                        auto intervalOverlap = leftInterval.getOverlap(rightInterval);

                                        if(intervalOverlap.isSound()){
                                            newIntervalTuple.addInterval(intervalOverlap);
                                        }
                                    }
                                }
                            
                                *leftTupleInterval = newIntervalTuple;
                                *rightTupleInterval = newIntervalTuple;
                                leftTupleInterval->applyModifier(-leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                                rightTupleInterval->applyModifier(-rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());
                            } else if(varPositionPair.second->colorType->productSize() > varPositionsR[index]->colorType->productSize()){
                                std::vector<Reachability::interval_t> resizedLeftIntervals;
                                for(auto interval : leftTupleIntervalVal._intervals){
                                    Reachability::interval_t resizedInterval;
                                    for(uint32_t i = 0; i < varPositionsR[index]->colorType->productSize(); i++){
                                        resizedInterval.addRange(interval[i]);
                                    }
                                    resizedLeftIntervals.push_back(resizedInterval);
                                }

                                auto intervalVec = rightTupleIntervalVal._intervals;
                                uint32_t i = index;
                                i += varPositionsR[index]->colorType->productSize();
                                while(intervalVec.size() < leftTupleInterval->tupleSize()){
                                    if(varPositionsR.count(i)){
                                        auto rightTupleInterval = varMap[varPositionsR[i]];
                                        int32_t rightVarMod;
                                        for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                            if(idModPair.first == index){
                                                rightVarMod = idModPair.second;
                                                break;
                                            }
                                        }
                                        rightTupleInterval.applyModifier(rightVarMod, varPositionsR[index]->colorType->getConstituentsSizes());
                                        intervalVec.insert(intervalVec.end(), rightTupleInterval._intervals.begin(), rightTupleInterval._intervals.end());
                                        i += varPositionsR[i]->colorType->productSize();
                                    } else {
                                        std::vector<uint32_t> colorIdVec;
                                        constantMapR[i]->getTupleId(&colorIdVec);
                                        int32_t leftVarModifier;
                                        for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                            if(idModPair.first == index){
                                                leftVarModifier = idModPair.second;
                                                break;
                                            }
                                        }
                                        for(auto id : colorIdVec){
                                            for(auto interval : intervalVec){
                                                int32_t val = varPositionPair.second->colorType->size() + (id + leftVarModifier);
                                                auto colorVal = val % varPositionPair.second->colorType->size(); 
                                                interval.addRange(colorVal,colorVal);
                                            }
                                        }                                    
                                        i+= colorIdVec.size();
                                    }
                                }

                                Reachability::intervalTuple_t newIntervalTupleR;
                                for(auto rightInterval : rightTupleIntervalVal._intervals){
                                    for(auto leftInterval : resizedLeftIntervals){
                                        auto intervalOverlap = leftInterval.getOverlap(rightInterval);

                                        if(intervalOverlap.isSound()){
                                            newIntervalTupleR.addInterval(intervalOverlap);
                                        }
                                    }
                                }

                                Reachability::intervalTuple_t newIntervalTupleL;
                                for(auto leftInterval : leftTupleIntervalVal._intervals){
                                    for(auto rightInterval : intervalVec){
                                        auto intervalOverlap = leftInterval.getOverlap(rightInterval);

                                        if(intervalOverlap.isSound()){
                                            newIntervalTupleL.addInterval(intervalOverlap);
                                        }
                                    }
                                }
                                newIntervalTupleL.applyModifier(-leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                                newIntervalTupleR.applyModifier(-rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());

                                *leftTupleInterval = newIntervalTupleL;
                                *rightTupleInterval = newIntervalTupleR;
                            } else {
                                std::vector<Reachability::interval_t> resizedRightIntervals;
                                for(auto interval : rightTupleIntervalVal._intervals){
                                    Reachability::interval_t resizedInterval;
                                    for(uint32_t i = 0; i < varPositionsL[index]->colorType->productSize(); i++){
                                        resizedInterval.addRange(interval[i]);
                                    }
                                    resizedRightIntervals.push_back(resizedInterval);
                                }

                                auto intervalVec = leftTupleIntervalVal._intervals;
                                uint32_t i = index;
                                i += varPositionsL[index]->colorType->productSize();
                                while(intervalVec.size() < rightTupleInterval->tupleSize()){
                                    if(varPositionsL.count(i)){
                                        auto leftTupleInterval = varMap[varPositionsL[i]];
                                        int32_t leftVarMod;
                                        for(auto idModPair : varModifierMapL[varPositionsL[index]].back()){
                                            if(idModPair.first == index){
                                                leftVarMod = idModPair.second;
                                                break;
                                            }
                                        }
                                        leftTupleInterval.applyModifier(leftVarMod, varPositionsL[index]->colorType->getConstituentsSizes());
                                        intervalVec.insert(intervalVec.end(), leftTupleInterval._intervals.begin(), leftTupleInterval._intervals.end());
                                        i += varPositionsL[i]->colorType->productSize();
                                    } else {
                                        std::vector<uint32_t> colorIdVec;
                                        constantMapL[i]->getTupleId(&colorIdVec);
                                        int32_t rightVarModifier;
                                        for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                            if(idModPair.first == index){
                                                rightVarModifier = idModPair.second;
                                                break;
                                            }
                                        }
                                        for(auto id : colorIdVec){
                                            for(auto interval : intervalVec){
                                                int32_t val = varPositionsR[index]->colorType->size() + (id + rightVarModifier);
                                                auto colorVal = val % varPositionsR[index]->colorType->size(); 
                                                interval.addRange(colorVal,colorVal);
                                            }
                                        }                                   
                                        i+= colorIdVec.size();
                                    }
                                }

                                Reachability::intervalTuple_t newIntervalTupleL;
                                for(auto leftInterval : leftTupleIntervalVal._intervals){
                                    for(auto rightInterval : resizedRightIntervals){
                                        auto intervalOverlap = leftInterval.getOverlap(rightInterval);

                                        if(intervalOverlap.isSound()){
                                            newIntervalTupleL.addInterval(intervalOverlap);
                                        }
                                    }
                                }

                                Reachability::intervalTuple_t newIntervalTupleR;
                                for(auto rightInterval : rightTupleIntervalVal._intervals){
                                    for(auto leftInterval : intervalVec){
                                        auto intervalOverlap = leftInterval.getOverlap(rightInterval);

                                        if(intervalOverlap.isSound()){
                                            newIntervalTupleR.addInterval(intervalOverlap);
                                        }
                                    }
                                }

                                newIntervalTupleL.applyModifier(-leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                                newIntervalTupleR.applyModifier(-rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());

                                *leftTupleInterval = newIntervalTupleL;
                                *rightTupleInterval = newIntervalTupleR;
                            }
                        } else {
                            auto rightColor = constantMapR[index];
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            rightColor->getTupleId(&idVec);
                            int32_t leftVarModifier;
                            for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                if(idModPair.first == index){
                                    leftVarModifier = idModPair.second;
                                    break;
                                }
                            }

                            std::vector<Reachability::interval_t> intervals;
                            Reachability::interval_t interval;
                            for(auto id : idVec){  
                                int32_t val = varPositionPair.second->colorType->size() + (id + leftVarModifier);
                                auto colorVal = val % varPositionPair.second->colorType->size();                          
                                interval.addRange(colorVal,colorVal);
                            } 
                            intervals.push_back(interval);
                            
                            index += idVec.size();
                            while(intervals.size() < leftTupleInterval->tupleSize()){
                                if(varPositionsR.count(index)){
                                    auto rightTupleInterval = varMap[varPositionsR[index]];
                                    int32_t rightVarMod;
                                    for(auto idModPair : varModifierMapR[varPositionsR[index]].back()){
                                        if(idModPair.first == index){
                                            rightVarMod = idModPair.second;
                                            break;
                                        }
                                    }
                                    rightTupleInterval.applyModifier(rightVarMod, varPositionsR[index]->colorType->getConstituentsSizes());
                                    intervals.insert(intervals.end(), rightTupleInterval._intervals.begin(), rightTupleInterval._intervals.end());
                                    index += varPositionsR[index]->colorType->productSize();
                                } else {
                                    std::vector<uint32_t> colorIdVec;
                                    constantMapR[index]->getTupleId(&colorIdVec);
                                    int32_t leftVarModifier;
                                    for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                        if(idModPair.first == index){
                                            leftVarModifier = idModPair.second;
                                            break;
                                        }
                                    }
                                    for(auto id : colorIdVec){
                                        for(auto interval : intervals){
                                            int32_t val = varPositionPair.second->colorType->size() + (id + leftVarModifier);
                                            auto colorVal = val % varPositionPair.second->colorType->size(); 
                                            interval.addRange(colorVal,colorVal);
                                        }
                                    }                                   
                                    index+= colorIdVec.size();
                                }
                            }

                            Reachability::intervalTuple_t newIntervalTupleL;
                            for(auto leftInterval : leftTupleInterval->_intervals){
                                for(auto rightInterval : intervals){
                                    auto intervalOverlap = leftInterval.getOverlap(rightInterval);

                                    if(intervalOverlap.isSound()){
                                        newIntervalTupleL.addInterval(intervalOverlap);
                                    }
                                }
                            }
                            *leftTupleInterval = newIntervalTupleL;
                        }
                    }

                    for(auto varPositionPair : varPositionsR){
                        uint32_t index = varPositionPair.first;

                        if(constantMapL.count(index)){
                            auto leftColor = constantMapL[index];
                            auto rightTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            leftColor->getTupleId(&idVec);
                            int32_t rightVarModifier;
                            for(auto idModPair : varModifierMapR[varPositionPair.second].back()){
                                if(idModPair.first == index){
                                    rightVarModifier = idModPair.second;
                                    break;
                                }
                            }

                            std::vector<Reachability::interval_t> intervals;
                            Reachability::interval_t interval;
                            for(auto id : idVec){          
                                int32_t val = varPositionPair.second->colorType->size() + (id + rightVarModifier);
                                auto colorVal = val % varPositionPair.second->colorType->size();                  
                                interval.addRange(colorVal,colorVal);
                            } 
                            intervals.push_back(interval);

                            index += idVec.size();
                            while(intervals.size() < rightTupleInterval->tupleSize()){
                                if(varPositionsL.count(index)){
                                    auto leftTupleInterval = varMap[varPositionsL[index]];
                                    int32_t leftVarMod;
                                    for(auto idModPair : varModifierMapL[varPositionsL[index]].back()){
                                        if(idModPair.first == index){
                                            leftVarMod = idModPair.second;
                                            break;
                                        }
                                    }
                                    leftTupleInterval.applyModifier(leftVarMod, varPositionsL[index]->colorType->getConstituentsSizes());
                                    intervals.insert(intervals.end(), leftTupleInterval._intervals.begin(), leftTupleInterval._intervals.end());
                                    index += varPositionsL[index]->colorType->productSize();
                                } else {
                                    std::vector<uint32_t> colorIdVec;
                                    constantMapL[index]->getTupleId(&colorIdVec);
                                    int32_t rightVarModifier;
                                    for(auto idModPair : varModifierMapL[varPositionPair.second].back()){
                                        if(idModPair.first == index){
                                            rightVarModifier = idModPair.second;
                                            break;
                                        }
                                    }
                                    for(auto id : colorIdVec){
                                        for(auto interval : intervals){
                                            int32_t val = varPositionPair.second->colorType->size() + (id + rightVarModifier);
                                            auto colorVal = val % varPositionPair.second->colorType->size(); 
                                            interval.addRange(colorVal,colorVal);
                                        }
                                    }                                   
                                    index+= colorIdVec.size();
                                }
                            }

                            Reachability::intervalTuple_t newIntervalTupleR;
                            for(auto rightInterval : rightTupleInterval->_intervals){
                                for(auto leftInterval : intervals){
                                    auto intervalOverlap = leftInterval.getOverlap(rightInterval);

                                    if(intervalOverlap.isSound()){
                                        newIntervalTupleR.addInterval(intervalOverlap);
                                    }
                                }
                            }
                            *rightTupleInterval = newIntervalTupleR;
                        }
                    }
                }

                
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

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMap) const override {
               //this is whatever for now
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

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMap) const override {
                std::set<const Colored::Variable *> variables;
                _expr->getVariables(variables);
                //TODO: invert the var intervals here instead of using the full intervals

                for(auto var : variables){
                    auto fullInterval = var->colorType->getFullInterval();
                    Reachability::intervalTuple_t fullTuple;
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

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMap) const override {
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

            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMap) const override{
                auto varMapCopy = variableMap;
                _left->restrictVars(variableMap);
                _right->restrictVars(varMapCopy);


                for(auto pair : varMapCopy){
                    variableMap.push_back(pair);
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

            virtual Reachability::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>>& varMapVec) const {
                std::vector<const Colored::ColorType *> colortypes;
                
                return getOutputIntervals(varMapVec, &colortypes);
            }

            virtual Reachability::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const {
                return Reachability::intervalTuple_t();
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

            Reachability::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const {
                Reachability::intervalTuple_t newIntervalTuple;
                newIntervalTuple.addInterval(_sort->getFullInterval());
                return newIntervalTuple;
            }

            bool getArcIntervals(Colored::ArcIntervals& arcIntervals, PetriEngine::Colored::ColorFixpoint& cfp, uint32_t *index, int32_t modifier) const {
                //TODO: fix to work even if intervals are not merged
                Reachability::range_t fullRange(0, _sort->size()-1);
                
                
                if(arcIntervals._intervalTupleVec.empty()){
                    bool colorsInFixpoint = false;
                    Reachability::intervalTuple_t newIntervalTuple;
                    for(auto interval : cfp.constraints._intervals){
                        if(interval[*index].compare(fullRange).first){
                            colorsInFixpoint = true;
                            newIntervalTuple.addInterval(interval);
                        }
                    }
                    arcIntervals._intervalTupleVec.push_back(newIntervalTuple);
                    return colorsInFixpoint;
                } else {
                    for(auto& intervalTuple : arcIntervals._intervalTupleVec){
                        std::vector<uint32_t> intervalsForRemoval;
                        for(uint32_t i = 0; i < intervalTuple._intervals.size(); i++){
                            if(!intervalTuple[i][*index].compare(fullRange).first){
                                intervalsForRemoval.push_back(i);
                            }
                        }

                        for (auto i = intervalsForRemoval.rbegin(); i != intervalsForRemoval.rend(); ++i) {
                            intervalTuple.removeInterval(*i);
                        }
                    }                   

                    return !arcIntervals._intervalTupleVec[0]._intervals.empty();
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

            Reachability::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const override {
                Reachability::intervalTuple_t intervals;
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
                    std::vector<Reachability::interval_t> newIntervals;
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

            Reachability::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const override {
                Reachability::intervalTuple_t intervals;
                
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

            Reachability::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const override {
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

            Reachability::intervalTuple_t getOutputIntervals(std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>>& varMapVec, std::vector<const Colored::ColorType *> *colortypes) const override {
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

