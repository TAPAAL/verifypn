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
#include <PetriEngine/Stubborn/StubbornSet.h>
#include "PQL.h"
#include "Contexts.h"
#include "..//Simplification/Member.h"
#include "../Simplification/LinearPrograms.h"
#include "../Simplification/Retval.h"
#include "utils/errors.h"

using namespace PetriEngine::Simplification;

namespace PetriEngine {
    namespace PQL {

        Condition_ptr makeOr(const std::vector<Condition_ptr>& cptr);
        Condition_ptr makeOr(const Condition_ptr& a, const Condition_ptr& b);
        Condition_ptr makeAnd(const std::vector<Condition_ptr>& cptr);
        Condition_ptr makeAnd(const Condition_ptr& a, const Condition_ptr& b);

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
            bool placeFree() const override;
            auto& expressions() const { return _exprs; }
            size_t operands() const { return _exprs.size(); }
            void visit(Visitor&) const override;
            void visit(MutatingVisitor& visitor) override;
            const Expr_ptr &operator[](size_t i) const {
                return _exprs[i];
            }
            virtual std::string op() const = 0;

        protected:
            std::vector<Expr_ptr> _exprs;
        };

        class PlusExpr;
        class MultiplyExpr;

        class CommutativeExpr : public NaryExpr
        {
            friend class AnalyzeVisitor;
        public:
            friend CompareCondition;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor& visitor) override;
            bool placeFree() const override;
            auto constant() const { return _constant; }
            auto& places() const { return _ids; }

        protected:
            CommutativeExpr(int constant): _constant(constant) {};
            void init(std::vector<Expr_ptr>&& exprs);
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
            bool tk = false;

            void visit(Visitor& visitor) const override;
            void visit(MutatingVisitor& visitor) override;
        protected:
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


