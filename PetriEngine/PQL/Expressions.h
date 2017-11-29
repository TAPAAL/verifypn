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


#include <iostream>
#include <fstream>
#include "PQL.h"
#include "Contexts.h"
#include "PetriEngine/Simplification/Member.h"
#include "PetriEngine/Simplification/LinearPrograms.h"
#include "PetriEngine/Simplification/Retval.h"

using namespace PetriEngine::Simplification;

namespace PetriEngine {
    namespace PQL {
        
        std::string generateTabs(uint32_t tabs);

        /******************** EXPRESSIONS ********************/

        /** Base class for all binary expressions */
        class NaryExpr : public Expr {
        public:

            NaryExpr(std::vector<Expr_ptr>&& exprs) : _exprs(std::move(exprs)) {
            }
            virtual void analyze(AnalysisContext& context);
            int evaluate(const EvaluationContext& context) const;
            int formulaSize() const{
                size_t sum = 0;
                for(auto& e : _exprs) sum += e->formulaSize();
                return sum;
            }
            void toString(std::ostream&) const;
            virtual void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const = 0;
        private:
            virtual int apply(int v1, int v2) const = 0;
            virtual std::string op() const = 0;
        protected:
            std::vector<Expr_ptr> _exprs;
            virtual int32_t preOp(const EvaluationContext& context) const;
        };

        /** Binary plus expression */
        class PlusExpr : public NaryExpr {
        public:

            PlusExpr(std::vector<Expr_ptr>&& exprs, bool tk = false) :
            NaryExpr(std::move(exprs)), tk(tk)
            {
            }
            
            Expr::Types type() const;
            Member constraint(SimplificationContext& context) const;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const;
            bool tk = false;
            void incr(ReducingSuccessorGenerator& generator) const;
            void decr(ReducingSuccessorGenerator& generator) const;
            void analyze(AnalysisContext& context);            
        protected:
            virtual int32_t preOp(const EvaluationContext& context) const;

        private:
            int apply(int v1, int v2) const;
            //int binaryOp() const;
            std::string op() const;
        };

        /** Binary minus expression */
        class SubtractExpr : public NaryExpr {
        public:

            SubtractExpr(std::vector<Expr_ptr>&& exprs) : NaryExpr(std::move(exprs))
            {
                if(_exprs.size() != 2)
                {
                    std::cout << "SubstractExpr requieres exactly two sub-expressions" << std::endl;
                    exit(-1);
                }
            }
            Expr::Types type() const;
            Member constraint(SimplificationContext& context) const;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const;
            void incr(ReducingSuccessorGenerator& generator) const;
            void decr(ReducingSuccessorGenerator& generator) const;
        private:
            int apply(int v1, int v2) const;
            //int binaryOp() const;
            std::string op() const;
        };

        /** Binary multiplication expression **/
        class MultiplyExpr : public NaryExpr {
        public:

            using NaryExpr::NaryExpr;
            Expr::Types type() const;
            Member constraint(SimplificationContext& context) const;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const;
            void incr(ReducingSuccessorGenerator& generator) const;
            void decr(ReducingSuccessorGenerator& generator) const;
            void analyze(AnalysisContext& context);
        protected:
            virtual int32_t preOp(const EvaluationContext& context) const;
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
            int evaluate(const EvaluationContext& context) const;
            void toString(std::ostream&) const;
            Expr::Types type() const;
            Member constraint(SimplificationContext& context) const;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const;
            void incr(ReducingSuccessorGenerator& generator) const;
            void decr(ReducingSuccessorGenerator& generator) const;
            int formulaSize() const{
                return _expr->formulaSize() + 1;
            }
        private:
            Expr_ptr _expr;
        };

        /** Literal integer value expression */
        class LiteralExpr : public Expr {
        public:

            LiteralExpr(int value) : _value(value) {
            }
            void analyze(AnalysisContext& context);
            int evaluate(const EvaluationContext& context) const;
            void toString(std::ostream&) const;
            Expr::Types type() const;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const;
            void incr(ReducingSuccessorGenerator& generator) const;
            void decr(ReducingSuccessorGenerator& generator) const;
            int formulaSize() const{
                return 1;
            }
            int value() const {
                return _value;
            };
            Member constraint(SimplificationContext& context) const;
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
            int evaluate(const EvaluationContext& context) const;
            void toString(std::ostream&) const;
            Expr::Types type() const;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const;
            void incr(ReducingSuccessorGenerator& generator) const;
            void decr(ReducingSuccessorGenerator& generator) const;
            int formulaSize() const{
                return 1;
            }
            /** Offset in marking or valuation */
            int offset() const {
                return _offsetInMarking;
            }
            Member constraint(SimplificationContext& context) const;
        private:
            /** Offset in marking, -1 if undefined, should be resolved during analysis */
            int _offsetInMarking;
            /** Identifier text */
            std::string _name;
        };
        
