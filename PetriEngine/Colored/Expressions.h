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

#include "Colors.h"
#include "Multiset.h"

namespace PetriEngine {
    class ColoredPetriNetBuilder;
    
    namespace Colored {
        struct ExpressionContext {
            std::unordered_map<std::string, Color*> binding;
        };
        
        class ColorExpression {
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
        
        class UserSortExpression {
        private:
            ColorType* _userSort;
            
        public:
            ColorType* eval(ExpressionContext& context) const {
                return _userSort;
            }
            
            UserSortExpression(ColorType* userSort)
                    : _userSort(userSort) {}
        };
        
        class NumberConstantExpression {
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
            
            PredecessorExpression(ColorExpression* color)
                    : _color(color) {}
        };
        
        class TupleExpression : public ColorExpression {
        private:
            std::vector<ColorExpression*> _colors;
            
        public:
            const Color* eval(ExpressionContext& context) const override {
                return nullptr; // TODO
            }
            
            TupleExpression(std::vector<ColorExpression*> colors)
                    : _colors(colors) {}
        };
        
        class GuardExpression {
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
            
            OrExpression(GuardExpression* left, GuardExpression* right)
                    : _left(left), _right(right) {}
        };
        
        class ArcExpression {
        public:
            ArcExpression() {}
            virtual ~ArcExpression() {}
            
            virtual Multiset eval(ExpressionContext& context) const = 0;
        };
        
        class AllExpression {
        private:
            ColorType* _sort;
            
        public:
            std::vector<const Color*> eval(ExpressionContext& context) const {
                std::vector<const Color*> colors;
                for (size_t i = 0; i < _sort->size(); i++) {
                    colors.push_back(&(*_sort)[i]);
                }
                return colors;
            }
            
            AllExpression(ColorType* sort) : _sort(sort) {}
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
            
            ScalarProductExpression(ArcExpression* expr, uint32_t scalar)
                    : _scalar(scalar), _expr(expr) {}
        };
    }
}

#endif /* COLORED_EXPRESSIONS_H */

