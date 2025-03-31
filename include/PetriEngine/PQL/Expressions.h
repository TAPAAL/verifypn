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

        class OrCondition;
        template<>
        constexpr type_id_t type_id<OrCondition>() { return 0; }

        class AndCondition;
        template<>
        constexpr type_id_t type_id<AndCondition>() { return type_id<OrCondition>() + 1; }

        class CompareConjunction;
        template<>
        constexpr type_id_t type_id<CompareConjunction>() { return type_id<AndCondition>() + 1; }

        class LessThanCondition;
        template<>
        constexpr type_id_t type_id<LessThanCondition>() { return type_id<CompareConjunction>() + 1; }

        class LessThanOrEqualCondition;
        template<>
        constexpr type_id_t type_id<LessThanOrEqualCondition>() { return type_id<LessThanCondition>() + 1; }

        class EqualCondition;
        template<>
        constexpr type_id_t type_id<EqualCondition>() { return type_id<LessThanOrEqualCondition>() + 1; }

        class NotEqualCondition;
        template<>
        constexpr type_id_t type_id<NotEqualCondition>() { return type_id<EqualCondition>() + 1; }

        class DeadlockCondition;
        template<>
        constexpr type_id_t type_id<DeadlockCondition>() { return type_id<NotEqualCondition>() + 1; }

        class UnfoldedUpperBoundsCondition;
        template<>
        constexpr type_id_t type_id<UnfoldedUpperBoundsCondition>() { return type_id<DeadlockCondition>() + 1; }

        class NotCondition;
        template<>
        constexpr type_id_t type_id<NotCondition>() { return type_id<UnfoldedUpperBoundsCondition>() + 1; }

        class BooleanCondition;
        template<>
        constexpr type_id_t type_id<BooleanCondition>() { return type_id<NotCondition>() + 1; }

        class ECondition;
        template<>
        constexpr type_id_t type_id<ECondition>() { return type_id<BooleanCondition>() + 1; }

        class ACondition;
        template<>
        constexpr type_id_t type_id<ACondition>() { return type_id<ECondition>() + 1; }

        class FCondition;
        template<>
        constexpr type_id_t type_id<FCondition>() { return type_id<ACondition>() + 1; }

        class GCondition;
        template<>
        constexpr type_id_t type_id<GCondition>() { return type_id<FCondition>() + 1; }

        class UntilCondition;
        template<>
        constexpr type_id_t type_id<UntilCondition>() { return type_id<GCondition>() + 1; }

        class XCondition;
        template<>
        constexpr type_id_t type_id<XCondition>() { return type_id<UntilCondition>() + 1; }

        class ControlCondition;
        template<>
        constexpr type_id_t type_id<ControlCondition>() { return type_id<XCondition>() + 1; }

        class StableMarkingCondition;
        template<>
        constexpr type_id_t type_id<StableMarkingCondition>() { return type_id<ControlCondition>() + 1; }

        class QuasiLivenessCondition;
        template<>
        constexpr type_id_t type_id<QuasiLivenessCondition>() { return type_id<StableMarkingCondition>() + 1; }

        class LivenessCondition;
        template<>
        constexpr type_id_t type_id<LivenessCondition>() { return type_id<QuasiLivenessCondition>() + 1; }

        class KSafeCondition;
        template<>
        constexpr type_id_t type_id<KSafeCondition>() { return type_id<LivenessCondition>() + 1; }

        class UpperBoundsCondition;
        template<>
        constexpr type_id_t type_id<UpperBoundsCondition>() { return type_id<KSafeCondition>() + 1; }

        class FireableCondition;
        template<>
        constexpr type_id_t type_id<FireableCondition>() { return type_id<UpperBoundsCondition>() + 1; }

        class UnfoldedFireableCondition;
        template<>
        constexpr type_id_t type_id<UnfoldedFireableCondition>() { return type_id<FireableCondition>() + 1; }

        class EFCondition;
        template<>
        constexpr type_id_t type_id<EFCondition>() { return type_id<UnfoldedFireableCondition>() + 1; }

        class AGCondition;
        template<>
        constexpr type_id_t type_id<AGCondition>() { return type_id<EFCondition>() + 1; }

        class AUCondition;
        template<>
        constexpr type_id_t type_id<AUCondition>() { return type_id<AGCondition>() + 1; }

        class EUCondition;
        template<>
        constexpr type_id_t type_id<EUCondition>() { return type_id<AUCondition>() + 1; }

        class EXCondition;
        template<>
        constexpr type_id_t type_id<EXCondition>() { return type_id<EUCondition>() + 1; }

        class AXCondition;
        template<>
        constexpr type_id_t type_id<AXCondition>() { return type_id<EXCondition>() + 1; }

        class AFCondition;
        template<>
        constexpr type_id_t type_id<AFCondition>() { return type_id<AXCondition>() + 1; }

        class EGCondition;
        template<>
        constexpr type_id_t type_id<EGCondition>() { return type_id<AFCondition>() + 1; }

        class AllPaths;
        template<>
        constexpr type_id_t type_id<AllPaths>() { return type_id<EGCondition>() + 1; }

        class ExistPath;
        template<>
        constexpr type_id_t type_id<ExistPath>() { return type_id<AllPaths>() + 1; }

        class PathSelectCondition;
        template<>
        constexpr type_id_t type_id<PathSelectCondition>() { return type_id<ExistPath>() + 1; }

        class PlusExpr;
        template<>
        constexpr type_id_t type_id<PlusExpr>() { return 0; }

        class MinusExpr;
        template<>
        constexpr type_id_t type_id<MinusExpr>() { return type_id<PlusExpr>() + 1; }

        class SubtractExpr;
        template<>
        constexpr type_id_t type_id<SubtractExpr>() { return type_id<MinusExpr>() + 1; }

        class MultiplyExpr;
        template<>
        constexpr type_id_t type_id<MultiplyExpr>() { return type_id<SubtractExpr>() + 1; }

        class IdentifierExpr;
        template<>
        constexpr type_id_t type_id<IdentifierExpr>() { return type_id<MultiplyExpr>() + 1; }

        class LiteralExpr;
        template<>
        constexpr type_id_t type_id<LiteralExpr>() { return type_id<IdentifierExpr>() + 1; }

        class UnfoldedIdentifierExpr;
        template<>
        constexpr type_id_t type_id<UnfoldedIdentifierExpr>() { return type_id<LiteralExpr>() + 1; }

        class PathSelectExpr;
        template<>
        constexpr type_id_t type_id<PathSelectExpr>() { return type_id<UnfoldedIdentifierExpr>() + 1; }

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
            bool placeFree() const override;
            auto constant() const { return _constant; }
            auto& places() const { return _ids; }

        protected:
            CommutativeExpr(int64_t constant): _constant(constant) {};

            void init(std::vector<Expr_ptr>&& exprs);
            void handle(const Expr_ptr& e);
            virtual int64_t apply(int64_t, int64_t) const = 0;
            int64_t _constant;
            std::vector<std::pair<uint32_t,shared_const_string>> _ids;
            Member commutativeCons(int constant, SimplificationContext& context, std::function<void(Member& a, Member b)> op) const;
        };

        /** Binary plus expression */
        class PlusExpr : public CommutativeExpr {
        public:
            PlusExpr(std::vector<Expr_ptr>&& exprs);
            PlusExpr(std::vector<shared_const_string>&& places)
            : CommutativeExpr(int64_t{0})
            {
                _ids.reserve(places.size());
                for(auto& p : places)
                    _ids.emplace_back(0,std::move(p));
            }
            virtual type_id_t type() const final { return PQL::type_id<decltype(this)>(); };
        protected:
            int64_t apply(int64_t a, int64_t b) const { return a + b; }
            std::string op() const override;

        };

        /** Binary minus expression */
        class SubtractExpr : public NaryExpr {
        public:

            SubtractExpr(std::vector<Expr_ptr>&& exprs) : NaryExpr(std::move(exprs))
            {
            }
            virtual type_id_t type() const final { return PQL::type_id<decltype(this)>(); };

        protected:
            //int binaryOp() const;
            std::string op() const override;
        };

        /** Binary multiplication expression **/
        class MultiplyExpr : public CommutativeExpr {
        public:

            MultiplyExpr(std::vector<Expr_ptr>&& exprs);
            virtual type_id_t type() const final { return PQL::type_id<decltype(this)>(); };

        protected:
            int64_t apply(int64_t a, int64_t b) const { return a * b; }
            std::string op() const override;
        };

        class PathSelectExpr : public Expr {
        private:
            std::string _name;
            size_t _offset;
            Expr_ptr _child;
        public:
            PathSelectExpr(std::string name, Expr_ptr child, size_t offset = 0)
            : _name(name), _offset(offset), _child(child) {};
            virtual type_id_t type() const final { return PQL::type_id<decltype(this)>(); };
            [[nodiscard]] virtual bool placeFree() const { return _child->placeFree(); };
            const Expr_ptr& child() const { return _child; }
            const std::string& name() const { return _name; }
            size_t offset() const { return _offset; }
            void set_offset(size_t offset) { _offset = offset; }
        };

        /** Unary minus expression*/
        class MinusExpr : public Expr {
        public:

            MinusExpr(const Expr_ptr expr) {
                _expr = expr;
            }
            virtual type_id_t type() const final { return PQL::type_id<decltype(this)>(); };

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
            virtual type_id_t type() const final { return PQL::type_id<decltype(this)>(); };

            int64_t value() const {
                return _value;
            };
            bool placeFree() const override { return true; }
        private:
            int64_t _value;
        };


        class IdentifierExpr : public Expr {
            friend class AnalyzeVisitor;
        public:
            IdentifierExpr(shared_const_string name) : _name(name) {}
            IdentifierExpr(const IdentifierExpr&) = default;
            virtual type_id_t type() const final { return PQL::type_id<decltype(this)>(); };

            virtual bool placeFree() const override {
                if(_compiled) return _compiled->placeFree();
                return false;
            }

            [[nodiscard]] const shared_const_string &name() const {
                return _name;
            }

            [[nodiscard]] const Expr_ptr &compiled() const {
                return _compiled;
            }

        private:
            shared_const_string _name;
            Expr_ptr _compiled;
        };

        /** Identifier expression */
        class UnfoldedIdentifierExpr : public Expr {
            friend class AnalyzeVisitor;
        public:
            UnfoldedIdentifierExpr(shared_const_string name, int offest)
            : _offsetInMarking(offest), _name(name) {
            }

            UnfoldedIdentifierExpr(shared_const_string name) : UnfoldedIdentifierExpr(name, -1) {
            }

            UnfoldedIdentifierExpr(const UnfoldedIdentifierExpr&) = default;

            virtual type_id_t type() const final { return PQL::type_id<decltype(this)>(); };
            /** Offset in marking or valuation */
            int offset() const {
                return _offsetInMarking;
            }

            const shared_const_string& name() const
            {
                return _name;
            }

            bool placeFree() const override { return false; }
        private:
            /** Offset in marking, -1 if undefined, should be resolved during analysis */
            int _offsetInMarking;
            /** Identifier text */
            shared_const_string _name;
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
        protected:
            Condition_ptr _compiled = nullptr;
        };

        /* Not condition */
        class NotCondition : public Condition {
        public:

            NotCondition(const Condition_ptr cond) {
                _cond = cond;
            }

            uint32_t distance(DistanceContext& context) const override;
            Quantifier getQuantifier() const override { return Quantifier::NEG; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::LOPERATOR; }
            const Condition_ptr& operator[](size_t i) const { return _cond; };
            const Condition_ptr& getCond() const { return _cond; };
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        private:
            Condition_ptr _cond;
        };


        /******************** TEMPORAL OPERATORS ********************/

        class PathQuant : public Condition {
        private:
            std::string _id;
            size_t _offset;
            Condition_ptr _child;
        public:
            PathQuant(std::string id, std::shared_ptr<Condition> child, size_t offset = 0);
            Quantifier getQuantifier() const override { return EMPTY; }
            Path getPath() const override { return pError; }
            CTLType getQueryType() const override { return TYPE_ERROR; }
            uint32_t distance(DistanceContext& context) const override {
                if(_child)
                    return _child->distance(context);
                else
                    return 0;
            }
            const Condition_ptr& child() const { return _child; }
            const std::string& name() const { return _id; }
            size_t offset() const { return _offset; }
        };

        class AllPaths : public PathQuant {
        public:
            using PathQuant::PathQuant;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        class ExistPath : public PathQuant {
        public:
            using PathQuant::PathQuant;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        class PathSelectCondition : public Condition {
        private:
            std::string _name;
            size_t _offset;
            Condition_ptr _child;
        public:
            PathSelectCondition(std::string name, Condition_ptr child, size_t offset = 0)
            : _name(name), _offset(offset), _child(child) {}
            Quantifier getQuantifier() const override { return EMPTY; }
            Path getPath() const override { return pError; }
            CTLType getQueryType() const override { return TYPE_ERROR; }
            uint32_t distance(DistanceContext& context) const override {
                context.set_offset(_offset);
                auto fn = [this](auto& context) -> uint32_t {
                if(_child)
                    return _child->distance(context);
                else
                    return 0;
                };
                auto r = fn(context);
                context.set_offset(0);
                return r;
            }
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
            const Condition_ptr& child() const { return _child; }
            const std::string& name() const { return _name; }
            size_t offset() const { return _offset; }
            void set_offset(size_t offset) { _offset = offset; }
        };

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
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
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
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
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

            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
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

          virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
      };


      class FCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override             { return Path::F; }
            uint32_t distance(DistanceContext& context) const override {
                return _cond->distance(context);
            }
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
      };

        class XCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;


            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override             { return Path::X; }
            uint32_t distance(DistanceContext& context) const override {
                return _cond->distance(context);
            }
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        class EXCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::X; }
            uint32_t distance(DistanceContext& context) const override;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        class EGCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;


            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::G; }
            uint32_t distance(DistanceContext& context) const override;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        class EFCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;


            Quantifier getQuantifier() const override { return Quantifier::E; }
            Path getPath() const override             { return Path::F; }
            uint32_t distance(DistanceContext& context) const override;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        class AXCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::X; }
            uint32_t distance(DistanceContext& context) const override;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        class AGCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::G; }
            uint32_t distance(DistanceContext& context) const override;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        class AFCondition : public SimpleQuantifierCondition {
        public:
            using SimpleQuantifierCondition::SimpleQuantifierCondition;

            Quantifier getQuantifier() const override { return Quantifier::A; }
            Path getPath() const override             { return Path::F; }
            uint32_t distance(DistanceContext& context) const override;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
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

            uint32_t distance(DistanceContext& context) const override { return (*this)[1]->distance(context); }
            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        protected:
            Condition_ptr _cond1;
            Condition_ptr _cond2;

        };

        class EUCondition : public UntilCondition {
        public:
            using UntilCondition::UntilCondition;
            Quantifier getQuantifier() const override { return Quantifier::E; }
            uint32_t distance(DistanceContext& context) const override;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        class AUCondition : public UntilCondition {
        public:
            using UntilCondition::UntilCondition;
            Quantifier getQuantifier() const override { return Quantifier::A; }
            uint32_t distance(DistanceContext& context) const override;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        /******************** CONDITIONS ********************/

        class UnfoldedFireableCondition : public ShallowCondition {
            friend class AnalyzeVisitor;
        public:
            UnfoldedFireableCondition(shared_const_string tname) : ShallowCondition(), _name(tname) {};
            const shared_const_string& getName() const {
                return _name;
            }
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        protected:
            Condition_ptr clone() { return std::make_shared<UnfoldedFireableCondition>(_name); }
        private:
            shared_const_string _name;
        };

        class FireableCondition : public ShallowCondition {
            friend class AnalyzeVisitor;
        public:
            FireableCondition(shared_const_string tname) : _name(tname) {};
            const shared_const_string& getName() const {
                return _name;
            }
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        protected:
            Condition_ptr clone() { return std::make_shared<FireableCondition>(_name); }
        private:
            shared_const_string _name;
        };

        /* Logical conditon */
        class LogicalCondition : public Condition {
        public:
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
            friend class AnalyzeVisitor;
        };

        /* Conjunctive and condition */
        class AndCondition : public LogicalCondition {
        public:

            AndCondition(std::vector<Condition_ptr>&& conds);

            AndCondition(const std::vector<Condition_ptr>& conds);

            AndCondition(Condition_ptr left, Condition_ptr right);

            Quantifier getQuantifier() const override { return Quantifier::AND; }
            uint32_t distance(DistanceContext& context) const override;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        /* Disjunctive or conditon */
        class OrCondition : public LogicalCondition {
        public:

            OrCondition(std::vector<Condition_ptr>&& conds);

            OrCondition(const std::vector<Condition_ptr>& conds);

            OrCondition(Condition_ptr left, Condition_ptr right);

            Quantifier getQuantifier() const override { return Quantifier::OR; }
            uint32_t distance(DistanceContext& context) const override;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        class CompareConjunction : public Condition
        {
            friend class AnalyzeVisitor;
        public:
            struct cons_t {
                uint32_t _place = std::numeric_limits<uint32_t>::max();
                uint32_t _upper = std::numeric_limits<uint32_t>::max();
                uint32_t _lower = 0;
                shared_const_string _name;
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
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        private:
            std::vector<cons_t> _constraints;
            bool _negated = false;
        };

        /* Comparison conditon */
        class CompareCondition : public Condition {
        public:

            CompareCondition(const Expr_ptr expr1, const Expr_ptr expr2)
            : _expr1(expr1), _expr2(expr2) {}

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
                    std::function<uint32_t(uint32_t, uint32_t, bool)>&& d) const;
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
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
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
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
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
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
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
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
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
            uint32_t distance(DistanceContext& context) const override;
            static Condition_ptr TRUE_CONSTANT;
            static Condition_ptr FALSE_CONSTANT;
            static Condition_ptr getShared(bool val);

            Quantifier getQuantifier() const override { return Quantifier::EMPTY; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
            const bool value;
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        };

        /* Deadlock condition */
        class DeadlockCondition : public Condition {
        public:

            DeadlockCondition() = default;
            uint32_t distance(DistanceContext& context) const override;

            static Condition_ptr DEADLOCK;
            Quantifier getQuantifier() const override { return Quantifier::DEADLOCK; }
            Path getPath() const override { return Path::pError; }
            CTLType getQueryType() const override { return CTLType::EVAL; }
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
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

            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        protected:
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
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        protected:
            Condition_ptr clone() override { return std::make_shared<LivenessCondition>(); }
        };

        class QuasiLivenessCondition : public ShallowCondition
        {
            friend class AnalyzeVisitor;
        public:
            QuasiLivenessCondition() {}
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        protected:
            Condition_ptr clone() override { return std::make_shared<QuasiLivenessCondition>(); }
        };


        class StableMarkingCondition : public ShallowCondition
        {
            friend class AnalyzeVisitor;
        public:
            StableMarkingCondition() {}
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };
        protected:
            Condition_ptr clone() override { return std::make_shared<StableMarkingCondition>(); }
        };


        class UpperBoundsCondition : public ShallowCondition
        {
            friend class AnalyzeVisitor;
        public:
            UpperBoundsCondition(const std::vector<shared_const_string>& places) : _places(places)
            {}
            const std::vector<shared_const_string> &getPlaces() const {
                return _places;
            }

            Condition_ptr clone() override
            {
                return std::make_shared<UpperBoundsCondition>(_places);
            }
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); };

        private:
            std::vector<shared_const_string> _places;
        };


        class UnfoldedUpperBoundsCondition : public Condition
        {
            friend class AnalyzeVisitor;
        public:
            struct place_t {
                shared_const_string _name;
                uint32_t _place = 0;
                double _max = std::numeric_limits<double>::infinity();
                bool _maxed_out = false;
                place_t(const shared_const_string& name)
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

            UnfoldedUpperBoundsCondition(const std::vector<shared_const_string>& places)
            {
                _places.reserve(places.size());
                for(auto& s : places)
                    _places.emplace_back(s);
            }
            UnfoldedUpperBoundsCondition(const std::vector<place_t>& places, double max, double offset)
                    : _places(places), _max(max), _offset(offset) {
            };
            UnfoldedUpperBoundsCondition(const UnfoldedUpperBoundsCondition&) = default;
            size_t value(const MarkVal*);
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
            virtual type_id_t type() const { return PQL::type_id<decltype(this)>(); }
        private:
            std::vector<place_t> _places;
            size_t _bound = 0;
            double _max = std::numeric_limits<double>::infinity();
            double _offset = 0;
        };
    }
}
#endif // EXPRESSIONS_H
