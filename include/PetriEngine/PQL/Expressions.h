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
#include <algorithm>
#include "PQL.h"
#include "Contexts.h"
#include "..//Simplification/Member.h"
#include "../Simplification/LinearPrograms.h"
#include "../Simplification/Retval.h"

using namespace PetriEngine::Simplification;

namespace PetriEngine {
    namespace PQL {
        
        std::string generateTabs(uint32_t tabs);
        class CompareCondition;
        class NotCondition;
        /******************** EXPRESSIONS ********************/

        /** Base class for all binary expressions */
        class NaryExpr : public Expr {
        protected:
            NaryExpr() {};
        public:

            NaryExpr(std::vector<Expr_ptr>&& exprs) : _exprs(std::move(exprs)) {
            }
            virtual void analyze(AnalysisContext& context) override;
            int evaluate(const EvaluationContext& context) override;
            int formulaSize() const override{
                size_t sum = 0;
                for(auto& e : _exprs) sum += e->formulaSize();
                return sum + 1;
            }
            void toString(std::ostream&) const override;
            bool placeFree() const override;
            auto& expressions() const { return _exprs; }
        protected:
            virtual int apply(int v1, int v2) const = 0;
            virtual std::string op() const = 0;
            std::vector<Expr_ptr> _exprs;
            virtual int32_t preOp(const EvaluationContext& context) const;
        };
        
        class PlusExpr;
        class MultiplyExpr;
        
        class CommutativeExpr : public NaryExpr
        {
        public:
            friend CompareCondition;
            virtual void analyze(AnalysisContext& context) override;
            int evaluate(const EvaluationContext& context) override;
            void toBinary(std::ostream&) const override;
            int formulaSize() const override{
                size_t sum = _ids.size();
                for(auto& e : _exprs) sum += e->formulaSize();
                return sum + 1;
            }
            void toString(std::ostream&) const override;            
            bool placeFree() const override;
            auto constant() const { return _constant; }
            auto& places() const { return _ids; }
        protected:
            CommutativeExpr(int constant): _constant(constant) {};
            void init(std::vector<Expr_ptr>&& exprs);
            virtual int32_t preOp(const EvaluationContext& context) const override;
            int32_t _constant;
            std::vector<std::pair<uint32_t,std::string>> _ids;
            Member commutativeCons(int constant, SimplificationContext& context, std::function<void(Member& a, Member b)> op) const;
        };

        /** Binary plus expression */
        class PlusExpr : public CommutativeExpr {
        public:

            PlusExpr(std::vector<Expr_ptr>&& exprs, bool tk = false);
            
            Expr::Types type() const override;
            Member constraint(SimplificationContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const override;
            bool tk = false;
            void incr(ReducingSuccessorGenerator& generator) const override;
            void decr(ReducingSuccessorGenerator& generator) const override;
            void visit(Visitor& visitor) const override;
        protected:
            int apply(int v1, int v2) const override;
            //int binaryOp() const;
            std::string op() const override;
        };

        /** Binary minus expression */
        class SubtractExpr : public NaryExpr {
        public:

            SubtractExpr(std::vector<Expr_ptr>&& exprs) : NaryExpr(std::move(exprs))
            {
            }
            Expr::Types type() const override;
            Member constraint(SimplificationContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const override;
            void incr(ReducingSuccessorGenerator& generator) const override;
            void decr(ReducingSuccessorGenerator& generator) const override;
            void toBinary(std::ostream&) const override;
            void visit(Visitor& visitor) const override;
        protected:
            int apply(int v1, int v2) const override;
            //int binaryOp() const;
            std::string op() const override;
        };

        /** Binary multiplication expression **/
        class MultiplyExpr : public CommutativeExpr {
        public:

            MultiplyExpr(std::vector<Expr_ptr>&& exprs);
            Expr::Types type() const override;
            Member constraint(SimplificationContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const override;
            void incr(ReducingSuccessorGenerator& generator) const override;
            void decr(ReducingSuccessorGenerator& generator) const override;
            void visit(Visitor& visitor) const override;
        protected:
            int apply(int v1, int v2) const override;
            //int binaryOp() const;
            std::string op() const override;
        };

