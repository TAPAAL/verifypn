/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include "PQL.h"
#include "Contexts.h"
#include <iostream>
#include <fstream>

using namespace PetriEngine::Simplification;

namespace PetriEngine {
    namespace PQL {
        /**
        std::string generateTabs(uint32_t tabs) {
            std::string res;

            for(uint32_t i = 0; i < tabs; i++) {
                res += "\t";
            }

            return res;
        }
        */

        /******************** EXPRESSIONS ********************/

        /** Base class for all binary expressions */
        class BinaryExpr : public Expr {
        public:

            BinaryExpr(const Expr_ptr expr1, const Expr_ptr expr2) {
                _expr1 = expr1;
                _expr2 = expr2;
            }
            void analyze(AnalysisContext& context);
            bool pfree() const;
            int evaluate(const EvaluationContext& context) const;
            //llvm::Value* codegen(CodeGenerationContext& context) const;
            std::string toString() const;
            virtual std::string toXML(uint32_t tabs) const = 0;
        private:
            virtual int apply(int v1, int v2) const = 0;
            /** LLVM binary operator (llvm::Instruction::BinaryOps) */
            //virtual int binaryOp() const = 0;
            virtual std::string op() const = 0;
        protected:
            Expr_ptr _expr1;
            Expr_ptr _expr2;
        };

        /** Binary plus expression */
        class PlusExpr : public BinaryExpr {
        public:

            using BinaryExpr::BinaryExpr;
            Expr::Types type() const;
            Member constraint(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
        private:
            int apply(int v1, int v2) const;
            //int binaryOp() const;
            std::string op() const;
        };

        /** Binary minus expression */
        class SubtractExpr : public BinaryExpr {
        public:

            using BinaryExpr::BinaryExpr;
            Expr::Types type() const;
            Member constraint(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
        private:
            int apply(int v1, int v2) const;
            //int binaryOp() const;
            std::string op() const;
        };

        /** Binary multiplication expression **/
        class MultiplyExpr : public BinaryExpr {
        public:

            using BinaryExpr::BinaryExpr;
            Expr::Types type() const;
            Member constraint(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
        private:
            int apply(int v1, int v2) const;
            //int binaryOp() const;
            std::string op() const;
        };

        /** Unary minus expression*/
        class MinusExpr : public Expr {
        public:

            MinusExpr(const Expr_ptr expr) {
                _expr = expr;
            }
            void analyze(AnalysisContext& context);
            bool pfree() const;
            int evaluate(const EvaluationContext& context) const;
            //llvm::Value* codegen(CodeGenerationContext& context) const;
            std::string toString() const;
            Expr::Types type() const;
            Member constraint(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
        private:
            Expr_ptr _expr;
        };

        /** Literal integer value expression */
        class LiteralExpr : public Expr {
        public:

            LiteralExpr(int value) : _value(value) {
            }
            void analyze(AnalysisContext& context);
            bool pfree() const;
            int evaluate(const EvaluationContext& context) const;
            //llvm::Value* codegen(CodeGenerationContext& context) const;
            std::string toString() const;
            Expr::Types type() const;
            std::string toXML(uint32_t tabs) const;

            int value() const {
                return _value;
            };
            Member constraint(SimplificationContext context) const;
        private:
            int _value;
        };

        /** Identifier expression */
        class IdentifierExpr : public Expr {
        public:

            IdentifierExpr(const std::string& name) : _name(name) {
                _offsetInMarking = -1;
            }
            void analyze(AnalysisContext& context);
            bool pfree() const;
            int evaluate(const EvaluationContext& context) const;
            //llvm::Value* codegen(CodeGenerationContext& context) const;
            std::string toString() const;
            Expr::Types type() const;
            std::string toXML(uint32_t tabs) const;

            /** Offset in marking or valuation */
            int offset() const {
                return _offsetInMarking;
            }
            Member constraint(SimplificationContext context) const;
        private:
            /** Offset in marking, -1 if undefined, should be resolved during analysis */
            int _offsetInMarking;
            /** Identifier text */
            std::string _name;
        };
        
        /******************** TEMPORAL OPERATORS ********************/

        class QuantifierCondition : public Condition {
        public:
            QuantifierCondition(const Condition_ptr cond) {
                _cond = cond;
            }
            