            void visit(Visitor& visitor) const override;
            void visit(MutatingVisitor& visitor) override;
        protected:
            //int binaryOp() const;
            std::string op() const override;
        };

        /** Binary multiplication expression **/
        class MultiplyExpr : public CommutativeExpr {
        public:

            MultiplyExpr(std::vector<Expr_ptr>&& exprs);
            Expr::Types type() const override;
            Member constraint(SimplificationContext& context) const override;

            void visit(Visitor& visitor) const override;
            void visit(MutatingVisitor& visitor) override;
        protected:
            //int binaryOp() const;
            std::string op() const override;
        };

        /** Unary minus expression*/
        class MinusExpr : public Expr {
        public:

            MinusExpr(const Expr_ptr expr) {
                _expr = expr;
            }
            Expr::Types type() const override;
            Member constraint(SimplificationContext& context) const override;

            void visit(Visitor& visitor) const override;
            void visit(MutatingVisitor& visitor) override;
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
            LiteralExpr(const LiteralExpr&) = default;
            Expr::Types type() const override;

            void visit(Visitor& visitor) const override;
            void visit(MutatingVisitor& visitor) override;
            int value() const {
                return _value;
            };
            Member constraint(SimplificationContext& context) const override;
            bool placeFree() const override { return true; }
        private:
            int _value;
        };


        class IdentifierExpr : public Expr {
            friend class AnalyzeVisitor;
        public:
            IdentifierExpr(const std::string& name) : _name(name) {}
            IdentifierExpr(const IdentifierExpr&) = default;
            [[nodiscard]] Expr::Types type() const override {
                if(_compiled) return _compiled->type();
                return Expr::IdentifierExpr;
            }

            virtual bool placeFree() const override {
                if(_compiled) return _compiled->placeFree();
                return false;
            }

            Member constraint(SimplificationContext& context) const override {
                return _compiled->constraint(context);
            }

            void visit(Visitor& visitor) const override;
            void visit(MutatingVisitor& visitor) override;

            [[nodiscard]] const std::string &name() const {
                return _name;
            }

            [[nodiscard]] const Expr_ptr &compiled() const {
                return _compiled;
            }

        private:
            std::string _name;
            Expr_ptr _compiled;
        };

        /** Identifier expression */
        class UnfoldedIdentifierExpr : public Expr {
            friend class AnalyzeVisitor;
        public:
            UnfoldedIdentifierExpr(const std::string& name, int offest)
            : _offsetInMarking(offest), _name(name) {
            }

            UnfoldedIdentifierExpr(const std::string& name) : UnfoldedIdentifierExpr(name, -1) {
            }

            UnfoldedIdentifierExpr(const UnfoldedIdentifierExpr&) = default;

            Expr::Types type() const override;
            /** Offset in marking or valuation */
            int offset() const {
                return _offsetInMarking;
            }
            const std::string& name() const
            {
                return _name;
            }
            Member constraint(SimplificationContext& context) const override;
            bool placeFree() const override { return false; }
            void visit(Visitor& visitor) const override;
            void visit(MutatingVisitor& visitor) override;
        private:
            /** Offset in marking, -1 if undefined, should be resolved during analysis */
            int _offsetInMarking;
            /** Identifier text */
            std::string _name;
        };

        class ShallowCondition : public Condition
        {
            uint32_t distance(DistanceContext& context) const override
            { return _compiled->distance(context); }

            Quantifier getQuantifier() const override
            { return _compiled->getQuantifier(); }
            Path getPath() const override { return _compiled->getPath(); }
            CTLType getQueryType() const override { return _compiled->getQueryType(); }
        public:
            const Condition_ptr &getCompiled() const {
                return _compiled;
            }
            virtual Condition_ptr clone() = 0;

            void visit(Visitor& visitor) const override;
            void visit(MutatingVisitor& visitor) override;

        protected:
            Condition_ptr _compiled = nullptr;
        };

        /* Not condition */
        class NotCondition : public Condition {
        public:

            NotCondition(const Condition_ptr cond) {
                _cond = cond;
            }

            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            uint32_t distance(DistanceContext& context) const override;
            Quantifier getQuantifier() const override { return Quantifier::NEG; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::LOPERATOR; }
            const Condition_ptr& operator[](size_t i) const { return _cond; };
            const Condition_ptr& getCond() const { return _cond; };
        private:
            Condition_ptr _cond;
        };


        /******************** TEMPORAL OPERATORS ********************/

        class QuantifierCondition : public Condition
        {
        public:
            CTLType getQueryType() const override { return CTLType::PATHQEURY; }
            virtual const Condition_ptr& operator[] (size_t i) const = 0;
        };

        class SimpleQuantifierCondition : public QuantifierCondition {
        public:
            template<typename T>
            SimpleQuantifierCondition(std::shared_ptr<T> cond) {
                assert(cond);
                static_assert(
                    std::is_base_of<Condition, T>::value,
                    "T must be a descendant of Condition"
                );
                _cond = std::dynamic_pointer_cast<Condition>(cond);
            }

            void visit(Visitor&) const override;
            void visit(MutatingVisitor& visitor) override;

            virtual const Condition_ptr& operator[] (size_t i) const override { return _cond;}
            const Condition_ptr& getCond() const { return _cond; }
        protected:
            Condition_ptr _cond;
        };

        // technically quantifies over strategies
        class ControlCondition : public SimpleQuantifierCondition
        {
            using SimpleQuantifierCondition::SimpleQuantifierCondition;
            Quantifier getQuantifier() const override { return Quantifier::BControl; }
            Path getPath() const override             { return Path::PControl; }
            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

        class ECondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::pError; }
            uint32_t distance(DistanceContext& context) const override {
                // TODO implement
                assert(false); throw base_error("TODO implement");
            }
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

      class ACondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;


            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::pError; }
            uint32_t distance(DistanceContext& context) const override {
                uint32_t retval = _cond->distance(context);
                return retval;
            }

            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

      class GCondition : public SimpleQuantifierCondition {
      public:
          using SimpleQuantifierCondition::SimpleQuantifierCondition;

          Quantifier getQuantifier() const override { return Quantifier::EMPTY; }

          Path getPath() const override { return Path::G; }

          uint32_t distance(DistanceContext &context) const override {
              context.negate();
              uint32_t retval = _cond->distance(context);
              context.negate();
              return retval;
          }

          void visit(Visitor &) const override;
          void visit(MutatingVisitor &) override;

      };

      class FCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override             { return Path::F; }
            uint32_t distance(DistanceContext& context) const override {
                return _cond->distance(context);
            }
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

        class XCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;


            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override             { return Path::X; }
            uint32_t distance(DistanceContext& context) const override {
                return _cond->distance(context);
            }
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

        class EXCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::X; }
            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

        class EGCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;


            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::G; }
            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

        class EFCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;


            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::F; }
            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

        class AXCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::X; }
            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

        class AGCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::G; }
            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

        class AFCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::F; }
            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        };

        class UntilCondition : public QuantifierCondition {
        public:
            UntilCondition(const Condition_ptr cond1, const Condition_ptr cond2) {
                _cond1 = cond1;
                _cond2 = cond2;
            }

            [[nodiscard]] virtual const Condition_ptr& operator[] (size_t i) const override
            { if(i == 0) return _cond1; return _cond2;}
            Path getPath() const override { return Path::U; }

            [[nodiscard]] const Condition_ptr& getCond1() const { return (*this)[0]; }
            [[nodiscard]] const Condition_ptr& getCond2() const { return (*this)[1]; }

            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            uint32_t distance(DistanceContext& context) const override { return (*this)[1]->distance(context); }
            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
        protected:
            Condition_ptr _cond1;
            Condition_ptr _cond2;

        };

        class EUCondition : public UntilCondition {
        public:
            using UntilCondition::UntilCondition;
            Quantifier getQuantifier() const override { return Quantifier::E; }
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            uint32_t distance(DistanceContext& context) const override;
        };

        class AUCondition : public UntilCondition {
        public:
            using UntilCondition::UntilCondition;
            Quantifier getQuantifier() const override { return Quantifier::A; }
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            uint32_t distance(DistanceContext& context) const override;
        };

        /******************** CONDITIONS ********************/

        class UnfoldedFireableCondition : public ShallowCondition {
            friend class AnalyzeVisitor;
        public:
            UnfoldedFireableCondition(const std::string& tname) : ShallowCondition(), _name(tname) {};
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            std::string getName() const {
                return _name;
            }

        protected:
            Condition_ptr clone() { return std::make_shared<UnfoldedFireableCondition>(_name); }
        public:

        private:
            const std::string _name;
        };

        class FireableCondition : public ShallowCondition {
            friend class AnalyzeVisitor;
        public:
            FireableCondition(const std::string& tname) : _name(tname) {};
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            std::string getName() const {
                return _name;
            }

        protected:
            Condition_ptr clone() { return std::make_shared<FireableCondition>(_name); }
        private:
            const std::string _name;
        };

        /* Logical conditon */
        class LogicalCondition : public Condition {
        public:

            void visit(Visitor&) const override;
            void visit(MutatingVisitor& visitor) override;
            const Condition_ptr& operator[](size_t i) const
            {
                return _conds[i];
            };
            size_t operands() const {
                return _conds.size();
            }
            const std::vector<Condition_ptr>& getOperands() const { return _conds; }
            CTLType getQueryType() const override { return CTLType::LOPERATOR; }
            Path getPath() const override         { return Path::pError; }

            auto begin() { return _conds.begin(); }
            auto end() { return _conds.end(); }
            auto begin() const { return _conds.begin(); }
            auto end() const { return _conds.end(); }
            bool empty() const { return _conds.size() == 0; }
            bool singular() const { return _conds.size() == 1; }
            size_t size() const { return _conds.size(); }

        protected:
            LogicalCondition() {};
        private:

        protected:
            std::vector<Condition_ptr> _conds;
        };

        /* Conjunctive and condition */
        class AndCondition : public LogicalCondition {
        public:

            AndCondition(std::vector<Condition_ptr>&& conds);

            AndCondition(const std::vector<Condition_ptr>& conds);

            AndCondition(Condition_ptr left, Condition_ptr right);

            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            Quantifier getQuantifier() const override { return Quantifier::AND; }
            uint32_t distance(DistanceContext& context) const override;
        };

        /* Disjunctive or conditon */
        class OrCondition : public LogicalCondition {
        public:

            OrCondition(std::vector<Condition_ptr>&& conds);

            OrCondition(const std::vector<Condition_ptr>& conds);

            OrCondition(Condition_ptr left, Condition_ptr right);

            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;

            Quantifier getQuantifier() const override { return Quantifier::OR; }
            uint32_t distance(DistanceContext& context) const override;
        };

        class CompareConjunction : public Condition
        {
            friend class AnalyzeVisitor;
        public:
            struct cons_t {
                uint32_t _place = std::numeric_limits<uint32_t>::max();
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

            uint32_t distance(DistanceContext& context) const override;
            CTLType getQueryType() const override { return CTLType::LOPERATOR; }
            Path getPath() const override         { return Path::pError; }
            void visit(Visitor&) const override;
            void visit(MutatingVisitor &visitor) override;

            Quantifier getQuantifier() const override { return _negated ? Quantifier::OR : Quantifier::AND; }
            bool isNegated() const { return _negated; }
            bool singular() const
            {
                return _constraints.size() == 1 &&
                                    (_constraints[0]._lower == 0 ||
                                     _constraints[0]._upper == std::numeric_limits<uint32_t>::max());
            };
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

            void visit(Visitor&) const override;
            void visit(MutatingVisitor& visitor) override;
            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
            const Expr_ptr& operator[](uint32_t id) const
            {
                if(id == 0) return _expr1;
                else return _expr2;
            }
            bool isTrivial() const;

            [[nodiscard]] const Expr_ptr &getExpr1() const { return _expr1; }

            [[nodiscard]] const Expr_ptr &getExpr2() const { return _expr2; }

            virtual std::string op() const = 0;

            /** Operator when exported to TAPAAL */
            virtual std::string opTAPAAL() const = 0;

            /** Swapped operator when exported to TAPAAL, e.g. operator when operands are swapped */
            virtual std::string sopTAPAAL() const = 0;

        protected:
            uint32_t _distance(DistanceContext& c,
                    std::function<uint32_t(uint32_t, uint32_t, bool)> d) const;
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

            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        private:
            std::string op() const override;
            std::string opTAPAAL() const override;
            std::string sopTAPAAL() const override;
        };

        /* None equality conditon */
        class NotEqualCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        private:
            std::string op() const override;
            std::string opTAPAAL() const override;
            std::string sopTAPAAL() const override;
        };

        /* Less-than conditon */
        class LessThanCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;
            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        private:
            std::string op() const override;
            std::string opTAPAAL() const override;
            std::string sopTAPAAL() const override;
        };

        /* Less-than-or-equal conditon */
        class LessThanOrEqualCondition : public CompareCondition {
        public:

            using CompareCondition::CompareCondition;

            uint32_t distance(DistanceContext& context) const override;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        private:
            std::string op() const override;
            std::string opTAPAAL() const override;
            std::string sopTAPAAL() const override;
        };

        /* Bool condition */
        class BooleanCondition : public Condition {
        public:

            BooleanCondition(bool value) : value(value) {
                if (value) {
                    trivial = 1;
                } else {
                    trivial = 2;
                }
            }
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            uint32_t distance(DistanceContext& context) const override;
            static Condition_ptr TRUE_CONSTANT;
            static Condition_ptr FALSE_CONSTANT;
            static Condition_ptr getShared(bool val);

            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
            const bool value;
        };

        /* Deadlock condition */
        class DeadlockCondition : public Condition {
        public:

            DeadlockCondition() = default;
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            uint32_t distance(DistanceContext& context) const override;

            static Condition_ptr DEADLOCK;
            Quantifier getQuantifier() const override { return Quantifier::DEADLOCK; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
        };

        class KSafeCondition : public ShallowCondition
        {
            friend class AnalyzeVisitor;
        public:
            KSafeCondition(const Expr_ptr& expr1) : _bound(expr1)
            {}

            const Expr_ptr &getBound() const {
                return _bound;
            }


        protected:
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            Condition_ptr clone() override
            {
                return std::make_shared<KSafeCondition>(_bound);
            }
        private:
            Expr_ptr _bound = nullptr;
        };

        class LivenessCondition : public ShallowCondition
        {
            friend class AnalyzeVisitor;
        public:
            LivenessCondition() {}
        protected:
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            Condition_ptr clone() override { return std::make_shared<LivenessCondition>(); }
        };

        class QuasiLivenessCondition : public ShallowCondition
        {
            friend class AnalyzeVisitor;
        public:
            QuasiLivenessCondition() {}
        protected:
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            Condition_ptr clone() override { return std::make_shared<QuasiLivenessCondition>(); }
        };

        class StableMarkingCondition : public ShallowCondition
        {
            friend class AnalyzeVisitor;
        public:
            StableMarkingCondition() {}
        protected:
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            Condition_ptr clone() override { return std::make_shared<StableMarkingCondition>(); }
        };

        class UpperBoundsCondition : public ShallowCondition
        {
            friend class AnalyzeVisitor;
        public:
            UpperBoundsCondition(const std::vector<std::string>& places) : _places(places)
            {}
            const std::vector<std::string> &getPlaces() const {
                return _places;
            }

            Condition_ptr clone() override
            {
                return std::make_shared<UpperBoundsCondition>(_places);
            }


        protected:
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
        private:
            std::vector<std::string> _places;
        };

        class UnfoldedUpperBoundsCondition : public Condition
        {
            friend class AnalyzeVisitor;
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
            UnfoldedUpperBoundsCondition(const UnfoldedUpperBoundsCondition&) = default;
            size_t value(const MarkVal*);
            void visit(Visitor&) const override;
            void visit(MutatingVisitor&) override;
            uint32_t distance(DistanceContext& context) const override;

            Quantifier getQuantifier() const override { return Quantifier::UPPERBOUNDS; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }

            double bounds(bool add_offset = true) const {
                return (add_offset ? _offset : 0 ) + _bound;
            }

            virtual void setUpperBound(size_t bound)
            {
                _bound = std::max(_bound, bound);
            }

            const std::vector<place_t>& places() const { return _places; }

            double getMax() const { return _max; }
            double getOffset() const { return _offset; }
            double getBound() const { return _bound; }

        private:
            std::vector<place_t> _places;
            size_t _bound = 0;
            double _max = std::numeric_limits<double>::infinity();
            double _offset = 0;
        };

    }
}



#endif // EXPRESSIONS_H
