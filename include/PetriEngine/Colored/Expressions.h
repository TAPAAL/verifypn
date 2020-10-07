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
            
            virtual void getVariables(std::set<Variable*>& variables) const {
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

            virtual std::set<const Color*> getConstants() const = 0;

            virtual int32_t getVariableIndex(Variable * variable) const = 0;

            virtual bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const = 0;

            virtual ColorType* getColorType(std::unordered_map<std::string,Colored::ColorType*>& colorTypes) const = 0;
        };
        
        class DotConstantExpression : public ColorExpression {
        public:
            const Color* eval(ExpressionContext& context) const override {
                return DotConstant::dotConstant();
            }

            int32_t getVariableIndex(Variable * variable) const override {
                return -1;
            }

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                restrictionValue->emplace_back(0);
                *intervalSize = 0;
                return true;
            }

            std::set<const Color*> getConstants() const override {
                std::set<const Color*> colors;
                const Color *dotColor = DotConstant::dotConstant();
                colors.insert(dotColor);
                return colors;
            }
            ColorType* getColorType(std::unordered_map<std::string,Colored::ColorType*>& colorTypes) const override{
                return DotConstant::dotConstant()->getColorType();
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

            int32_t getVariableIndex(Variable * variable) const override {
                return 0;
            }

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                return false;
            }
            
            void getVariables(std::set<Variable*>& variables) const override {
                variables.insert(_variable);
            }

            std::set<const Color*> getConstants() const override {
                std::set<const Color*> colors;
                //Find a way to find bindings with computing all bindings
                //not implemented, will give problems if vars in product type

                return colors;
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                Colored::Pattern pattern(
                    Colored::PatternType::Var,
                    this,
                    {_variable},
                    getColorType(colorTypes)
                );
                pattern.toString();
                std::pair<std::set<Colored::Pattern>::iterator, bool> res = patterns.insert(pattern);
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

            int32_t getVariableIndex(Variable * variable) const override {
                return -1;
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

            std::set<const Color*> getConstants() const override {
                std::set<const Color*> colors;
                colors.insert(_userOperator);
                return colors;
            }
            ColorType* getColorType(std::unordered_map<std::string,Colored::ColorType*>& colorTypes) const override{
                return _userOperator->getColorType();
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _color->getVariables(variables);
            }

            int32_t getVariableIndex(Variable * variable) const override {
                return _color->getVariableIndex(variable);
            }

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                if (_color->getVariableRestriction(index, restrictionValue, intervalSize)){
                    auto lastElement = restrictionValue->back();
                    if(lastElement < (*intervalSize) -1) {
                        lastElement++;
                    } else {
                        lastElement = 0;
                    }
                    return true;
                }
                return false;
            }

            std::set<const Color*> getConstants() const override {
                return _color->getConstants();
            }
            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                /*std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    variables.empty() ? Colored::PatternType::Constant : Colored::PatternType::Var,
                    SuccessorExpression(_color),
                    variables,
                    nullptr
                );
                patterns.insert(pattern);*/

                _color->getPatterns(patterns, colorTypes);
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _color->getVariables(variables);
            }

            int32_t getVariableIndex(Variable * variable) const override {
                return _color->getVariableIndex(variable);
            }

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                if (_color->getVariableRestriction(index, restrictionValue, intervalSize)){
                    
                    auto lastElement = restrictionValue->back();
                    if(lastElement < (*intervalSize) -1) {
                        lastElement--;
                    } else {
                        lastElement = *intervalSize;
                    }
                    return true;
                }
                return false;
            }

            std::set<const Color*> getConstants() const override {
                return _color->getConstants();
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

            int32_t getVariableIndex(Variable * variable) const override {
                int32_t index = 0;
                for(auto colorExp : _colors) {
                    int32_t nested_index = colorExp->getVariableIndex(variable);
                    if ( nested_index > -1) {
                        return nested_index + index;
                    }
                    index++;
                }
                return -1;
            }
            
            void getVariables(std::set<Variable*>& variables) const override {
                for (auto elem : _colors) {
                    elem->getVariables(variables);
                }
            }

            bool getVariableRestriction(int32_t index, std::vector<uint32_t> * restrictionValue, uint32_t *intervalSize) const override {
                
                if (index == 0) {
                    for (auto colorExp : _colors) {
                        bool succes = colorExp->getVariableRestriction(0, restrictionValue, intervalSize);
                    }
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
            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    Colored::PatternType::Tuple,
                    this,
                    variables,
                    //TODO: how to retrieve???
                    _colorType
                );
                patterns.insert(pattern);
            }

            std::set<const Color*> getConstants() const override {
                std::set<std::set<const Color*>> tupleColorsSet;
                std::set<const Color*> colors;
                for (auto elem : _colors) {
                    std::set<const Color*> elemColors = elem->getConstants();
                    if (elemColors.size() > 1) {
                       std::set<std::set<const Color*>> tupleColorsSubsets;
                        for (auto color : elemColors) {
                            std::set<std::set<const Color*>> tupleColorsSubset = tupleColorsSet;
                            for (auto tupleColors: tupleColorsSubset) {
                                tupleColors.insert(color);
                            }
                            tupleColorsSubsets.insert(tupleColorsSubset.begin(), tupleColorsSubset.end());
                        }
                        tupleColorsSet.insert(tupleColorsSubsets.begin(), tupleColorsSubsets.end());

                    } else {
                        for (auto tupleColors: tupleColorsSet){
                            tupleColors.insert(elemColors.begin(), elemColors.end());
                        }                        
                    }
                }

                for (auto tupleColors : tupleColorsSet ){

                    std::vector<const Color*> tupleColorsVec(tupleColors.begin(), tupleColors.end());
                    const Color* productColor = new Color(_colorType, _colorType->getId(), tupleColorsVec);
                    colors.insert(productColor);
                }            

                tupleColorsSet.clear();               

                return colors;
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

            virtual void restrictVar(Variable * var) const = 0;
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }

            void restrictVar(Variable * var) const override {
                int32_t varIndexLeft = _left->getVariableIndex(var);
                int32_t varIndexRight = _right->getVariableIndex(var);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;
                if(varIndexLeft > -1) {
                    bool succes = _right->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        //comparing vars
                    } else if(restrictionvector.size() == 1) {
                        var->interval_upper = restrictionvector.back()-1;
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }
                }
                if (varIndexRight > -1) {
                    bool sucess = _left->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!sucess) {
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    } else if(restrictionvector.size() == 1) {
                        var->interval_lower = restrictionvector.back()+1;
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }
                }
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    Colored::PatternType::Guard,
                    this,
                    variables,
                    nullptr
                );
                patterns.insert(pattern);
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }

            void restrictVar(Variable * var) const override {
                int32_t varIndexLeft = _left->getVariableIndex(var);
                int32_t varIndexRight = _right->getVariableIndex(var);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;
                if(varIndexLeft > -1) {
                    bool succes = _right->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if(!succes) {
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }else if(restrictionvector.size() == 1) {
                        var->interval_lower = restrictionvector.back()+1;
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }
                }
                if (varIndexRight > -1) {
                    bool succes = _left->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        //comparing vars
                    } else if(restrictionvector.size() == 1) {
                        var->interval_upper = restrictionvector.back()-1;
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }
                }
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    Colored::PatternType::Guard,
                    this,
                    variables,
                    nullptr
                );
                patterns.insert(pattern);
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{


                std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    Colored::PatternType::Guard,
                    this,
                    variables,
                    nullptr
                );
                patterns.insert(pattern);
            }

            void restrictVar(Variable * var) const override {
                int32_t varIndexLeft = _left->getVariableIndex(var);
                int32_t varIndexRight = _right->getVariableIndex(var);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;
                if(varIndexLeft > -1) {
                    bool succes = _right->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        //comparing vars
                    } else if(restrictionvector.size() == 1) {
                        var->interval_upper = restrictionvector.back();
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }
                }
                if (varIndexRight > -1) {
                    bool succes = _left->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if(!succes) {
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    } else if(restrictionvector.size() == 1) {
                        var->interval_lower = restrictionvector.back();
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    Colored::PatternType::Guard,
                    this,
                    variables,
                    nullptr
                );
                patterns.insert(pattern);
            }

            void restrictVar(Variable * var) const override {
                int32_t varIndexLeft = _left->getVariableIndex(var);
                int32_t varIndexRight = _right->getVariableIndex(var);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;
                if(varIndexLeft > -1) {
                    bool succes = _right->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        //comparing vars
                    } else if(restrictionvector.size() == 1) {
                        var->interval_lower = restrictionvector.back();
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }
                }
                if (varIndexRight > -1) {
                    bool succes = _left->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    } else if(restrictionvector.size() == 1) {
                        var->interval_upper = restrictionvector.back();
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }

            void restrictVar(Variable * var) const override {
                int32_t varIndexLeft = _left->getVariableIndex(var);
                int32_t varIndexRight = _right->getVariableIndex(var);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;
                if(varIndexLeft > -1) {
                    bool succes = _right->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        //comparing vars;
                    } else if(restrictionvector.size() == 1) {
                        var->interval_upper = var->interval_lower = restrictionvector.back();
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }
                }
                if (varIndexRight > -1) {
                    bool succes = _left->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        //comparing vars
                    } else if(restrictionvector.size() == 1) {
                        var->interval_upper = var->interval_lower = restrictionvector.back();
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }
                }
            }
            
            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    Colored::PatternType::Guard,
                    this,
                    variables,
                    nullptr
                );
                patterns.insert(pattern);
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    Colored::PatternType::Guard,
                    this,
                    variables,
                    nullptr
                );
                patterns.insert(pattern);
            }

            void restrictVar(Variable * var) const override {
                int32_t varIndexLeft = _left->getVariableIndex(var);
                int32_t varIndexRight = _right->getVariableIndex(var);
                uint32_t intervalSize = 0;
                std::vector<uint32_t> restrictionvector;
                if(varIndexLeft > -1) {
                    bool succes = _right->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        //comparing vars
                    } else if(restrictionvector.size() == 1) {
                        if (restrictionvector.back() == var->interval_lower) {
                            var->interval_lower++;                            
                        } else if (restrictionvector.back() == var->interval_upper) {
                            var->interval_upper--;
                        } else {
                            var->interval_lower = 0;
                            var->interval_upper = var->colorType->size()-1;
                        }
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }
                }
                if (varIndexRight > -1) {
                    bool succes = _left->getVariableRestriction(varIndexLeft, &restrictionvector, &intervalSize);
                    if (!succes) {
                        //comparing vars
                    } else if(restrictionvector.size() == 1) {
                        if(restrictionvector.size() == 1) {
                            if (restrictionvector.back() == var->interval_lower) {
                                var->interval_lower++;                            
                            } else if (restrictionvector.back() == var->interval_upper) {
                                var->interval_upper--;
                            } else {
                                var->interval_lower = 0;
                                var->interval_upper = var->colorType->size()-1;
                            }
                        }
                    } else if (restrictionvector.size() > 1) {
                        //handle tuple vars
                        var->interval_lower = 0;
                        var->interval_upper = var->colorType->size()-1;
                    }
                }
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _expr->getVariables(variables);
            }

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    Colored::PatternType::Guard,
                    this,
                    variables,
                    //TODO: how to retrieve???
                    nullptr
                );
                patterns.insert(pattern);
            }

            std::string toString() const override {
                std::string res = "!" + _expr->toString();
                return res;
            }

            void restrictVar(Variable * var) const override {
                var->interval_lower = 0;
                var->interval_upper = var->colorType->size()-1;
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }

            void restrictVar(Variable * var) const override {
                _left->restrictVar(var);
                _right->restrictVar(var);
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }

            void restrictVar(Variable * var) const override {
                var->interval_lower = 0;
                var->interval_upper = var->colorType->size()-1;
            }
            
            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{

                std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    Colored::PatternType::Guard,
                    this,
                    variables,
                    nullptr
                );
                patterns.insert(pattern);
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
            virtual std::set<const Color*> getConstants() const = 0;

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

            std::set<const Color*> getConstants() const {
                std::set<const Color*> colors;
                for(auto color : *_sort) {
                    colors.insert(&color);
                }
                return colors;
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                if (_all != nullptr)
                    return;
                for (auto elem : _color) {
                    elem->getVariables(variables);
                }
            }

            std::set<const Color*> getConstants() const override {
                std::set<const Color*> colors;
                if (_all != nullptr)
                    return _all->getConstants();
                else for (auto elem : _color) {
                    auto elemColors = elem->getConstants();
                    colors.insert(elemColors.begin(), elemColors.end());
                }
                return colors;
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                for (auto elem : _constituents) {
                    elem->getVariables(variables);
                }
            }

            std::set<const Color*> getConstants() const override {
                std::set<const Color*> colors;
                for (auto elem : _constituents) {
                    auto subColors = elem->getConstants();
                    colors.insert(subColors.begin(), subColors.end());
                }

                return colors;
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }

            std::set<const Color*> getConstants() const override {
                std::set<const Color*> colors;
                colors.insert(_left->getConstants().begin(), _left->getConstants().end());
                colors.insert(_right->getConstants().begin(), _right->getConstants().end());

                return colors;
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

            void getPatterns(PatternSet& patterns, std::unordered_map<std::string, Colored::ColorType*>& colorTypes) const override{
                std::set<Variable*> variables = {};
                getVariables(variables);
                Colored::Pattern pattern (
                    variables.empty() ? Colored::PatternType::Constant : Colored::PatternType::Var,
                    this,
                    variables,
                    nullptr
                );
                patterns.insert(pattern);
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
            
            void getVariables(std::set<Variable*>& variables) const override {
                _expr->getVariables(variables);
            }

            std::set<const Color*> getConstants() const override {
                return _expr->getConstants();
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