            void analyze(AnalysisContext& context);
            bool evaluate(const EvaluationContext& context) const;
            std::string toString() const;
            std::string toTAPAALQuery(TAPAALConditionExportContext& context) const;
            uint32_t distance(DistanceContext& context) const;
            virtual Retval simplify(SimplificationContext context) const = 0;
            virtual bool isReachability(uint32_t depth) const = 0;
            Condition_ptr prepareForReachability(bool negated) const = 0;
            virtual std::string toXML(uint32_t tabs) const = 0;
            
        private:
            virtual std::string op() const = 0;
            
        protected:
            Condition_ptr _cond;
        };
        
        class EXCondition : public QuantifierCondition {
        public:
            using QuantifierCondition::QuantifierCondition;
            Retval simplify(SimplificationContext context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            std::string op() const;
        };
        
        class EGCondition : public QuantifierCondition {
        public:
            using QuantifierCondition::QuantifierCondition;
            Retval simplify(SimplificationContext context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            std::string op() const;
        };
        
        class EFCondition : public QuantifierCondition {
        public:
            using QuantifierCondition::QuantifierCondition;
            Retval simplify(SimplificationContext context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            std::string op() const;
        };
        
        class AXCondition : public QuantifierCondition {
        public:
            using QuantifierCondition::QuantifierCondition;
            Retval simplify(SimplificationContext context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            std::string op() const;
        };
        
        class AGCondition : public QuantifierCondition {
        public:
            using QuantifierCondition::QuantifierCondition;
            Retval simplify(SimplificationContext context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            std::string op() const;
        };
        
        class AFCondition : public QuantifierCondition {
        public:
            using QuantifierCondition::QuantifierCondition;
            Retval simplify(SimplificationContext context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            std::string op() const;
        };     
        
        class UntilCondition : public Condition {
        public:
            UntilCondition(const Condition_ptr cond1, const Condition_ptr cond2) {
                _cond1 = cond1;
                _cond2 = cond2;
            }
            
            void analyze(AnalysisContext& context);
            bool evaluate(const EvaluationContext& context) const;
            std::string toString() const;
            std::string toTAPAALQuery(TAPAALConditionExportContext& context) const;
            uint32_t distance(DistanceContext& context) const;
            virtual Retval simplify(SimplificationContext context) const = 0;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            virtual std::string toXML(uint32_t tabs) const = 0;
            
        private:
            virtual std::string op() const = 0;
            
        protected:
            Condition_ptr _cond1;
            Condition_ptr _cond2;
        };
        
        class EUCondition : public UntilCondition {
        public:
            using UntilCondition::UntilCondition;  
            Retval simplify(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            std::string op() const;
        };
        
        class AUCondition : public UntilCondition {
        public:
            using UntilCondition::UntilCondition;
            Retval simplify(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            std::string op() const;
        };
        
        /******************** CONDITIONS ********************/

        /* Logical conditon */
        class LogicalCondition : public Condition {
        public:

            LogicalCondition(const Condition_ptr cond1, const Condition_ptr cond2) {
                _cond1 = cond1;
                _cond2 = cond2;
            }
            void analyze(AnalysisContext& context);
            bool evaluate(const EvaluationContext& context) const;
            //llvm::Value* codegen(CodeGenerationContext& context) const;
            uint32_t distance(DistanceContext& context) const;
            std::string toString() const;
            std::string toTAPAALQuery(TAPAALConditionExportContext& context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            virtual std::string toXML(uint32_t tabs) const = 0;
            
        private:
            virtual bool apply(bool b1, bool b2) const = 0;
            /** LLVM binary operator (llvm::Instruction::BinaryOps) */
            //virtual int logicalOp() const = 0;
            virtual uint32_t delta(uint32_t d1, uint32_t d2, const DistanceContext& context) const = 0;
            virtual std::string op() const = 0;
        protected:
            Condition_ptr _cond1;
            Condition_ptr _cond2;
        };

        /* Conjunctive and condition */
        class AndCondition : public LogicalCondition {
        public:

            using LogicalCondition::LogicalCondition;
            Retval simplify(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;

        private:
            bool apply(bool b1, bool b2) const;
            //int logicalOp() const;
            uint32_t delta(uint32_t d1, uint32_t d2, const DistanceContext& context) const;
            std::string op() const;
        };

        /* Disjunctive or conditon */
        class OrCondition : public LogicalCondition {
        public:

            using LogicalCondition::LogicalCondition;
            Retval simplify(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            bool apply(bool b1, bool b2) const;
            //int logicalOp() const;
            uint32_t delta(uint32_t d1, uint32_t d2, const DistanceContext& context) const;
            std::string op() const;
        };