        /** Unary minus expression*/
        class MinusExpr : public Expr {
        public:

            MinusExpr(const Expr_ptr expr) {
                _expr = expr;
            }
            void analyze(AnalysisContext& context) override;
            int evaluate(const EvaluationContext& context) override;
            void toString(std::ostream&) const override;
            Expr::Types type() const override;
            Member constraint(SimplificationContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const override;
            void toBinary(std::ostream&) const override;
            void incr(ReducingSuccessorGenerator& generator) const override;
            void decr(ReducingSuccessorGenerator& generator) const override;
            void visit(Visitor& visitor) const override;
            int formulaSize() const override{
                return _expr->formulaSize() + 1;
            }
            bool placeFree() const override;
            const Expr_ptr& operator[](size_t i) const { return _expr; };
        private:
            Expr_ptr _expr;
        };

        /** Literal integer value expression */
        class LiteralExpr : public Expr {
        public:

            LiteralExpr(int value) : _value(value) {
            }
            void analyze(AnalysisContext& context) override;
            int evaluate(const EvaluationContext& context) override;
            void toString(std::ostream&) const override;
            Expr::Types type() const override;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const override;
            void toBinary(std::ostream&) const override;
            void incr(ReducingSuccessorGenerator& generator) const override;
            void decr(ReducingSuccessorGenerator& generator) const override;
            void visit(Visitor& visitor) const override;
            int formulaSize() const override{
                return 1;
            }
            int value() const {
                return _value;
            };
            Member constraint(SimplificationContext& context) const override;
            bool placeFree() const override { return true; }
        private:
            int _value;
        };


        class IdentifierExpr : public Expr {
        public:
            IdentifierExpr(const std::string& name) : _name(name) {}
            void analyze(AnalysisContext& context) override;
            int evaluate(const EvaluationContext& context) override {
                return _compiled->evaluate(context);
            }
            void toString(std::ostream& os) const override {
                if(_compiled)
                    _compiled->toString(os);
                else
                    os << _name;
            }
            Expr::Types type() const override {
                if(_compiled) return _compiled->type();
                return Expr::IdentifierExpr;
            }
            void toXML(std::ostream& os, uint32_t tabs, bool tokencount = false) const override {
                _compiled->toXML(os, tabs, tokencount);
            }
            void incr(ReducingSuccessorGenerator& generator) const override {
                _compiled->incr(generator);
            }
            void decr(ReducingSuccessorGenerator& generator) const override {
                _compiled->decr(generator);
            }
            int formulaSize() const override {
                if(_compiled) return _compiled->formulaSize();
                return 1;
            }
            virtual bool placeFree() const override {
                if(_compiled) return _compiled->placeFree();
                return false;
            }

            Member constraint(SimplificationContext& context) const override {
                return _compiled->constraint(context);
            }
            
            void toBinary(std::ostream& s) const override {
                _compiled->toBinary(s);
            }
            void visit(Visitor& visitor) const override;            
        private:
            std::string _name;
            Expr_ptr _compiled;
        };

        /** Identifier expression */
        class UnfoldedIdentifierExpr : public Expr {
        public:
            UnfoldedIdentifierExpr(const std::string& name, int offest)
            : _offsetInMarking(offest), _name(name) {
            }
            
            UnfoldedIdentifierExpr(const std::string& name) : UnfoldedIdentifierExpr(name, -1) {
            }
            