        /******************** TEMPORAL OPERATORS ********************/

        class QuantifierCondition : public Condition
        {
        public:
            virtual bool isTemporal() const override { return true;}
            CTLType getQueryType() const override { return CTLType::PATHQEURY; }
            virtual const Condition_ptr& operator[] (size_t i) const = 0;
        };
        
        class SimpleQuantifierCondition : public QuantifierCondition {
        public:
            SimpleQuantifierCondition(const Condition_ptr cond) {
                _cond = cond;
            }
            int formulaSize() const{
                return _cond->formulaSize() + 1;
            }
            
            void analyze(AnalysisContext& context);
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);
            void toString(std::ostream&) const;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const;
            virtual Retval simplify(SimplificationContext& context) const = 0;
            virtual bool isReachability(uint32_t depth) const = 0;
            bool isUpperBound();
            Condition_ptr prepareForReachability(bool negated) const = 0;
            virtual void toXML(std::ostream&, uint32_t tabs) const = 0;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            virtual const Condition_ptr& operator[] (size_t i) const { return _cond;}
        private:
            virtual std::string op() const = 0;
            
        protected:
            Condition_ptr _cond;
        };
        
        class EXCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Retval simplify(SimplificationContext& context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::X; }
            uint32_t distance(DistanceContext& context) const;
            