        /* Comparison conditon */
        class CompareCondition : public Condition {
        public:

            CompareCondition(const Expr_ptr expr1, const Expr_ptr expr2) {
                _expr1 = expr1;
                _expr2 = expr2;
            }
            void analyze(AnalysisContext& context);
            bool evaluate(const EvaluationContext& context) const;
            //llvm::Value* codegen(CodeGenerationContext& context) const;
            uint32_t distance(DistanceContext& context) const;
            std::string toString() const;
            std::string toTAPAALQuery(TAPAALConditionExportContext& context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            virtual std::string toXML(uint32_t tabs) const = 0;
            
        private:
            virtual bool apply(int v1, int v2) const = 0;
            /** LLVM Comparison predicate (llvm::ICmpInst::Predicate) */
            //virtual int compareOp() const = 0;
            virtual uint32_t delta(int v1, int v2, bool negated) const = 0;
            virtual std::string op() const = 0;
            /** Operator when exported to TAPAAL */
            virtual std::string opTAPAAL() const = 0;
            /** Swapped operator when exported to TAPAAL, e.g. operator when operands are swapped */
            virtual std::string sopTAPAAL() const = 0;
        protected:
            Expr_ptr _expr1;
            Expr_ptr _expr2;
        };

        /* Equality conditon */
        class EqualCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
        private:
            bool apply(int v1, int v2) const;
            //int compareOp() const;
            uint32_t delta(int v1, int v2, bool negated) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* None equality conditon */
        class NotEqualCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            std::string toTAPAALQuery(TAPAALConditionExportContext& context) const;
            Retval simplify(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
        private:
            bool apply(int v1, int v2) const;
            //int compareOp() const;
            uint32_t delta(int v1, int v2, bool negated) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* Less-than conditon */
        class LessThanCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
        private:
            bool apply(int v1, int v2) const;
            //int compareOp() const;
            uint32_t delta(int v1, int v2, bool negated) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* Less-than-or-equal conditon */
        class LessThanOrEqualCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
        private:
            bool apply(int v1, int v2) const;
            //int compareOp() const;
            uint32_t delta(int v1, int v2, bool negated) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* Greater-than conditon */
        class GreaterThanCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;
        private:
            bool apply(int v1, int v2) const;
            //int compareOp() const;
            uint32_t delta(int v1, int v2, bool negated) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* Greater-than-or-equal conditon */
        class GreaterThanOrEqualCondition : public CompareCondition {
        public:
            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext context) const;
            std::string toXML(uint32_t tabs) const;

        private:
            bool apply(int v1, int v2) const;
            //int compareOp() const;
            uint32_t delta(int v1, int v2, bool negated) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* Not condition */
        class NotCondition : public Condition {
        public:

            NotCondition(const Condition_ptr cond) {
                _cond = cond;
            }
            void analyze(AnalysisContext& context);
            bool evaluate(const EvaluationContext& context) const;
            //llvm::Value* codegen(CodeGenerationContext& context) const;
            uint32_t distance(DistanceContext& context) const;
            std::string toString() const;
            std::string toTAPAALQuery(TAPAALConditionExportContext& context) const;
            Retval simplify(SimplificationContext context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            Condition_ptr _cond;
        };

        /* Bool condition */
        class BooleanCondition : public Condition {
        public:

            BooleanCondition(bool value) : _value(value) {
            }
            void analyze(AnalysisContext& context);
            bool evaluate(const EvaluationContext& context) const;
            uint32_t distance(DistanceContext& context) const;
            std::string toString() const;
            std::string toTAPAALQuery(TAPAALConditionExportContext& context) const;
            static Condition_ptr TRUE;
            static Condition_ptr FALSE;
            Retval simplify(SimplificationContext context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            std::string toXML(uint32_t tabs) const;
            
        private:
            const bool _value;
        };

        /* Deadlock condition */
        class DeadlockCondition : public Condition {
        public:

            DeadlockCondition() {
            }
            void analyze(AnalysisContext& context);
            bool evaluate(const EvaluationContext& context) const;
            uint32_t distance(DistanceContext& context) const;
            std::string toString() const;
            std::string toTAPAALQuery(TAPAALConditionExportContext& context) const;
            Retval simplify(SimplificationContext context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            std::string toXML(uint32_t tabs) const;
        };

    }
}



#endif // EXPRESSIONS_H
