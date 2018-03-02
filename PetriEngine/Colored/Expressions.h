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


#include "Colors.h"
#include "Multiset.h"

namespace PetriEngine {
    class ColoredPetriNetBuilder;
    
    namespace Colored {
        struct ExpressionContext {
            std::unordered_map<std::string, const Color*>& binding;
            std::unordered_map<std::string, ColorType*>& colorTypes;
            
            const Color* findColor(const std::string& color) const {
                if (color.compare("dot") == 0)
                    return DotConstant::dotConstant();
                for (auto elem : colorTypes) {
                    try {
                        return &(*elem.second)[color];
                    } catch (...) {}
                }
                printf("Could not find color: %s\nCANNOT_COMPUTE\n", color.c_str());
                exit(-1);
            }
        };
        
        class Expression {
        public:
            Expression() {}
            
            virtual void getVariables(std::set<Variable*>& variables) {
                std::cout << "Calling unimplemented getVariables()" << std::endl;
            }
            
            virtual void expressionType() {
                std::cout << "Expression" << std::endl;
            }
        };
        
        class ColorExpression : public Expression {
        public:
            ColorExpression() {}
            virtual ~ColorExpression() {}
            
            virtual const Color* eval(ExpressionContext& context) const = 0;
        };
        
        class DotConstantExpression : public ColorExpression {
        public:
            const Color* eval(ExpressionContext& context) const override {
                return DotConstant::dotConstant();
            }
        };
        
        class VariableExpression : public ColorExpression {
        private:
            Variable* _variable;
            
        public:
            const Color* eval(ExpressionContext& context) const override {
                return context.binding[_variable->name];
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                variables.insert(_variable);
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
            
            UserSortExpression(ColorType* userSort)
                    : _userSort(userSort) {}
        };
        
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
        
        class SuccessorExpression : public ColorExpression {
        private:
            ColorExpression* _color;
            
        public:
            const Color* eval(ExpressionContext& context) const override {
                return &++(*_color->eval(context));
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _color->getVariables(variables);
            }
            
            SuccessorExpression(ColorExpression* color)
                    : _color(color) {}
        };
        
        class PredecessorExpression : public ColorExpression {
        private:
            ColorExpression* _color;
            
        public:
            const Color* eval(ExpressionContext& context) const override {
                return &--(*_color->eval(context));
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _color->getVariables(variables);
            }
            
            PredecessorExpression(ColorExpression* color)
                    : _color(color) {}
        };
        
        class TupleExpression : public ColorExpression {
        private:
            std::vector<ColorExpression*> _colors;
            
        public:
            const Color* eval(ExpressionContext& context) const override {
                std::vector<const Color*> colors;
                for (auto color : _colors) {
                    colors.push_back(color->eval(context));
                }
                return context.findColor(Color::toString(colors));
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                for (auto elem : _colors) {
                    elem->getVariables(variables);
                }
            }
            
            TupleExpression(std::vector<ColorExpression*> colors)
                    : _colors(colors) {}
        };
        
        class GuardExpression : public Expression {
        public:
            GuardExpression() {}
            virtual ~GuardExpression() {}
            
            virtual bool eval(ExpressionContext& context) const = 0;
        };
        
        class LessThanExpression : public GuardExpression {
        private:
            ColorExpression* _left;
            ColorExpression* _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) < _right->eval(context);
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }
            
            LessThanExpression(ColorExpression* left, ColorExpression* right)
                    : _left(left), _right(right) {}
        };
        