        private:
            std::string op() const;
        };
        
        class EGCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Retval simplify(SimplificationContext& context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::G; }            
            uint32_t distance(DistanceContext& context) const;
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);
        private:
            std::string op() const;
        };
        
        class EFCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Retval simplify(SimplificationContext& context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::F; }            
            uint32_t distance(DistanceContext& context) const;
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);
        private:
            std::string op() const;
        };
        
        class AXCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Retval simplify(SimplificationContext& context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::X; }
            uint32_t distance(DistanceContext& context) const;
        private:
            std::string op() const;
        };
        
        class AGCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Retval simplify(SimplificationContext& context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::G; } 
            uint32_t distance(DistanceContext& context) const;
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);
        private:
            std::string op() const;
        };
        
        class AFCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Retval simplify(SimplificationContext& context) const;
            bool isReachability(uint32_t depth) const;
            Condition_ptr prepareForReachability(bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::F; }            
            uint32_t distance(DistanceContext& context) const;
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);
        private:
            std::string op() const;
        };     
        
        class UntilCondition : public QuantifierCondition {
        public:
            UntilCondition(const Condition_ptr cond1, const Condition_ptr cond2) {
                _cond1 = cond1;
                _cond2 = cond2;
            }
            int formulaSize() const{
                return _cond1->formulaSize() + _cond2->formulaSize() + 1;
            }
            
            void analyze(AnalysisContext& context);
            Result evaluate(const EvaluationContext& context) const;
            void toString(std::ostream&) const;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const;
            virtual Retval simplify(SimplificationContext& context) const = 0;
            bool isReachability(uint32_t depth) const;
            bool isUpperBound();
            Condition_ptr prepareForReachability(bool negated) const;
            virtual void toXML(std::ostream&, uint32_t tabs) const = 0;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            Result evalAndSet(const EvaluationContext& context);
            virtual const Condition_ptr& operator[] (size_t i) const
            { if(i == 0) return _cond1; return _cond2;}
            Path getPath() const override             
            { return Path::U; }

        private:
            virtual std::string op() const = 0; 
            
        protected:
            Condition_ptr _cond1;
            Condition_ptr _cond2;
        };
        
        class EUCondition : public UntilCondition {
        public:
            using UntilCondition::UntilCondition;  
            Retval simplify(SimplificationContext& context) const;
            Quantifier getQuantifier() const override { return Quantifier::E; }
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            uint32_t distance(DistanceContext& context) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            
        private:
            std::string op() const;
        };
        
        class AUCondition : public UntilCondition {
        public:
            using UntilCondition::UntilCondition;
            Retval simplify(SimplificationContext& context) const;
            Quantifier getQuantifier() const override { return Quantifier::A; }            
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            uint32_t distance(DistanceContext& context) const;
            void toXML(std::ostream&, uint32_t tabs) const;
        private:
            std::string op() const;
        };
        
        /******************** CONDITIONS ********************/

        /* Logical conditon */
        class LogicalCondition : public Condition {
        public:
            int formulaSize() const{
                size_t i = 1;
                for(auto& c : _conds) i += c->formulaSize();
                return i;
            }
            void analyze(AnalysisContext& context);
            
            uint32_t distance(DistanceContext& context) const;
            void toString(std::ostream&) const;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const;
            bool isReachability(uint32_t depth) const;
            bool isUpperBound();
            Condition_ptr prepareForReachability(bool negated) const;
            const Condition_ptr& operator[](size_t i) const
            { 
                return _conds[i];
            };            
            CTLType getQueryType() const override { return CTLType::LOPERATOR; }
            Path getPath() const override         { return Path::pError; }
            
            bool isTemporal() const override
            {
                return _temporal;
            }
            virtual void toXML(std::ostream&, uint32_t tabs) const = 0;
            auto begin() { return _conds.begin(); }
            auto end() { return _conds.end(); }
            auto begin() const { return _conds.begin(); }
            auto end() const { return _conds.end(); }
        protected:
            LogicalCondition() {};
            Retval simplifyOr(SimplificationContext& context) const;
            Retval simplifyAnd(SimplificationContext& context) const;      
            template<typename T>
            void tryMerge(const Condition_ptr& ptr)
            {
                if(auto lor = std::dynamic_pointer_cast<T>(ptr))
                {
                    _conds.insert(_conds.begin(), lor->_conds.begin(), lor->_conds.end());
                }
                else
                {
                    _conds.emplace_back(ptr);
                }
            }
            void sort()
            {
                std::sort(std::begin(_conds), std::end(_conds), 
                        [](auto& a, auto& b){ return a->isTemporal() < b->isTemporal(); });
            }

        private:
            virtual uint32_t delta(uint32_t d1, uint32_t d2, const DistanceContext& context) const = 0;
            virtual std::string op() const = 0;
            
        protected:
            bool _temporal = false;
            std::vector<Condition_ptr> _conds;
        };

        /* Conjunctive and condition */
        class AndCondition : public LogicalCondition {
        public:

            AndCondition(std::vector<Condition_ptr>&& conds) {
                for(auto& c : conds) tryMerge<AndCondition>(c);
                for(auto& c : _conds) _temporal = _temporal || c->isTemporal();
                sort();
            }

            AndCondition(const std::vector<Condition_ptr>& conds)
            {
                for(auto& c : conds) tryMerge<AndCondition>(c);
                for(auto& c : conds) _temporal = _temporal || c->isTemporal();
                sort();
            }
            
            AndCondition(Condition_ptr left, Condition_ptr right)
            {
                this->tryMerge<AndCondition>(left);
                this->tryMerge<AndCondition>(right);
                for(auto& c : _conds) _temporal = _temporal || c->isTemporal();
                sort();
            }
            
            Retval simplify(SimplificationContext& context) const;
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);

            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            Quantifier getQuantifier() const override { return Quantifier::AND; }
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;

        private:
            //int logicalOp() const;
            uint32_t delta(uint32_t d1, uint32_t d2, const DistanceContext& context) const;
            std::string op() const;
        };

        /* Disjunctive or conditon */
        class OrCondition : public LogicalCondition {
        public:

            OrCondition(std::vector<Condition_ptr>&& conds) {
                for(auto& c : conds) tryMerge<OrCondition>(c);
                for(auto& c : conds) _temporal = _temporal || c->isTemporal();
                sort();
            }

            OrCondition(const std::vector<Condition_ptr>& conds)
            {
                for(auto& c : conds) tryMerge<OrCondition>(c);
                for(auto& c : conds) _temporal = _temporal || c->isTemporal();
                sort();
            }

            OrCondition(Condition_ptr left, Condition_ptr right)
            {
                this->tryMerge<OrCondition>(left);
                this->tryMerge<OrCondition>(right);
                for(auto& c : _conds) _temporal = _temporal || c->isTemporal();
                sort();
            };
            
            Retval simplify(SimplificationContext& context) const;
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);

            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;   
            Quantifier getQuantifier() const override { return Quantifier::OR; }
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
        private:
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
            int formulaSize() const{
                return _expr1->formulaSize() + _expr2->formulaSize() + 1;
            }
            void analyze(AnalysisContext& context);
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);
            void toString(std::ostream&) const;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const;
            bool isReachability(uint32_t depth) const;
            bool isUpperBound();
            Condition_ptr prepareForReachability(bool negated) const;
            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
            virtual void toXML(std::ostream&, uint32_t tabs) const = 0;
        protected:
            uint32_t _distance(DistanceContext& c, 
                    std::function<uint32_t(uint32_t, uint32_t, bool)> d) const
            {
                return d(_expr1->evaluate(c), _expr2->evaluate(c), c.negated());
            }
        private:
            virtual bool apply(int v1, int v2) const = 0;
            virtual std::string op() const = 0;
            /** Operator when exported to TAPAAL */
            virtual std::string opTAPAAL() const = 0;
            /** Swapped operator when exported to TAPAAL, e.g. operator when operands are swapped */
            virtual std::string sopTAPAAL() const = 0;

        protected:
            Expr_ptr _expr1;
            Expr_ptr _expr2;
        };

        /* delta */
        template<typename T>
        uint32_t delta(int v1, int v2, bool negated)
        { return 0; }
        
        class EqualCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext& context) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            uint32_t distance(DistanceContext& context) const;

        private:
            bool apply(int v1, int v2) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* None equality conditon */
        class NotEqualCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const;
            Retval simplify(SimplificationContext& context) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            uint32_t distance(DistanceContext& context) const;
        private:
            bool apply(int v1, int v2) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* Less-than conditon */
        class LessThanCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext& context) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            uint32_t distance(DistanceContext& context) const;
        private:
            bool apply(int v1, int v2) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* Less-than-or-equal conditon */
        class LessThanOrEqualCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext& context) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            uint32_t distance(DistanceContext& context) const;
        private:
            bool apply(int v1, int v2) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* Greater-than conditon */
        class GreaterThanCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext& context) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            uint32_t distance(DistanceContext& context) const;
        private:
            bool apply(int v1, int v2) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* Greater-than-or-equal conditon */
        class GreaterThanOrEqualCondition : public CompareCondition {
        public:
            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext& context) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            uint32_t distance(DistanceContext& context) const;
        private:
            bool apply(int v1, int v2) const;
            std::string op() const;
            std::string opTAPAAL() const;
            std::string sopTAPAAL() const;
        };

        /* Not condition */
        class NotCondition : public Condition {
        public:

            NotCondition(const Condition_ptr cond) {
                _cond = cond;
                _temporal = _cond->isTemporal();
            }
            int formulaSize() const{
                return _cond->formulaSize(); // do not count the not node
            }
            void analyze(AnalysisContext& context);
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);
            uint32_t distance(DistanceContext& context) const;
            void toString(std::ostream&) const;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const;
            Retval simplify(SimplificationContext& context) const;
            bool isReachability(uint32_t depth) const;
            bool isUpperBound();
            Condition_ptr prepareForReachability(bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            Quantifier getQuantifier() const override { return Quantifier::NEG; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::LOPERATOR; }
            const Condition_ptr& operator[](size_t i) const { return _cond; };
            virtual bool isTemporal() const override { return _temporal;}

        private:
            Condition_ptr _cond;
            bool _temporal = false;
        };

        /* Bool condition */
        class BooleanCondition : public Condition {
        public:

            BooleanCondition(bool value) : _value(value) {
                if (value) {
                    trivial = 1;
                } else {
                    trivial = 2;
                }
            }
            int formulaSize() const{
                if(trivial > 0)
                    return 0;
                else 
                    return 1;
            }
            void analyze(AnalysisContext& context);
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);
            uint32_t distance(DistanceContext& context) const;
            static Condition_ptr TRUE_CONSTANT;
            static Condition_ptr FALSE_CONSTANT;
            void toString(std::ostream&) const;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const;
            static Condition_ptr getShared(bool val);
            Retval simplify(SimplificationContext& context) const;
            bool isReachability(uint32_t depth) const;
            bool isUpperBound();
            Condition_ptr prepareForReachability(bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
        private:
            const bool _value;
        };

        /* Deadlock condition */
        class DeadlockCondition : public Condition {
        public:

            DeadlockCondition() {
            }
            int formulaSize() const{
                return 1;
            }
            void analyze(AnalysisContext& context);
            Result evaluate(const EvaluationContext& context) const;
            Result evalAndSet(const EvaluationContext& context);
            uint32_t distance(DistanceContext& context) const;
            void toString(std::ostream&) const;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const;
            Retval simplify(SimplificationContext& context) const;
            bool isReachability(uint32_t depth) const;
            bool isUpperBound();
            Condition_ptr prepareForReachability(bool negated) const;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const;
            void toXML(std::ostream&, uint32_t tabs) const;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const;
            static Condition_ptr DEADLOCK;
            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
        };

    }
}



#endif // EXPRESSIONS_H