            void analyze(AnalysisContext& context) override;
            int evaluate(const EvaluationContext& context) override;
            void toString(std::ostream&) const override;
            Expr::Types type() const override;
            void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const override;
            void toBinary(std::ostream&) const override;
            void incr(ReducingSuccessorGenerator& generator) const override;
            void decr(ReducingSuccessorGenerator& generator) const override;
            int formulaSize() const override{
                return 1;
            }
            /** Offset in marking or valuation */
            int offset() const {
                return _offsetInMarking;
            }
            const std::string& name()
            {
                return _name;
            }
            Member constraint(SimplificationContext& context) const override;
            bool placeFree() const override { return false; }
            void visit(Visitor& visitor) const override;
        private:
            /** Offset in marking, -1 if undefined, should be resolved during analysis */
            int _offsetInMarking;
            /** Identifier text */
            std::string _name;
        };

        class ShallowCondition : public Condition
        {
            Result evaluate(const EvaluationContext& context) override
            { return _compiled->evaluate(context); }
            Result evalAndSet(const EvaluationContext& context) override
            { return _compiled->evalAndSet(context); }
            uint32_t distance(DistanceContext& context) const override
            { return _compiled->distance(context); }
            void toTAPAALQuery(std::ostream& out,TAPAALConditionExportContext& context) const override
            { _compiled->toTAPAALQuery(out, context); }
            void toBinary(std::ostream& out) const override
            { return _compiled->toBinary(out); }
            Retval simplify(SimplificationContext& context) const override
            { return _compiled->simplify(context); }
            Condition_ptr prepareForReachability(bool negated) const override
            { return _compiled->prepareForReachability(negated); }
            bool isReachability(uint32_t depth) const override
            { return _compiled->isReachability(depth); }

            void toXML(std::ostream& out, uint32_t tabs) const override
            { _compiled->toXML(out, tabs); }
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override
            { _compiled->findInteresting(generator, negated);}
            Quantifier getQuantifier() const override
            { return _compiled->getQuantifier(); }
            Path getPath() const override { return _compiled->getPath(); }
            CTLType getQueryType() const override { return _compiled->getQueryType(); }
            bool containsNext() const override { return _compiled->containsNext(); }
            bool nestedDeadlock() const override { return _compiled->nestedDeadlock(); }
#ifdef VERIFYPN_TAR
            virtual z3::expr encodeSat(const PetriNet& net, z3::context& context, std::vector<int32_t>& uses, std::vector<bool>& incremented) const
            { return _compiled->encodeSat(net, context, uses, incremented); }
#endif
            int formulaSize() const override{
                return _compiled->formulaSize();
            }

            virtual Condition_ptr pushNegation(negstat_t& neg, const EvaluationContext& context, bool nested, bool negated, bool initrw) override
            {
                if(_compiled)
                    return _compiled->pushNegation(neg, context, nested, negated, initrw);
                else {
                    if(negated)
                        return std::static_pointer_cast<Condition>(std::make_shared<NotCondition>(clone()));
                    else
                        return clone();
                }
            }

            void analyze(AnalysisContext& context) override
            {
                if (_compiled) _compiled->analyze(context);
                else _analyze(context);
            }
            void toString(std::ostream &out) const {
                if (_compiled) _compiled->toString(out);
                else _toString(out);
            }

        protected:
            virtual void _analyze(AnalysisContext& context) = 0;
            virtual void _toString(std::ostream& out) const = 0;
            virtual Condition_ptr clone() = 0;
            Condition_ptr _compiled = nullptr;
        };

        /* Not condition */
        class NotCondition : public Condition {
        public:

            NotCondition(const Condition_ptr cond) {
                _cond = cond;
                _temporal = _cond->isTemporal();
                _loop_sensitive = _cond->isLoopSensitive();
            }
            int formulaSize() const override{
                return _cond->formulaSize() + 1;
            }
            void analyze(AnalysisContext& context) override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void visit(Visitor&) const override;
            uint32_t distance(DistanceContext& context) const override;
            void toString(std::ostream&) const override;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const override;
            Retval simplify(SimplificationContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void toBinary(std::ostream&) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            Quantifier getQuantifier() const override { return Quantifier::NEG; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::LOPERATOR; }
            const Condition_ptr& operator[](size_t i) const { return _cond; };
            virtual bool isTemporal() const override { return _temporal;}
            bool containsNext() const override { return _cond->containsNext(); }
            bool nestedDeadlock() const override { return _cond->nestedDeadlock(); }
        private:
            Condition_ptr _cond;
            bool _temporal = false;
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
                _loop_sensitive = cond->isLoopSensitive();
            }
            int formulaSize() const override{
                return _cond->formulaSize() + 1;
            }
            
