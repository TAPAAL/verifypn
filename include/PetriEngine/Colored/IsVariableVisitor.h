//
// Created by jespo on 13-05-2022.
//

#ifndef VERIFYPN_ISVARIABLEVISITOR_H
#define VERIFYPN_ISVARIABLEVISITOR_H

#include "Expressions.h"

namespace PetriEngine {
    namespace Colored {
        class IsVariableVisitor : public ColorExpressionVisitor{
            bool _isVariable = false;
            const Variable* _variable = nullptr;
            
        public:            
            void accept(const DotConstantExpression*) override {
                _isVariable = false;
            };
            void accept(const VariableExpression* e) override {
                _isVariable = true;
                _variable = e->variable();
            };
             void accept(const UserOperatorExpression*) override {
                _isVariable = false;
            };
             void accept(const SuccessorExpression*) override {
                _isVariable = false;
            };
             void accept(const PredecessorExpression*) override {
                _isVariable = false;
            };
             void accept(const TupleExpression*)  override {
                _isVariable = false;
            };
             void accept(const LessThanExpression*)  override {
                _isVariable = false;
            };
             void accept(const LessThanEqExpression*) override {
                _isVariable = false;
            };
             void accept(const EqualityExpression*) override {
                _isVariable = false;
            };
             void accept(const InequalityExpression*) override {
                _isVariable = false;
            };
             void accept(const AndExpression*) override {
                _isVariable = false;
            };
             void accept(const OrExpression*) override {
                _isVariable = false;
            };
             void accept(const AllExpression*) override {
                _isVariable = false;
            };
             void accept(const NumberOfExpression*) override {
                _isVariable = false;
            };
             void accept(const AddExpression*) override {
                _isVariable = false;
            };
             void accept(const SubtractExpression*) override {
                _isVariable = false;
            };
             void accept(const ScalarProductExpression*) override {
                _isVariable = false;
            };

            bool isVariableExpr(const ColorExpression* e){
                e->visit(*this);
                return _isVariable;
            }

            const Variable* getVariable(const ColorExpression* e){
                e->visit(*this);
                return _variable;
            }

            std::string getVariableName(const ColorExpression* e){
                e->visit(*this);
                return (_isVariable ? _variable->name : "No");
            }
        };

    }
}

#endif //VERIFYPN_ISVARIABLEVISITOR_H
