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
            typedef std::unordered_map<std::string, const PetriEngine::Colored::Color *> BindingMap;

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
        
            virtual void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const {}

            virtual void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap) const {
                uint32_t index = 0;
                getVariables(variables, varPositions, varModifierMap, &index);
            }


            virtual void getVariables(std::set<Variable*>& variables) const {
                std::unordered_map<std::string, std::set<uint32_t>> varPositions;
                std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>> varModifierMap;

                getVariables(variables, varPositions, varModifierMap);
            }

            virtual Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, PetriEngine::Colored::VariableInterval> *varIntervals) const {
                std::vector<Colored::ColorType *> colortypes;
                
                return getOutputIntervals(varIntervals, &colortypes);
            }

            virtual Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, PetriEngine::Colored::VariableInterval> *varIntervals, std::vector<Colored::ColorType *> *colortypes) const {
                return Reachability::intervalTuple_t();
            }

            virtual void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const {
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

            virtual void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const = 0;

            virtual bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const = 0;

            virtual ColorType* getColorType(std::unordered_map<std::string,Colored::ColorType*>& colorTypes) const = 0;
        };
        
        class DotConstantExpression : public ColorExpression {
        public:
            const Color* eval(ExpressionContext& context) const override {
                return DotConstant::dotConstant();
            }

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                restrictionValue->emplace_back(0);
                *intervalSize = 0;
                return true;
            }

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const override {
                const Color *dotColor = DotConstant::dotConstant();
                constantMap[dotColor].push_back(index);
            }
            ColorType* getColorType(std::unordered_map<std::string,Colored::ColorType*>& colorTypes) const override{
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
                return context.binding[_variable->name];
            }

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                return false;
            }
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *, std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                variables.insert(_variable);
                varPositions[_variable->name].insert(*index);
                varModifierMap[_variable].push_back(std::make_pair(*index, 0));
            }

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, Colored::VariableInterval> *varIntervals, std::vector<Colored::ColorType *> *colortypes) const override {
                auto varInterval = varIntervals->find(_variable->name);
                
                if (varInterval == varIntervals->end()){
                    Reachability::interval_t interval = _variable->colorType->getFullInterval();
                    Reachability::intervalTuple_t rangeInterval;
                    rangeInterval.addInterval(interval);                
                    varIntervals->insert(std::make_pair(_variable->name, Colored::VariableInterval(_variable, rangeInterval)));
                }

                varInterval = varIntervals->find(_variable->name);

                std::vector<ColorType*> varColorTypes;

                _variable->colorType->getColortypes(varColorTypes);

                for(auto ct : varColorTypes){
                    colortypes->push_back(ct);
                }              
                
                return varInterval->second._intervalTuple;
            }

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const override {
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                Colored::Pattern pattern(
                    Colored::PatternType::Var,
                    this,
                    {_variable},
                    getColorType(colorTypes)
                );
                pattern.toString();
                //std::pair<std::set<Colored::Pattern>::iterator, bool> res = patterns.insert(pattern);
            }

            std::string toString() const override {
                return _variable->name;
            }

            ColorType* getColorType(std::unordered_map<std::string,Colored::ColorType*>& colorTypes) const override{
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

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                restrictionValue->emplace_back(_userOperator->getId());
                *intervalSize = _userOperator->getColorType()->size();
                return true;
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                Colored::Pattern pattern(
                    Colored::PatternType::Constant,
                    this,
                    std::set<Variable*>{},
                    getColorType(colorTypes)
                );

                patterns.insert(pattern);
            }
            std::string toString() const override {
                return _userOperator->toString();
            }

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const override {
                constantMap[_userOperator].push_back(index);
            }
            ColorType* getColorType(std::unordered_map<std::string,Colored::ColorType*>& colorTypes) const override{
                return _userOperator->getColorType();
            }

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, Colored::VariableInterval> *varIntervals, std::vector<Colored::ColorType *> *colortypes) const override {
                Reachability::interval_t interval;
                Reachability::intervalTuple_t rangeInterval;
                 
                colortypes->push_back(_userOperator->getColorType());
                auto colorId = _userOperator->getId();
                
                interval.addRange(colorId, colorId);
                rangeInterval.addInterval(interval);
                return rangeInterval;
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                //save index before evaluating nested expression to decrease all the correct modifiers
                uint32_t indexBefore = *index;
                _color->getVariables(variables, varPositions, varModifierMap, index);
                for(auto& varModifierPair : varModifierMap){
                    for(auto& idModPair : varModifierPair.second){
                        if(idModPair.first <= *index && idModPair.first >= indexBefore){
                            idModPair.second--;
                        } 
                    }                   
                }                
            }

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, Colored::VariableInterval> *varIntervals, std::vector<Colored::ColorType *> *colortypes) const override {
                //store the number of colortyps already in colortypes vector and use that as offset when indexing it
                auto colortypesBefore = colortypes->size();

                auto nestedInterval = _color->getOutputIntervals(varIntervals, colortypes);
                Reachability::intervalTuple_t newIntervals;

                for(uint32_t i = 0;  i < nestedInterval.size(); i++) {
                    Reachability::interval_t newInterval;
                    std::vector<Reachability::interval_t> tempIntervals;
                    auto interval = &nestedInterval[i];
                    for(uint32_t j = 0; j < interval->_ranges.size(); j++) {
                        auto& range = interval->operator[](j);
                        
                        range._lower++;
                        if(range._upper < colortypes->operator[](j+ colortypesBefore)->size()-1){
                            range._upper++;
                            if(tempIntervals.empty()){
                                newInterval.addRange(range);
                                tempIntervals.push_back(newInterval);
                            } else {
                                for(auto tempInterval : tempIntervals){
                                    tempInterval.addRange(range);
                                }
                            }                            
                        } else {
                            if(tempIntervals.empty()){
                                auto intervalCopy = newInterval;
                                newInterval.addRange(range);
                                intervalCopy.addRange(0,0);
                                tempIntervals.push_back(newInterval);
                                tempIntervals.push_back(intervalCopy);
                            }
                        }
                    }

                    for(auto tempInterval : tempIntervals){
                        newIntervals.addInterval(tempInterval);
                    }                   
                }
                newIntervals.mergeIntervals();

                return newIntervals;
            }

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                if (_color->getVariableRestriction(index, restrictionValue, intervalSize)){
                    uint32_t lastElement = restrictionValue->back();
                    if(lastElement > 0) {
                        lastElement--;
                    } else {
                        lastElement = *intervalSize;
                    }

                    restrictionValue->pop_back();
                    restrictionValue->push_back(lastElement);
                    return true;
                }
                return false;
            }

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const override {
                return _color->getConstants(constantMap, index);
            }

            std::string toString() const override {
                return _color->toString() + "++";
            }

            ColorType* getColorType(std::unordered_map<std::string,Colored::ColorType*>& colorTypes) const override{
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                //save index before evaluating nested expression to decrease all the correct modifiers
                uint32_t indexBefore = *index;
                _color->getVariables(variables, varPositions, varModifierMap, index);
                for(auto& varModifierPair : varModifierMap){
                    for(auto& idModPair : varModifierPair.second){
                        if(idModPair.first <= *index && idModPair.first >= indexBefore){
                            idModPair.second++;
                        } 
                    }                   
                } 
            }
            
            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, Colored::VariableInterval> *varIntervals, std::vector<Colored::ColorType *> *colortypes) const override {
                //store the number of colortyps already in colortypes vector and use that as offset when indexing it
                auto colortypesBefore = colortypes->size();

                auto nestedInterval = _color->getOutputIntervals(varIntervals, colortypes);
                Reachability::intervalTuple_t newIntervals;

                for(uint32_t i = 0;  i < nestedInterval.size(); i++) {
                    Reachability::interval_t newInterval;
                    std::vector<Reachability::interval_t> tempIntervals;

                    auto interval = &nestedInterval[i];
                    for(uint32_t j = 0; j < interval->_ranges.size(); j++) {
                        auto& range = interval->operator[](j);
                        range._upper--;
                        if(range._lower > 0){
                            range._lower--;
                            if(tempIntervals.empty()){
                                newInterval.addRange(range);
                                tempIntervals.push_back(newInterval);
                            } else {
                                for(auto tempInterval : tempIntervals){
                                    tempInterval.addRange(range);
                                }
                            }
                            
                        } else {
                            range._lower = 0;
                            auto size = colortypes->operator[](j + colortypesBefore)->size()-1;
                            if(tempIntervals.empty()){
                                auto intervalCopy = newInterval;
                                newInterval.addRange(range);
                                intervalCopy.addRange(size,size);
                                tempIntervals.push_back(newInterval);
                                tempIntervals.push_back(intervalCopy);

                            } else {
                                for (auto tempInterval : tempIntervals){
                                    tempInterval.addRange(range);
                                    tempInterval.addRange(size, size);
                                }
                            }                            
                        }
                    }

                    for(auto tempInterval : tempIntervals){
                        newIntervals.addInterval(tempInterval);
                    }                    
                }
                newIntervals.mergeIntervals();
                return newIntervals;
            }

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                if (_color->getVariableRestriction(index, restrictionValue, intervalSize)){
                    
                    auto lastElement = restrictionValue->back();
                    if(lastElement < (*intervalSize) -1) {
                        lastElement++;
                    } else {
                        lastElement = 0;
                    }
                    restrictionValue->pop_back();
                    restrictionValue->push_back(lastElement);
                    return true;
                }
                return false;
            }

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const override {
                return _color->getConstants(constantMap, index);
            }

            std::string toString() const override {
                return _color->toString() + "--";
            }
            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                _color->getPatterns(patterns, colorTypes);
            }

            ColorType* getColorType(std::unordered_map<std::string,Colored::ColorType*>& colorTypes) const override{
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

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, Colored::VariableInterval> *varIntervals, std::vector<Colored::ColorType *> *colortypes) const override {
                Reachability::intervalTuple_t intervals;
                Reachability::intervalTuple_t intervalHolder;
                

                for(auto colorExp : _colors) {
                    Reachability::intervalTuple_t intervalHolder;
                    auto nested_intervals = colorExp->getOutputIntervals(varIntervals, colortypes);

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
                            newIntervals.mergeIntervals();
                            intervalHolder._intervals.insert(intervalHolder._intervals.end(), newIntervals._intervals.begin(), newIntervals._intervals.end());
                        }
                        intervals = intervalHolder;
                    }                  
                }
                return intervals;
            }
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                for (auto elem : _colors) {
                    elem->getVariables(variables, varPositions, varModifierMap, index);
                    (*index)++;
                }
            }

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                
                if (index == 0) {
                    bool succes = true;
                    for (auto colorExp : _colors) {
                        if(!colorExp->getVariableRestriction(0, restrictionValue, intervalSize)){
                            succes = false;
                        }
                    }
                    return succes;
                }

                for(auto colorExp : _colors) {
                    index--;

                    bool succes = colorExp->getVariableRestriction(index, restrictionValue, intervalSize);
                    if(index == 0) {
                        return succes;
                    }
                }

                return false;
            }

            ColorType* getColorType(std::unordered_map<std::string,Colored::ColorType*>& colorTypes) const override{
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

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const override {
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

            virtual void restrictVar(Colored::VariableInterval *varInterval, std::unordered_map<std::string, Colored::VariableInterval> *varIntervals) const = 0;
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

            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVar(Colored::VariableInterval *varInterval, std::unordered_map<std::string, Colored::VariableInterval> *varIntervals) const override {
                std::unordered_map<Colored::Variable *,std::vector<std::pair<uint32_t, int32_t>>>  varModifierMapLeft;
                std::unordered_map<Colored::Variable *,std::vector<std::pair<uint32_t, int32_t>>>  varModifierMapRight;
                uint32_t index = 0;
                std::set<Variable *> vars;
                std::unordered_map<std::string, std::set<uint32_t>> varLeftPositions, varRightPositions;
                std::string varName = varInterval->_variable->name;
                _left->getVariables(vars, varLeftPositions,  varModifierMapLeft, &index);
                _right->getVariables(vars, varRightPositions,  varModifierMapRight, &index);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;

                for (auto index : varLeftPositions[varName]) {
                    bool succes = _right->getVariableRestriction(index, &restrictionvector, &intervalSize);
                    int32_t modifier = 0;
                    for(auto idModifierPair : varModifierMapLeft[varInterval->_variable]){
                        if(idModifierPair.first == index){
                            modifier = idModifierPair.second;
                        }
                    }
                    if (!succes) {
                        //comparing vars
                        for(auto varPositions : varRightPositions) {
                            if(varPositions.second.find(index) != varPositions.second.end()) {
                                if(varIntervals->count(varPositions.first) == 0){
                                    continue;
                                }
                                auto otherVar = &varIntervals->operator[](varPositions.first);
                                if(otherVar->size() == 0) {
                                    continue;
                                }

                                auto rightMaxVec = otherVar->back().getUpperIds();

                                for(auto& interval : varInterval->_intervalTuple._intervals){
                                    if (interval.size() != rightMaxVec.size()) {
                                        std::cout << "Interval sizes of variables does not match";
                                    }
                                    for(uint32_t i = 0; i < interval.size(); i++) {
                                        //hope that other direction is handled by right side
                                        interval[i]._upper = std::min(interval[i]._upper, rightMaxVec[i]-1);
                                    }
                                }
                            }
                        }
                    } else if(restrictionvector.size() == 1) {
                        uint32_t value = restrictionvector.back() + modifier;
                        auto ctSize = varInterval->_variable->colorType->size();
                        uint32_t finalVal = value % ctSize;
                        for(auto& interval : varInterval->_intervalTuple._intervals){
                            if(interval.getFirst()._lower > finalVal -1){
                                interval.getFirst().invalidate();
                            } else {
                                interval.getFirst()._upper =  std::max(finalVal-1, interval.getFirst()._upper);
                            }
                        }
                        
                    } else if (restrictionvector.size() > 1){
                        auto ct = (Colored::ProductType *) varInterval->_variable->colorType;
                        auto constituentSizes = ct->getConstituentsSizes();
                        for (auto& interval : varInterval->_intervalTuple._intervals){
                            for(uint32_t i = 0; i < interval.size(); i++){
                                auto value = restrictionvector[i]+modifier;
                                uint32_t finalVal = value % constituentSizes[i];
                                if(interval[i]._lower > finalVal-1) {
                                    interval[i].invalidate();
                                } else {
                                    interval[i]._upper = std::min(finalVal-1, interval[i]._upper);
                                }
                            }
                        }
                    }
                }

                for(auto index : varRightPositions[varName]) {
                    bool sucess = _left->getVariableRestriction(index, &restrictionvector, &intervalSize);
                    int32_t modifier = 0;
                    for(auto idModifierPair : varModifierMapRight[varInterval->_variable]){
                        if(idModifierPair.first == index){
                            modifier = idModifierPair.second;
                        }
                    }
                    
                    if (!sucess) {
                        //comparing vars
                        for(auto varPositions : varLeftPositions) {
                            if(varPositions.second.find(index) != varPositions.second.end()) {
                                if(varIntervals->count(varPositions.first) == 0){
                                    continue;
                                }
                                auto otherVar = &varIntervals->operator[](varPositions.first);
                                if(otherVar->size() == 0) {
                                    continue;
                                }

                                auto leftMinVec = otherVar->operator[](0).getLowerIds();

                                for(auto& interval : varInterval->_intervalTuple._intervals){
                                    if (interval.size() != leftMinVec.size()) {
                                        std::cout << "Interval sizes of variables does not match";
                                    }
                                    for(uint32_t i = 0; i < interval.size(); i++) {
                                        //hope that other direction is handled by right side
                                        interval[i]._lower = std::max(interval[i]._lower, leftMinVec[i]+1);
                                    }
                                }
                            }
                        }
                    } else if(restrictionvector.size() == 1) {
                        int32_t value = restrictionvector.back() + modifier;
                        auto ctSize = varInterval->_variable->colorType->size();
                        uint32_t finalVal = value % ctSize;
                        for(auto& interval : varInterval->_intervalTuple._intervals){
                            if(interval.getFirst()._upper < finalVal + 1){
                                interval.getFirst().invalidate();
                            } else {
                                interval.getFirst()._lower =  std::max(finalVal+1, interval.getFirst()._lower);
                            }
                        }
                    } else if (restrictionvector.size() > 1) {
                        auto ct = (Colored::ProductType *) varInterval->_variable->colorType;
                        auto constituentSizes = ct->getConstituentsSizes();
                        for (auto& interval : varInterval->_intervalTuple._intervals){
                            for(uint32_t i = 0; i < interval.size(); i++){
                                int32_t value = restrictionvector[i]+modifier;
                                uint32_t finalVal = value % constituentSizes[i];
                                if(interval[i]._upper < finalVal+1) {
                                    interval[i].invalidate();
                                } else {
                                    interval[i]._lower = std::max(finalVal+1, interval[i]._lower);
                                }
                            }
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVar(Colored::VariableInterval *varInterval, std::unordered_map<std::string, Colored::VariableInterval> *varIntervals) const override {
                std::unordered_map<Colored::Variable *,std::vector<std::pair<uint32_t, int32_t>>>  varModifierMapLeft;
                std::unordered_map<Colored::Variable *,std::vector<std::pair<uint32_t, int32_t>>>  varModifierMapRight;
                uint32_t index = 0;
                std::set<Variable *> vars;
                std::unordered_map<std::string, std::set<uint32_t>> varLeftPositions, varRightPositions;
                std::string varName = varInterval->_variable->name;
                _left->getVariables(vars, varLeftPositions,  varModifierMapLeft, &index);
                _right->getVariables(vars, varRightPositions,  varModifierMapRight, &index);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;
                for (auto index : varLeftPositions[varName]) {
                    bool succes = _right->getVariableRestriction(index, &restrictionvector, &intervalSize);
                    int32_t modifier = 0;
                    for(auto idModifierPair : varModifierMapLeft[varInterval->_variable]){
                        if(idModifierPair.first == index){
                            modifier = idModifierPair.second;
                        }
                    }
                    
                    if(!succes) {
                        //comparing vars
                        for(auto varPositions : varRightPositions) {
                            if(varPositions.second.find(index) != varPositions.second.end()) {
                                if(varIntervals->count(varPositions.first) == 0){
                                    continue;
                                }
                                auto otherVar = &varIntervals->operator[](varPositions.first);
                                if(otherVar->size() == 0) {
                                    continue;
                                }

                                auto rightMinVec = otherVar->operator[](0).getLowerIds();

                                for(auto& interval : varInterval->_intervalTuple._intervals){
                                    if (interval.size() != rightMinVec.size()) {
                                        std::cout << "Interval sizes of variables does not match";
                                    }
                                    for(uint32_t i = 0; i < interval.size(); i++) {
                                        //hope that other direction is handled by right side
                                        interval[i]._lower = std::max(interval[i]._lower, rightMinVec[i]+1);
                                    }
                                }

                            }
                        }
                    } else if(restrictionvector.size() == 1) {
                        int32_t value = restrictionvector.back() + modifier;
                        auto ctSize = varInterval->_variable->colorType->size();
                        uint32_t finalVal = value % ctSize;
                        for(auto& interval : varInterval->_intervalTuple._intervals){
                            if(interval.getFirst()._upper < finalVal+1){
                                interval.getFirst().invalidate();
                            } else {
                                interval.getFirst()._lower =  std::max(finalVal+1, interval.getFirst()._lower);
                            }
                        }
                        
                    } else if (restrictionvector.size() > 1) {
                        auto ct = (Colored::ProductType *) varInterval->_variable->colorType;
                        auto constituentSizes = ct->getConstituentsSizes();
                        for (auto& interval : varInterval->_intervalTuple._intervals){
                            for(uint32_t i = 0; i < interval.size(); i++){
                                int32_t value = restrictionvector[i]+modifier;
                                uint32_t finalVal = value % constituentSizes[i];
                                if(interval[i]._upper < finalVal+1) {
                                    interval[i].invalidate();
                                } else {
                                    interval[i]._lower = std::max(finalVal+1, interval[i]._lower);
                                }
                            }
                        }
                    }
                }
                for (auto index : varRightPositions[varName]) {
                    bool succes = _left->getVariableRestriction(index, &restrictionvector, &intervalSize);
                    int32_t modifier = 0;
                    for(auto idModifierPair : varModifierMapRight[varInterval->_variable]){
                        if(idModifierPair.first == index){
                            modifier = idModifierPair.second;
                        }
                    }
                    
                    if (!succes) {
                        //comparing vars
                        for(auto varPositions : varLeftPositions) {
                            if(varPositions.second.find(index) != varPositions.second.end()) {
                                if(varIntervals->count(varPositions.first) == 0){
                                    continue;
                                }
                                auto otherVar = &varIntervals->operator[](varPositions.first);
                                if(otherVar->size() == 0) {
                                    continue;
                                }

                                auto leftMaxVec = otherVar->back().getUpperIds();

                                for(auto& interval : varInterval->_intervalTuple._intervals){
                                    if (interval.size() != leftMaxVec.size()) {
                                        std::cout << "Interval sizes of variables does not match";
                                    }
                                    for(uint32_t i = 0; i < interval.size(); i++) {
                                        //hope that other direction is handled by right side
                                        interval[i]._upper = std::min(interval[i]._upper, leftMaxVec[i]-1);
                                    }
                                }
                            }
                        }
                    } else if(restrictionvector.size() == 1) {
                        uint32_t value = restrictionvector.back() + modifier;
                        auto ctSize = varInterval->_variable->colorType->size();
                        uint32_t finalVal = value % ctSize;
                        for(auto& interval : varInterval->_intervalTuple._intervals){
                            if(finalVal == 0 || interval.getFirst()._lower > finalVal - 1){
                                interval.getFirst().invalidate();
                            } else {
                                interval.getFirst()._upper =  std::max(finalVal-1, interval.getFirst()._upper);
                            }
                        }
                        
                    } else if (restrictionvector.size() > 1){
                        auto ct = (Colored::ProductType *) varInterval->_variable->colorType;
                        auto constituentSizes = ct->getConstituentsSizes();
                        for (auto& interval : varInterval->_intervalTuple._intervals){
                            for(uint32_t i = 0; i < interval.size(); i++){
                                auto value = restrictionvector[i]+modifier;
                                uint32_t finalVal = value % constituentSizes[i];
                                if(finalVal == 0 || interval[i]._lower > finalVal-1) {
                                    interval[i].invalidate();
                                } else {
                                    interval[i]._upper = std::min(finalVal-1, interval[i]._upper);
                                }
                            }
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVar(Colored::VariableInterval *varInterval, std::unordered_map<std::string, Colored::VariableInterval> *varIntervals) const override {
                std::unordered_map<Colored::Variable *,std::vector<std::pair<uint32_t, int32_t>>>  varModifierMapLeft;
                std::unordered_map<Colored::Variable *,std::vector<std::pair<uint32_t, int32_t>>>  varModifierMapRight;
                uint32_t index = 0;
                std::set<Variable *> vars;
                std::unordered_map<std::string, std::set<uint32_t>> varLeftPositions, varRightPositions;
                std::string varName = varInterval->_variable->name;
                _left->getVariables(vars, varLeftPositions,  varModifierMapLeft, &index);
                _right->getVariables(vars, varRightPositions,  varModifierMapRight, &index);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;
                for (auto index : varLeftPositions[varName]) {
                    bool succes = _right->getVariableRestriction(index, &restrictionvector, &intervalSize);
                    int32_t modifier = 0;
                    for(auto idModifierPair : varModifierMapLeft[varInterval->_variable]){
                        if(idModifierPair.first == index){
                            modifier = idModifierPair.second;
                        }
                    }

                    if (!succes) {
                        //comparing vars
                        for(auto varPositions : varRightPositions) {
                            if(varPositions.second.find(index) != varPositions.second.end()) {
                                if(varIntervals->count(varPositions.first) == 0){
                                    continue;
                                }
                                auto otherVar = &varIntervals->operator[](varPositions.first);
                                if(otherVar->size() == 0) {
                                    continue;
                                }

                                auto rightMaxVec = otherVar->back().getUpperIds();

                                for(auto& interval : varInterval->_intervalTuple._intervals){
                                    if (interval.size() != rightMaxVec.size()) {
                                        std::cout << "Interval sizes of variables does not match";
                                    }
                                    for(uint32_t i = 0; i < interval.size(); i++) {
                                        //hope that other direction is handled by right side
                                        interval[i]._upper = std::min(interval[i]._upper, rightMaxVec[i]);
                                    }
                                }
                            }
                        }
                    } else if(restrictionvector.size() == 1) {
                        uint32_t value = restrictionvector.back() + modifier;
                        auto ctSize = varInterval->_variable->colorType->size();
                        uint32_t finalVal = value % ctSize;
                        for(auto& interval : varInterval->_intervalTuple._intervals){
                            if(interval.getFirst()._lower > finalVal ){
                                interval.getFirst().invalidate();
                            } else {
                                interval.getFirst()._upper =  std::max(finalVal, interval.getFirst()._upper);
                            }
                        }
                        
                    } else if (restrictionvector.size() > 1){
                        auto ct = (Colored::ProductType *) varInterval->_variable->colorType;
                        auto constituentSizes = ct->getConstituentsSizes();
                        for (auto& interval : varInterval->_intervalTuple._intervals){
                            for(uint32_t i = 0; i < interval.size(); i++){
                                auto value = restrictionvector[i]+modifier;
                                uint32_t finalVal = value % constituentSizes[i];
                                if(interval[i]._lower > finalVal) {
                                    interval[i].invalidate();
                                } else {
                                    interval[i]._upper = std::min(finalVal, interval[i]._upper);
                                }
                            }
                        }
                    }
                }
                for (auto index : varRightPositions[varName]) {
                    bool succes = _left->getVariableRestriction(index, &restrictionvector, &intervalSize);
                    int32_t modifier = 0;
                    for(auto idModifierPair : varModifierMapRight[varInterval->_variable]){
                        if(idModifierPair.first == index){
                            modifier = idModifierPair.second;
                        }
                    }
                    
                    if(!succes) {
                        //comparing vars
                        for(auto varPositions : varLeftPositions) {
                            if(varPositions.second.find(index) != varPositions.second.end()) {
                                if(varIntervals->count(varPositions.first) == 0){
                                    continue;
                                }
                                auto otherVar = &varIntervals->operator[](varPositions.first);
                                if(otherVar->size() == 0) {
                                    continue;
                                }

                                auto leftMinVec = otherVar->operator[](0).getLowerIds();

                                for(auto& interval : varInterval->_intervalTuple._intervals){
                                    if (interval.size() != leftMinVec.size()) {
                                        std::cout << "Interval sizes of variables does not match";
                                    }
                                    for(uint32_t i = 0; i < interval.size(); i++) {
                                        //hope that other direction is handled by right side
                                        interval[i]._lower = std::max(interval[i]._lower, leftMinVec[i]);
                                    }
                                }
                            }
                        }
                    } else if(restrictionvector.size() == 1) {
                        int32_t value = restrictionvector.back() + modifier;
                        auto ctSize = varInterval->_variable->colorType->size();
                        uint32_t finalVal = value % ctSize;
                        for(auto& interval : varInterval->_intervalTuple._intervals){
                            if(interval.getFirst()._upper < finalVal){
                                interval.getFirst().invalidate();
                            } else {
                                interval.getFirst()._lower =  std::max(finalVal, interval.getFirst()._lower);
                            }
                        }
                        
                    } else if (restrictionvector.size() > 1) {
                        auto ct = (Colored::ProductType *) varInterval->_variable->colorType;
                        auto constituentSizes = ct->getConstituentsSizes();
                        for (auto& interval : varInterval->_intervalTuple._intervals){
                            for(uint32_t i = 0; i < interval.size(); i++){
                                int32_t value = restrictionvector[i]+modifier;
                                uint32_t finalVal = value % constituentSizes[i];
                                if(interval[i]._upper < finalVal) {
                                    interval[i].invalidate();
                                } else {
                                    interval[i]._lower = std::max(finalVal, interval[i]._lower);
                                }
                            }
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVar(Colored::VariableInterval *varInterval, std::unordered_map<std::string, Colored::VariableInterval> *varIntervals) const override {
                std::unordered_map<Colored::Variable *,std::vector<std::pair<uint32_t, int32_t>>>  varModifierMapLeft;
                std::unordered_map<Colored::Variable *,std::vector<std::pair<uint32_t, int32_t>>>  varModifierMapRight;
                uint32_t index = 0;
                std::set<Variable *> vars;
                std::unordered_map<std::string, std::set<uint32_t>> varLeftPositions, varRightPositions;
                std::string varName = varInterval->_variable->name;
                _left->getVariables(vars, varLeftPositions,  varModifierMapLeft, &index);
                _right->getVariables(vars, varRightPositions,  varModifierMapRight, &index);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;
                for (auto index : varLeftPositions[varName]) {
                    bool succes = _right->getVariableRestriction(index, &restrictionvector, &intervalSize);
                    int32_t modifier = 0;
                    for(auto idModifierPair : varModifierMapLeft[varInterval->_variable]){
                        if(idModifierPair.first == index){
                            modifier = idModifierPair.second;
                        }
                    }
                    
                    if (!succes) {
                        //comparing vars
                        for(auto varPositions : varRightPositions) {
                            if(varPositions.second.find(index) != varPositions.second.end()) {
                                if(varIntervals->count(varPositions.first) == 0){
                                    continue;
                                }
                                auto otherVar = &varIntervals->operator[](varPositions.first);
                                if(otherVar->size() == 0) {
                                    continue;
                                }

                                auto rightMinVec = otherVar->operator[](0).getLowerIds();

                                for(auto& interval : varInterval->_intervalTuple._intervals){
                                    if (interval.size() != rightMinVec.size()) {
                                        std::cout << "Interval sizes of variables does not match";
                                    }
                                    for(uint32_t i = 0; i < interval.size(); i++) {
                                        //hope that other direction is handled by right side
                                        interval[i]._lower = std::max(interval[i]._lower, rightMinVec[i]);
                                    }
                                }

                            }
                        }
                    }else if(restrictionvector.size() == 1) {
                        int32_t value = restrictionvector.back() + modifier;
                        auto ctSize = varInterval->_variable->colorType->size();
                        uint32_t finalVal = value % ctSize;
                        for(auto& interval : varInterval->_intervalTuple._intervals){
                            if(interval.getFirst()._upper < finalVal){
                                interval.getFirst().invalidate();
                            } else {
                                interval.getFirst()._lower =  std::max(finalVal, interval.getFirst()._lower);
                            }
                        }
                        
                    } else if (restrictionvector.size() > 1) {
                        auto ct = (Colored::ProductType *) varInterval->_variable->colorType;
                        auto constituentSizes = ct->getConstituentsSizes();
                        for (auto& interval : varInterval->_intervalTuple._intervals){
                            for(uint32_t i = 0; i < interval.size(); i++){
                                int32_t value = restrictionvector[i]+modifier;
                                uint32_t finalVal = value % constituentSizes[i];
                                if(interval[i]._upper < finalVal) {
                                    interval[i].invalidate();
                                } else {
                                    interval[i]._lower = std::max(finalVal, interval[i]._lower);
                                }
                            }
                        }
                    }
                }
                for (auto index : varRightPositions[varName]) {
                    bool succes = _left->getVariableRestriction(index, &restrictionvector, &intervalSize);
                    int32_t modifier = 0;
                    for(auto idModifierPair : varModifierMapRight[varInterval->_variable]){
                        if(idModifierPair.first == index){
                            modifier = idModifierPair.second;
                        }
                    }
                    
                    if (!succes) {
                        //comparing vars
                        for(auto varPositions : varLeftPositions) {
                            if(varPositions.second.find(index) != varPositions.second.end()) {
                                if(varIntervals->count(varPositions.first) == 0){
                                    continue;
                                }
                                auto otherVar = &varIntervals->operator[](varPositions.first);
                                if(otherVar->size() == 0) {
                                    continue;
                                }

                                auto leftMaxVec = otherVar->back().getUpperIds();

                                for(auto& interval : varInterval->_intervalTuple._intervals){
                                    if (interval.size() != leftMaxVec.size()) {
                                        std::cout << "Interval sizes of variables does not match";
                                    }
                                    for(uint32_t i = 0; i < interval.size(); i++) {
                                        //hope that other direction is handled by right side
                                        interval[i]._upper = std::min(interval[i]._upper, leftMaxVec[i]);
                                    }
                                }
                            }
                        }
                    } else if(restrictionvector.size() == 1) {
                        uint32_t value = restrictionvector.back() + modifier;
                        auto ctSize = varInterval->_variable->colorType->size();
                        uint32_t finalVal = value % ctSize;
                        for(auto& interval : varInterval->_intervalTuple._intervals){
                            if(interval.getFirst()._lower > finalVal){
                                interval.getFirst().invalidate();
                            } else {
                                interval.getFirst()._upper =  std::max(finalVal, interval.getFirst()._upper);
                            }
                        }
                        
                    } else if (restrictionvector.size() > 1){
                        auto ct = (Colored::ProductType *) varInterval->_variable->colorType;
                        auto constituentSizes = ct->getConstituentsSizes();
                        for (auto& interval : varInterval->_intervalTuple._intervals){
                            for(uint32_t i = 0; i < interval.size(); i++){
                                auto value = restrictionvector[i]+modifier;
                                uint32_t finalVal = value % constituentSizes[i];
                                if(interval[i]._lower > finalVal) {
                                    interval[i].invalidate();
                                } else {
                                    interval[i]._upper = std::min(finalVal, interval[i]._upper);
                                }
                            }
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVar(Colored::VariableInterval *varInterval, std::unordered_map<std::string, Colored::VariableInterval> *varIntervals) const override {
                std::unordered_map<Colored::Variable *,std::vector<std::pair<uint32_t, int32_t>>>  varModifierMapLeft;
                std::unordered_map<Colored::Variable *,std::vector<std::pair<uint32_t, int32_t>>>  varModifierMapRight;
                uint32_t index = 0;
                std::set<Variable *> vars;
                std::unordered_map<std::string, std::set<uint32_t>> varLeftPositions, varRightPositions;
                std::string varName = varInterval->_variable->name;
                _left->getVariables(vars, varLeftPositions,  varModifierMapLeft, &index);
                _right->getVariables(vars, varRightPositions,  varModifierMapRight, &index);
                uint32_t intervalSize = 0;

                std::vector<uint32_t> restrictionvector;
                for (auto index : varLeftPositions[varName]) {
                    bool succes = _right->getVariableRestriction(index, &restrictionvector, &intervalSize);
                    int32_t modifier = 0;
                    for(auto idModifierPair : varModifierMapLeft[varInterval->_variable]){
                        if(idModifierPair.first == index){
                            modifier = idModifierPair.second;
                        }
                    }

                    if (!succes) {
                        //comparing vars;
                        for(auto varPositions : varRightPositions) {
                            if(varPositions.second.find(index) != varPositions.second.end()) {
                                if(varIntervals->count(varPositions.first) == 0){
                                    continue;
                                }

                                auto otherVar = &varIntervals->operator[](varPositions.first);
                                if(otherVar->size() == 0) {
                                    continue;
                                }

                                for(auto& interval : varInterval->_intervalTuple._intervals){
                                    for(auto& otherInterval : otherVar->_intervalTuple._intervals){
                                        for(uint32_t j = 0; j < interval.size(); j++){
                                            Reachability::range_t oldRange = interval[j];
                                            Reachability::range_t otherOldRange = otherInterval[j];
                                            interval[j] &= otherInterval[j];
                                            otherInterval[j] &= interval[j];

                                            if(varInterval->size() == 1 && !interval[j].isSound()){
                                                interval[j] = oldRange;
                                            }
                                            if(otherInterval.size() == 1 && !otherInterval[j].isSound()){
                                                otherInterval[j] = otherOldRange;
                                            }
                                        }
                                    }
                                }
                                varInterval->_intervalTuple.mergeIntervals();
                                otherVar->_intervalTuple.mergeIntervals();
                            }
                        }
                        
                    } else if(restrictionvector.size() == 1) {
                        auto ctSize = varInterval->_variable->colorType->size();
                        auto value = restrictionvector.back() + modifier;
                        auto finalVal = value % ctSize;
                        for(auto& interval : varInterval->_intervalTuple._intervals){
                            if(interval.getFirst().contains(finalVal)){
                                interval.getFirst() &= finalVal;
                            } else {
                                interval.getFirst().invalidate();
                            }                            
                        }
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        auto ct = (Colored::ProductType *) varInterval->_variable->colorType;
                        auto constituentSizes = ct->getConstituentsSizes();
                        for (auto& interval : varInterval->_intervalTuple._intervals){
                            for(uint32_t i = 0; i < interval.size(); i++){
                                auto value = restrictionvector[i]+modifier;
                                auto finalVal = value % constituentSizes[i];
                                if(interval[i].contains(finalVal)){
                                    interval[i] &= finalVal;
                                } else {
                                    interval[i].invalidate();
                                    break;
                                }                                
                            }
                        }
                    }
                }
                for (auto index : varRightPositions[varName]) {
                    bool succes = _left->getVariableRestriction(index, &restrictionvector, &intervalSize);
                    int32_t modifier = 0;
                    for(auto idModifierPair : varModifierMapRight[varInterval->_variable]){
                        if(idModifierPair.first == index){
                            modifier = idModifierPair.second;
                        }
                    }

                    if (!succes) {
                        //comparing vars
                        for(auto varPositions : varLeftPositions) {
                            if(varPositions.second.find(index) != varPositions.second.end()) {
                                if(varIntervals->count(varPositions.first) == 0){
                                    continue;
                                }
                                auto otherVar = &varIntervals->operator[](varPositions.first);
                                if(otherVar->size() == 0) {
                                    continue;
                                }

                                for(auto& interval : varInterval->_intervalTuple._intervals){
                                    for(auto& otherInterval : otherVar->_intervalTuple._intervals){
                                        for(uint32_t j = 0; j < interval.size(); j++){
                                            Reachability::range_t oldRange = interval[j];
                                            Reachability::range_t otherOldRange = otherInterval[j];
                                            interval[j] &= otherInterval[j];
                                            otherInterval[j] &= interval[j];

                                            if(!interval[j].isSound()){
                                                interval[j] = oldRange;
                                            }
                                            if(!otherInterval[j].isSound()){
                                                otherInterval[j] = otherOldRange;
                                            }
                                        }
                                    }
                                }
                                varInterval->_intervalTuple.mergeIntervals();
                                otherVar->_intervalTuple.mergeIntervals();

                            }
                        }
                    } else if(restrictionvector.size() == 1) {
                        auto ctSize = varInterval->_variable->colorType->size();
                        auto value = restrictionvector.back() + modifier;
                        auto finalVal = value % ctSize;
                        for(auto& interval : varInterval->_intervalTuple._intervals){
                            if(interval.getFirst().contains(finalVal)){
                                interval.getFirst() &= finalVal;
                            } else {
                                interval.getFirst().invalidate();
                            }                           
                        }
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        auto ct = (Colored::ProductType *) varInterval->_variable->colorType;
                        auto constituentSizes = ct->getConstituentsSizes();
                        for (auto& interval : varInterval->_intervalTuple._intervals){
                            for(uint32_t i = 0; i < interval.size(); i++){
                                auto value = restrictionvector[i]+modifier;
                                auto finalVal = value % constituentSizes[i];
                                if(interval[i].contains(finalVal)){
                                    interval[i] &= finalVal;
                                } else {
                                    interval[i].invalidate();
                                    break;
                                }                                 
                            }
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVar(Colored::VariableInterval *varInterval, std::unordered_map<std::string, Colored::VariableInterval> *varIntervals) const override {
                /*int32_t leftModifier = 0, rightmodifier = 0;
                int32_t varIndexLeft = _left->getVariableIndex(varInterval->varaible, &leftModifier);
                int32_t varIndexRight = _right->getVariableIndex(varInterval->varaible, &rightmodifier);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;
                if(varIndexLeft > -1) {
                    bool succes = _right->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        //comparing vars
                    } else if(restrictionvector.size() == 1) {
                        if (restrictionvector.back() == varInterval->interval_lower) {
                            varInterval->interval_lower += 1 + leftModifier;                            
                        } else if (restrictionvector.back() == varInterval->interval_upper) {
                            varInterval->interval_upper += -1 + leftModifier;
                        } else {
                            varInterval->interval_lower = 0;
                            varInterval->interval_upper = varInterval->varaible->colorType->size()-1;
                        }
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                    }
                }
                if (varIndexRight > -1) {
                    bool succes = _left->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        //comparing vars
                    } else if(restrictionvector.size() == 1) {
                        if(restrictionvector.size() == 1) {
                            if (restrictionvector.back() == varInterval->interval_lower) {
                                varInterval->interval_lower++;                            
                            } else if (restrictionvector.back() == varInterval->interval_upper) {
                                varInterval->interval_upper--;
                            } else {
                                varInterval->interval_lower = 0;
                                varInterval->interval_upper = varInterval->varaible->colorType->size()-1;
                            }
                        }
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                    }
                }
                */
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _expr->getVariables(variables, varPositions, varModifierMap);
            }

            std::string toString() const override {
                std::string res = "!" + _expr->toString();
                return res;
            }

            void restrictVar(Colored::VariableInterval *varInterval, std::unordered_map<std::string, Colored::VariableInterval> *varIntervals) const override {
                Reachability::intervalTuple_t fullInterval;
                Reachability::interval_t interval = varInterval->_variable->colorType->getFullInterval();
                
                fullInterval.addInterval(interval);
                
                varInterval->_intervalTuple = fullInterval;
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVar(Colored::VariableInterval *varInterval,std::unordered_map<std::string, Colored::VariableInterval> *varIntervals) const override {
                _left->restrictVar(varInterval, varIntervals);
                _right->restrictVar(varInterval, varIntervals);
            }
            
            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                _left->getPatterns(patterns, colorTypes);
                _right->getPatterns(patterns, colorTypes);
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            void restrictVar(Colored::VariableInterval *varInterval, std::unordered_map<std::string, Colored::VariableInterval> *varIntervals) const override {
                Colored::VariableInterval varIntervalCopy = Colored::VariableInterval {varInterval->_variable, varInterval->_intervalTuple};
                _left->restrictVar(varInterval, varIntervals);
                _right->restrictVar(&varIntervalCopy, varIntervals);


                for(auto copyInterval : varIntervalCopy._intervalTuple._intervals){
                    varInterval->_intervalTuple.addInterval(copyInterval);
                }
                
                varInterval->_intervalTuple.mergeIntervals();
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
            virtual void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const = 0;

            virtual uint32_t weight() const = 0;
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

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const {
                for (size_t i = 0; i < _sort->size(); i++) {
                    constantMap[&(*_sort)[i]].push_back(index);
                }
            }

            size_t size() const {
                return  _sort->size();
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                std::set<Variable*> variables = {};
                Colored::Pattern pattern (
                    Colored::PatternType::Constant,
                    this,
                    variables,
                    _sort
                );
                patterns.insert(pattern);
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

            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                if (_all != nullptr)
                    return;
                for (auto elem : _color) {
                    //TODO: can there be more than one element in a number of expression?
                    elem->getVariables(variables, varPositions, varModifierMap);
                }
            }

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, Colored::VariableInterval> *varIntervals, std::vector<Colored::ColorType *> *colortypes) const override {
                Reachability::intervalTuple_t intervals;
                if (_all == nullptr) {
                    for (auto elem : _color) {
                        auto nestedIntervals = elem->getOutputIntervals(varIntervals, colortypes);

                        if (intervals._intervals.empty()) {
                            intervals = nestedIntervals;
                        } else {
                            for(auto interval : nestedIntervals._intervals) {
                                intervals.addInterval(interval);
                            }
                        }
                    }
                }
                intervals.mergeIntervals();
                return intervals;

            }

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const override {
                if (_all != nullptr)
                    _all->getConstants(constantMap, index);
                else for (auto elem : _color) {
                    elem->getConstants(constantMap, index);
                    index++;//not sure if index should be increased here, but no number expression have multiple elements
                }
            }
            
            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                if(_all != nullptr){
                    _all->getPatterns(patterns,colorTypes);
                } else{
                    for (auto elem : _color) {
                        elem->getPatterns(patterns, colorTypes);
                    }
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                for (auto elem : _constituents) {
                    elem->getVariables(variables, varPositions, varModifierMap);
                }
            }            

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, Colored::VariableInterval> *varIntervals, std::vector<Colored::ColorType *> *colortypes) const override {
                Reachability::intervalTuple_t intervals;
                
                for (auto elem : _constituents) {
                    auto nestedIntervals = elem->getOutputIntervals(varIntervals, colortypes);

                    if (intervals._intervals.empty()) {
                        intervals = nestedIntervals;
                    } else {
                        for(auto interval : nestedIntervals._intervals) {
                            intervals.addInterval(interval);
                        }
                    }

                }
                intervals.mergeIntervals();
                return intervals;
            }

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const override {
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

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                for (auto elem : _constituents) {
                    elem->getPatterns(patterns, colorTypes);
                }
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _left->getVariables(variables, varPositions, varModifierMap);
                _right->getVariables(variables, varPositions, varModifierMap);
            }

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, Colored::VariableInterval> *varIntervals, std::vector<Colored::ColorType *> *colortypes) const override {
                //We could maybe reduce the intervals slightly by checking if the upper or lower bound is being subtracted
                auto leftIntervals = _left->getOutputIntervals(varIntervals, colortypes);

                return leftIntervals;
            }   

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const override {
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
            
            void getVariables(std::set<Variable*>& variables, std::unordered_map<std::string, std::set<uint32_t>>& varPositions, std::unordered_map<Variable *,std::vector<std::pair<uint32_t, int32_t>>>& varModifierMap, uint32_t *index) const override {
                _expr->getVariables(variables, varPositions,varModifierMap);
            }

            Reachability::intervalTuple_t getOutputIntervals(std::unordered_map<std::string, Colored::VariableInterval> *varIntervals, std::vector<Colored::ColorType *> *colortypes) const override {
                return _expr->getOutputIntervals(varIntervals, colortypes);
            }

            void getConstants(std::unordered_map<const Color*, std::vector<uint32_t>> &constantMap, uint32_t &index) const override {
                _expr->getConstants(constantMap, index);
            }

            uint32_t weight() const override {
                return _scalar * _expr->weight();
            }

            std::string toString() const override {
                return std::to_string(_scalar) + " * " + _expr->toString();
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes){

                _expr->getPatterns(patterns, colorTypes);
            }

            ScalarProductExpression(ArcExpression_ptr&& expr, uint32_t scalar)
                    : _scalar(std::move(scalar)), _expr(expr) {}
        };
    }
}

#endif /* COLORED_EXPRESSIONS_H */