            void analyze(AnalysisContext& context) override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void toString(std::ostream&) const override;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const override;
            void toBinary(std::ostream& out) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            virtual const Condition_ptr& operator[] (size_t i) const override { return _cond;}
            virtual bool containsNext() const override { return _cond->containsNext(); }
            bool nestedDeadlock() const override { return _cond->nestedDeadlock(); }
        private:
            virtual std::string op() const = 0;
            
        protected:
            Condition_ptr _cond;
        };
        
        class EXCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Retval simplify(SimplificationContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::X; }
            uint32_t distance(DistanceContext& context) const override;
            bool containsNext() const override { return true; }            
            virtual bool isLoopSensitive() const override { return true; }
            void visit(Visitor&) const override;
        private:
            std::string op() const override;
        };
        
        class EGCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
 
 	    Retval simplify(SimplificationContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::G; }            
            uint32_t distance(DistanceContext& context) const override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            virtual bool isLoopSensitive() const override { return true; }
            void visit(Visitor&) const override;
        private:
            std::string op() const override;
        };
        
        class EFCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
 
 	    Retval simplify(SimplificationContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::F; }            
            uint32_t distance(DistanceContext& context) const override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void visit(Visitor&) const override;
        private:
            std::string op() const override;
        };

        class AXCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Retval simplify(SimplificationContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::X; }
            uint32_t distance(DistanceContext& context) const override;
            bool containsNext() const override { return true; }
            virtual bool isLoopSensitive() const override { return true; }
            void visit(Visitor&) const override;
        private:
            std::string op() const override;
        };
        
        class AGCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Retval simplify(SimplificationContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::G; } 
            uint32_t distance(DistanceContext& context) const override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void visit(Visitor&) const override;
        private:
            std::string op() const override;
        };
        
