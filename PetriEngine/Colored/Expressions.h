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
            ColorExpression();
            virtual ~ColorExpression() {}
            
            virtual Color* eval(ExpressionContext& context) const = 0;
        };
        
        class DotConstantExpression : public ColorExpression {
        public:
            Color* eval(ExpressionContext& context) const override {
                return DotConstant::dotConstant();
            }
        };
        
        class VariableExpression : public ColorExpression {
        private:
            Variable* _variable;
            
        public:
            Color* eval(ExpressionContext& context) const override {
                return context.binding[_variable->name];
            }
            
            VariableExpression(Variable* variable);
        };
        
        class UserOperatorExpression : public ColorExpression {
        private:
            Color* _userOperator;
            
        public:
            Color* eval(ExpressionContext& context) const override {
                return _userOperator;
            }
            
            UserOperatorExpression(Color* userOperator);
        };
        
        class UserSortExpression {
        private:
            ColorType* _userSort;
            
        public:
            ColorType* eval(ExpressionContext& context) const {
                return _userSort;
            }
            
            UserSortExpression(ColorType* userSort);
        };
        
        class NumberConstantExpression {
        private:
            uint32_t _number;
            
        public:
            uint32_t eval(ExpressionContext& context) const {
                return _number;
            }
            
            NumberConstantExpression(uint32_t number);
        };
        
        class SuccessorExpression : public ColorExpression {
        private:
            ColorExpression* _color;
            
        public:
            Color* eval(ExpressionContext& context) const override {
                return &++(*_color->eval(context));
            }
            
            SuccessorExpression(ColorExpression* color);
        };
        
        class PredecessorExpression : public ColorExpression {
        private:
            ColorExpression* _color;
            
        public:
            Color* eval(ExpressionContext& context) const override {
                return &--(*_color->eval(context));
            }
            
            PredecessorExpression(ColorExpression* color);
        };
        
        class TupleExpression : public ColorExpression {
        private:
            std::vector<ColorExpression*> _colors;
            
        public:
            Color* eval(ExpressionContext& context) const override {
                return nullptr; // TODO
            }
            
            TupleExpression(std::vector<ColorExpression*> colors);
        };
        
        class GuardExpression {
        public:
            GuardExpression();
            virtual ~GuardExpression() {}
            
            virtual bool eval(ExpressionContext& context) const = 0;
        };
        
        class ArcExpression {
        public:
            ArcExpression();
            virtual ~ArcExpression() {}
            
            virtual Multiset eval(ExpressionContext& context) const = 0;
        };
        
        class NumberOfExpression : public ArcExpression {
        private:
            uint32_t _number;
            ColorExpression* _color;
            
        public:
            Multiset eval(ExpressionContext& context) const {
                // TODO
            }
            
            NumberOfExpression(ColorExpression* color, uint32_t number = 1);
        };
        
        class AllExpression {
            // TODO
        };
        
        class AddExpression : public ArcExpression {
        private:
            std::vector<ArcExpression*> _constituents;
            
        public:
            Multiset eval(ExpressionContext& context) const {
                Multiset ms();
                for (auto expr : _constituents) {
                    ms += expr->eval(context);
                }
                return ms;
            }
            
            AddExpression(std::vector<ArcExpression*> constituents);
        };
        
        class SubtractExpression : public ArcExpression {
        private:
            ArcExpression* _left;
            ArcExpression* _right;
            
        public:
            Multiset eval(ExpressionContext& context) const {
                return _left->eval(context) - _right->eval(context);
            }
            
            SubtractExpression(ArcExpression* _left, ArcExpression* _right);
        };
        
        class ScalarProductExpression : public ArcExpression {
        private:
            uint32_t _scalar;
            ArcExpression* _expr;
            
        public:
            Multiset eval(ExpressionContext& context) const {
                return _expr->eval(context) * _scalar;
            }
            
            ScalarProductExpression(ArcExpression* expr, uint32_t scalar);
        };
    }
}

#endif /* COLORED_EXPRESSIONS_H */