        class GreaterThanExpression : public GuardExpression {
        private:
            ColorExpression* _left;
            ColorExpression* _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) > _right->eval(context);
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }
            
            GreaterThanExpression(ColorExpression* left, ColorExpression* right)
                    : _left(left), _right(right) {}
        };
        
        class LessThanEqExpression : public GuardExpression {
        private:
            ColorExpression* _left;
            ColorExpression* _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) <= _right->eval(context);
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }
            
            LessThanEqExpression(ColorExpression* left, ColorExpression* right)
                    : _left(left), _right(right) {}
        };
        
        class GreaterThanEqExpression : public GuardExpression {
        private:
            ColorExpression* _left;
            ColorExpression* _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) >= _right->eval(context);
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }
            
            GreaterThanEqExpression(ColorExpression* left, ColorExpression* right)
                    : _left(left), _right(right) {}
        };
        
        class EqualityExpression : public GuardExpression {
        private:
            ColorExpression* _left;
            ColorExpression* _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) == _right->eval(context);
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }
            
            EqualityExpression(ColorExpression* left, ColorExpression* right)
                    : _left(left), _right(right) {}
        };
        
        class InequalityExpression : public GuardExpression {
        private:
            ColorExpression* _left;
            ColorExpression* _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) != _right->eval(context);
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }
            
            InequalityExpression(ColorExpression* left, ColorExpression* right)
                    : _left(left), _right(right) {}
        };
        
        class NotExpression : public GuardExpression {
        private:
            GuardExpression* _expr;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return !_expr->eval(context);
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _expr->getVariables(variables);
            }
            
            NotExpression(GuardExpression* expr) : _expr(expr) {}
        };
        
        class AndExpression : public GuardExpression {
        private:
            GuardExpression* _left;
            GuardExpression* _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) && _right->eval(context);
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }
            
            AndExpression(GuardExpression* left, GuardExpression* right)
                    : _left(left), _right(right) {}
        };
        
        class OrExpression : public GuardExpression {
        private:
            GuardExpression* _left;
            GuardExpression* _right;
            
        public:
            bool eval(ExpressionContext& context) const override {
                return _left->eval(context) || _right->eval(context);
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }
            
            OrExpression(GuardExpression* left, GuardExpression* right)
                    : _left(left), _right(right) {}
        };
        
        class ArcExpression : public Expression {
        public:
            ArcExpression() {}
            virtual ~ArcExpression() {}
            
            virtual Multiset eval(ExpressionContext& context) const = 0;
            virtual void getVariables(std::set<Variable*>& variables) override {}
            virtual void expressionType() override {
                std::cout << "ArcExpression" << std::endl;
            }

        };
        
        class AllExpression : public Expression {
        private:
            ColorType* _sort;
            
        public:
            std::vector<const Color*> eval(ExpressionContext& context) const {
                std::vector<const Color*> colors;
                assert(_sort != nullptr);
                for (size_t i = 0; i < _sort->size(); i++) {
                    colors.push_back(&(*_sort)[i]);
                }
                return colors;
            }
            
            AllExpression(ColorType* sort) : _sort(sort) 
            {
                assert(sort != nullptr);
            }
        };
        
        class NumberOfExpression : public ArcExpression {
        private:
            uint32_t _number;
            std::vector<ColorExpression*> _color;
            AllExpression* _all;
            
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
            
            void getVariables(std::set<Variable*>& variables) override {
                for (auto elem : _color) {
                    elem->getVariables(variables);
                }
            }
            
            NumberOfExpression(std::vector<ColorExpression*> color, uint32_t number = 1)
                    : _number(number), _color(color), _all(nullptr) {}
            NumberOfExpression(AllExpression* all, uint32_t number = 1)
                    : _number(number), _color(), _all(all) {}
        };
        
        class AddExpression : public ArcExpression {
        private:
            std::vector<ArcExpression*> _constituents;
            
        public:
            Multiset eval(ExpressionContext& context) const override {
                Multiset ms;
                for (auto expr : _constituents) {
                    ms += expr->eval(context);
                }
                return ms;
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                for (auto elem : _constituents) {
                    elem->getVariables(variables);
                }
            }
            
            AddExpression(std::vector<ArcExpression*> constituents)
                    : _constituents(constituents) {}
        };
        
        class SubtractExpression : public ArcExpression {
        private:
            ArcExpression* _left;
            ArcExpression* _right;
            
        public:
            Multiset eval(ExpressionContext& context) const override {
                return _left->eval(context) - _right->eval(context);
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _left->getVariables(variables);
                _right->getVariables(variables);
            }
            
            SubtractExpression(ArcExpression* left, ArcExpression* right)
                    : _left(left), _right(right) {}
        };
        
        class ScalarProductExpression : public ArcExpression {
        private:
            uint32_t _scalar;
            ArcExpression* _expr;
            
        public:
            Multiset eval(ExpressionContext& context) const override {
                return _expr->eval(context) * _scalar;
            }
            
            void getVariables(std::set<Variable*>& variables) override {
                _expr->getVariables(variables);
            }
            
            ScalarProductExpression(ArcExpression* expr, uint32_t scalar)
                    : _scalar(scalar), _expr(expr) {}
        };
    }
}

#endif /* COLORED_EXPRESSIONS_H */