        class AFCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Retval simplify(SimplificationContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::F; }            
            uint32_t distance(DistanceContext& context) const override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void visit(Visitor&) const override;
            virtual bool isLoopSensitive() const override { return true; }
        private:
            std::string op() const override;
        };     
        
        class UntilCondition : public QuantifierCondition {
        public:
            UntilCondition(const Condition_ptr cond1, const Condition_ptr cond2) {
                _cond1 = cond1;
                _cond2 = cond2;
                _loop_sensitive = _cond1->isLoopSensitive() || _cond2->isLoopSensitive();
            }
            int formulaSize() const override{
                return _cond1->formulaSize() + _cond2->formulaSize() + 1;
            }
            
            void analyze(AnalysisContext& context) override;
            Result evaluate(const EvaluationContext& context) override;
            void toString(std::ostream&) const override;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const override;
            void toBinary(std::ostream& out) const override;            
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            Result evalAndSet(const EvaluationContext& context) override;

            virtual const Condition_ptr& operator[] (size_t i) const override
            { if(i == 0) return _cond1; return _cond2;}
            Path getPath() const override             
            { return Path::U; }
            bool containsNext() const override { return _cond1->containsNext() || _cond2->containsNext(); }
            bool nestedDeadlock() const override { return _cond1->nestedDeadlock() || _cond2->nestedDeadlock(); }
        private:
            virtual std::string op() const = 0; 
            
        protected:
            Condition_ptr _cond1;
            Condition_ptr _cond2;
        };
        
        class EUCondition : public UntilCondition {
        public:
            using UntilCondition::UntilCondition;  
            Retval simplify(SimplificationContext& context) const override;
            Quantifier getQuantifier() const override { return Quantifier::E; }
            void visit(Visitor&) const override;
            uint32_t distance(DistanceContext& context) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            
        private:
            std::string op() const override;
        };
        
        class AUCondition : public UntilCondition {
        public:
            using UntilCondition::UntilCondition;
            Retval simplify(SimplificationContext& context) const override;
            Quantifier getQuantifier() const override { return Quantifier::A; }            
            void visit(Visitor&) const override;
            uint32_t distance(DistanceContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            virtual bool isLoopSensitive() const override { return true; }
        private:
            std::string op() const override;
        };
        
        /******************** CONDITIONS ********************/

        class UnfoldedFireableCondition : public ShallowCondition {
        public:
            UnfoldedFireableCondition(const std::string& tname) : ShallowCondition(), _name(tname) {};
            Condition_ptr pushNegation(negstat_t& stat, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void visit(Visitor&) const override;
        protected:
            void _analyze(AnalysisContext& context) override;
            virtual void _toString(std::ostream& out) const;
            Condition_ptr clone() { return std::make_shared<UnfoldedFireableCondition>(_name); }
        private:
            const std::string _name;
        };

        class FireableCondition : public ShallowCondition {
        public:
            FireableCondition(const std::string& tname) : _name(tname) {};
            Condition_ptr pushNegation(negstat_t& stat, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void visit(Visitor&) const override;
        protected:
            void _analyze(AnalysisContext& context) override;
            virtual void _toString(std::ostream& out) const;
            Condition_ptr clone() { return std::make_shared<FireableCondition>(_name); }
        private:
            const std::string _name;
        };

        /* Logical conditon */
        class LogicalCondition : public Condition {
        public:
            int formulaSize() const override {
                size_t i = 1;
                for(auto& c : _conds) i += c->formulaSize();
                return i;
            }
            void analyze(AnalysisContext& context) override;
            
            uint32_t distance(DistanceContext& context) const override;
            void toString(std::ostream&) const override;
            void toBinary(std::ostream& out) const override;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
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
            auto begin() { return _conds.begin(); }
            auto end() { return _conds.end(); }
            auto begin() const { return _conds.begin(); }
            auto end() const { return _conds.end(); }
            bool empty() const { return _conds.size() == 0; }
            bool singular() const { return _conds.size() == 1; }
            size_t size() const { return _conds.size(); }
            bool containsNext() const override 
            { return std::any_of(begin(), end(), [](auto& a){return a->containsNext();}); }
            bool nestedDeadlock() const override;

        protected:
            LogicalCondition() {};
            Retval simplifyOr(SimplificationContext& context) const;
            Retval simplifyAnd(SimplificationContext& context) const;                  

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

            AndCondition(std::vector<Condition_ptr>&& conds);

            AndCondition(const std::vector<Condition_ptr>& conds);
            
            AndCondition(Condition_ptr left, Condition_ptr right);
            
            Retval simplify(SimplificationContext& context) const override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void visit(Visitor&) const override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            Quantifier getQuantifier() const override { return Quantifier::AND; }
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
        private:
            //int logicalOp() const;
            uint32_t delta(uint32_t d1, uint32_t d2, const DistanceContext& context) const override;
            std::string op() const override;
        };

        /* Disjunctive or conditon */
        class OrCondition : public LogicalCondition {
        public:

            OrCondition(std::vector<Condition_ptr>&& conds);

            OrCondition(const std::vector<Condition_ptr>& conds);

            OrCondition(Condition_ptr left, Condition_ptr right);
            
            Retval simplify(SimplificationContext& context) const override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void visit(Visitor&) const override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;   
            Quantifier getQuantifier() const override { return Quantifier::OR; }
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
        private:
            //int logicalOp() const;
            uint32_t delta(uint32_t d1, uint32_t d2, const DistanceContext& context) const override;
            std::string op() const override;
        };

        class CompareConjunction : public Condition
        {
        public:
            struct cons_t {
                int32_t _place  = -1;
                uint32_t _upper = std::numeric_limits<uint32_t>::max();
                uint32_t _lower = 0;
                std::string _name;
                bool operator<(const cons_t& other) const
                {
                    return _place < other._place;
                }
                
                void invert()
                {
                    if(_lower == 0 && _upper == std::numeric_limits<uint32_t>::max())
                        return;
                    assert(_lower == 0 || _upper == std::numeric_limits<uint32_t>::max());
                    if(_lower == 0)
                    {
                        _lower = _upper + 1;
                        _upper = std::numeric_limits<uint32_t>::max();
                    }
                    else if(_upper == std::numeric_limits<uint32_t>::max())
                    {
                        _upper = _lower - 1;
                        _lower = 0;
                    }
                    else
                    {
                        assert(false);
                    }
                }
                
                void intersect(const cons_t& other)
                {
                    _lower = std::max(_lower, other._lower);
                    _upper = std::min(_upper, other._upper);
                }
            };

            CompareConjunction(bool negated = false) 
                    : _negated(false) {};
            friend FireableCondition;
            CompareConjunction(const std::vector<cons_t>&& cons, bool negated) 
                    : _constraints(cons), _negated(negated) {};
            CompareConjunction(const std::vector<Condition_ptr>&, bool negated);
            CompareConjunction(const CompareConjunction& other, bool negated = false)
            : _constraints(other._constraints), _negated(other._negated != negated)
            {
            };

            void merge(const CompareConjunction& other);
            void merge(const std::vector<Condition_ptr>&, bool negated);
            
            int formulaSize() const override{
                int sum = 0;
                for(auto& c : _constraints)
                {
                    assert(c._place >= 0);
                    if(c._lower == c._upper) ++sum;
                    else {
                        if(c._lower != 0) ++sum;
                        if(c._upper != std::numeric_limits<uint32_t>::max()) ++sum;
                    }
                }
                if(sum == 1) return 2;
                else return (sum*2) + 1;
            }
            void analyze(AnalysisContext& context) override;
            uint32_t distance(DistanceContext& context) const override;
            void toString(std::ostream& stream) const override;
            void toTAPAALQuery(std::ostream& stream,TAPAALConditionExportContext& context) const override;
            void toBinary(std::ostream& out) const override;
            bool isReachability(uint32_t depth) const override { return depth > 0; };
            Condition_ptr prepareForReachability(bool negated) const override;
            CTLType getQueryType() const override { return CTLType::LOPERATOR; }
            Path getPath() const override         { return Path::pError; }            
            virtual void toXML(std::ostream& stream, uint32_t tabs) const override;
            Retval simplify(SimplificationContext& context) const override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void visit(Visitor&) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;   
            Quantifier getQuantifier() const override { return _negated ? Quantifier::OR : Quantifier::AND; }
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            bool isNegated() const { return _negated; }
            bool singular() const 
            { 
                return _constraints.size() == 1 && 
                                    (_constraints[0]._lower == 0 || 
                                     _constraints[0]._upper == std::numeric_limits<uint32_t>::max());
            };
            bool containsNext() const override { return false;}
            bool nestedDeadlock() const override { return false; }
            const std::vector<cons_t>& constraints() const { return _constraints; }
            std::vector<cons_t>::const_iterator begin() const { return _constraints.begin(); }
            std::vector<cons_t>::const_iterator end() const { return _constraints.end(); }
        private:
            std::vector<cons_t> _constraints;
            bool _negated = false;
        };


        /* Comparison conditon */
        class CompareCondition : public Condition {
        public:

            CompareCondition(const Expr_ptr expr1, const Expr_ptr expr2)
            : _expr1(expr1), _expr2(expr2) {}

            int formulaSize() const override{
                return _expr1->formulaSize() + _expr2->formulaSize() + 1;
            }
            void analyze(AnalysisContext& context) override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void toString(std::ostream&) const override;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const override;
            void toBinary(std::ostream& out) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
            const Expr_ptr& operator[](uint32_t id) const
            {
                if(id == 0) return _expr1;
                else return _expr2;
            }
            bool containsNext() const override { return false; }
            bool nestedDeadlock() const override { return false; }
            bool isTrivial() const;
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
            Retval simplify(SimplificationContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            uint32_t distance(DistanceContext& context) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void visit(Visitor&) const override;
        private:
            bool apply(int v1, int v2) const override;
            std::string op() const override;
            std::string opTAPAAL() const override;
            std::string sopTAPAAL() const override;
        };

        /* None equality conditon */
        class NotEqualCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const override;
            Retval simplify(SimplificationContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            uint32_t distance(DistanceContext& context) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void visit(Visitor&) const override;
        private:
            bool apply(int v1, int v2) const override;
            std::string op() const override;
            std::string opTAPAAL() const override;
            std::string sopTAPAAL() const override;
        };

        /* Less-than conditon */
        class LessThanCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            uint32_t distance(DistanceContext& context) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void visit(Visitor&) const override;
        private:
            bool apply(int v1, int v2) const override;
            std::string op() const override;
            std::string opTAPAAL() const override;
            std::string sopTAPAAL() const override;
        };

        /* Less-than-or-equal conditon */
        class LessThanOrEqualCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            uint32_t distance(DistanceContext& context) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void visit(Visitor&) const override;
        private:
            bool apply(int v1, int v2) const override;
            std::string op() const override;
            std::string opTAPAAL() const override;
            std::string sopTAPAAL() const override;
        };

        /* Greater-than conditon */
        class GreaterThanCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            uint32_t distance(DistanceContext& context) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void visit(Visitor&) const override;
        private:
            bool apply(int v1, int v2) const override;
            std::string op() const override;
            std::string opTAPAAL() const override;
            std::string sopTAPAAL() const override;
        };

        /* Greater-than-or-equal conditon */
        class GreaterThanOrEqualCondition : public CompareCondition {
        public:
            using CompareCondition::CompareCondition;
            Retval simplify(SimplificationContext& context) const override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            uint32_t distance(DistanceContext& context) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void visit(Visitor&) const override;
        private:
            bool apply(int v1, int v2) const override;
            std::string op() const override;
            std::string opTAPAAL() const override;
            std::string sopTAPAAL() const override;
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
            int formulaSize() const override{
                return 0;
            }
            void analyze(AnalysisContext& context) override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void visit(Visitor&) const override;           
            uint32_t distance(DistanceContext& context) const override;
            static Condition_ptr TRUE_CONSTANT;
            static Condition_ptr FALSE_CONSTANT;
            void toString(std::ostream&) const override;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const override;
            static Condition_ptr getShared(bool val);
            Retval simplify(SimplificationContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void toBinary(std::ostream&) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
            bool containsNext() const override { return false; }
            bool nestedDeadlock() const override { return false; }
        private:
            const bool _value;
        };

        /* Deadlock condition */
        class DeadlockCondition : public Condition {
        public:

            DeadlockCondition() {
                _loop_sensitive = true;
            }
            int formulaSize() const override{
                return 1;
            }
            void analyze(AnalysisContext& context) override;
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void visit(Visitor&) const override;
            uint32_t distance(DistanceContext& context) const override;
            void toString(std::ostream&) const override;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const override;
            Retval simplify(SimplificationContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void toBinary(std::ostream&) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            static Condition_ptr DEADLOCK;
            Quantifier getQuantifier() const override { return Quantifier::DEADLOCK; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
            bool containsNext() const override { return false; }
            bool nestedDeadlock() const override { return false; }
        };

        class KSafeCondition : public ShallowCondition
        {
        public:
            KSafeCondition(const Expr_ptr& expr1) : _bound(expr1)
            {}

        protected:
            void _analyze(AnalysisContext& context) override;
            void _toString(std::ostream& out) const override;
            void visit(Visitor&) const override;
            Condition_ptr clone() override
            {
                return std::make_shared<KSafeCondition>(_bound);
            }
        private:
            Expr_ptr _bound = nullptr;
        };

        class LivenessCondition : public ShallowCondition
        {
        public:
            LivenessCondition() {}
        protected:
            void _analyze(AnalysisContext& context) override;
            void _toString(std::ostream& out) const override;
            void visit(Visitor&) const override;
            Condition_ptr clone() override { return std::make_shared<LivenessCondition>(); }
        };

        class QuasiLivenessCondition : public ShallowCondition
        {
        public:
            QuasiLivenessCondition() {}
        protected:
            void _analyze(AnalysisContext& context) override;
            void _toString(std::ostream& out) const override;
            void visit(Visitor&) const override;
            Condition_ptr clone() override { return std::make_shared<QuasiLivenessCondition>(); }
        };

        class StableMarkingCondition : public ShallowCondition
        {
        public:
            StableMarkingCondition() {}
        protected:
            void _analyze(AnalysisContext& context) override;
            void _toString(std::ostream& out) const override;
            void visit(Visitor&) const override;
            Condition_ptr clone() override { return std::make_shared<StableMarkingCondition>(); }
        };

        class UpperBoundsCondition : public ShallowCondition
        {
        public:
            
            UpperBoundsCondition(const std::vector<std::string>& places) : _places(places)
            {}
        protected:
            void _toString(std::ostream& out) const override;
            void _analyze(AnalysisContext& context) override;
            void visit(Visitor&) const override;
            Condition_ptr clone() override
            {
                return std::make_shared<UpperBoundsCondition>(_places);
            }

        private:
            std::vector<std::string> _places;
        };

        class UnfoldedUpperBoundsCondition : public Condition
        {
        public:
            struct place_t {
                std::string _name;
                uint32_t _place = 0;
                double _max = std::numeric_limits<double>::infinity();
                bool _maxed_out = false;
                place_t(const std::string& name)
                {
                    _name = name;
                }
                place_t(const place_t& other, double max)
                {
                    _name = other._name;
                    _place = other._place;
                    _max = max;
                }
                bool operator<(const place_t& other) const{
                    return _place < other._place;
                }
            };
            
            UnfoldedUpperBoundsCondition(const std::vector<std::string>& places)
            {
                for(auto& s : places) _places.push_back(s);
            }
            UnfoldedUpperBoundsCondition(const std::vector<place_t>& places, double max, double offset)
                    : _places(places), _max(max), _offset(offset) {
            };
            int formulaSize() const override{
                return _places.size();
            }
            void analyze(AnalysisContext& context) override;
            size_t value(const MarkVal*);
            Result evaluate(const EvaluationContext& context) override;
            Result evalAndSet(const EvaluationContext& context) override;
            void visit(Visitor&) const override;
            uint32_t distance(DistanceContext& context) const override;
            void toString(std::ostream&) const override;
            void toTAPAALQuery(std::ostream&,TAPAALConditionExportContext& context) const override;
            void toBinary(std::ostream&) const override;
            Retval simplify(SimplificationContext& context) const override;
            bool isReachability(uint32_t depth) const override;
            Condition_ptr prepareForReachability(bool negated) const override;
            Condition_ptr pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated, bool initrw) override;
            void toXML(std::ostream&, uint32_t tabs) const override;
            void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const override;
            Quantifier getQuantifier() const override { return Quantifier::UPPERBOUNDS; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
            bool containsNext() const override { return false; }
            bool nestedDeadlock() const override { return false; }
            
            double bounds(bool add_offset = true) const { 
                return (add_offset ? _offset : 0 ) + _bound; 
            }
            
            virtual void setUpperBound(size_t bound)
            {
                _bound = std::max(_bound, bound);
            }
            
            const std::vector<place_t>& places() const { return _places; }
            
        private:
            std::vector<place_t> _places;
            size_t _bound = 0;
            double _max = std::numeric_limits<double>::infinity();
            double _offset = 0;
        };

    }
}



#endif // EXPRESSIONS_H
