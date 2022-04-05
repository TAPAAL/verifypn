/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
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

#ifndef COLORED_EXPRESSIONS_H
#define COLORED_EXPRESSIONS_H

#include <string>
#include <unordered_map>
#include <set>
#include <stdlib.h>
#include <iostream>
#include <cassert>
#include <memory>
#include <typeinfo>


#include "Colors.h"
#include "Multiset.h"
#include "EquivalenceVec.h"
#include "ArcIntervals.h"
#include "GuardRestrictor.h"
#include "utils/errors.h"
#include "ColorExpressionVisitor.h"
#include "CExprToString.h"

namespace PetriEngine {
    class ColoredPetriNetBuilder;

    namespace Colored {
        class Expression {
        public:
            Expression() {}
            virtual void visit(ColorExpressionVisitor& visitor) const = 0;

        };

        class ColorExpression : public Expression {
        public:
            ColorExpression() {}
            virtual ~ColorExpression() {}
            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const = 0;
        };

        class DotConstantExpression : public ColorExpression {
        public:
            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override{
                return ColorType::dotInstance();
            }
        };

        typedef std::shared_ptr<ColorExpression> ColorExpression_ptr;

        class VariableExpression : public ColorExpression {
        private:
            const Variable* _variable;
        public:
            const Variable* variable() const {
                return _variable;
            }

            VariableExpression(const Variable* variable)
                    : _variable(variable) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override{
                return _variable->colorType;
            }

        };

        class UserOperatorExpression : public ColorExpression {
        private:
            const Color* _userOperator;
        public:
            const Color* user_operator() const {
                return _userOperator;
            }

            UserOperatorExpression(const Color* userOperator)
                    : _userOperator(userOperator) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override {
                return _userOperator->getColorType();
            }
        };

        class SuccessorExpression : public ColorExpression {
        private:
            ColorExpression_ptr _color;
        public:
            const ColorExpression_ptr& child() const {
                return _color;
            }

            SuccessorExpression(ColorExpression_ptr&& color)
                    : _color(std::move(color)) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override{
                return _color->getColorType(colorTypes);
            }
        };

        class PredecessorExpression : public ColorExpression {
        private:
            ColorExpression_ptr _color;
        public:
            const ColorExpression_ptr& child() const {
                return _color;
            }

            PredecessorExpression(ColorExpression_ptr&& color)
                    : _color(std::move(color)) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override{
                return _color->getColorType(colorTypes);
            }
        };

        class TupleExpression : public ColorExpression {
        private:
            std::vector<ColorExpression_ptr> _colors;
            const ColorType* _colorType = nullptr;
        public:

            size_t size() const {
                return _colors.size();
            }

            auto begin() const {
                return _colors.begin();
            }

            auto end() const {
                return _colors.end();
            }

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }

            TupleExpression(std::vector<ColorExpression_ptr>&& colors, const ColorTypeMap& colorTypes)
                    : _colors(std::move(colors)) {
                _colorType = getColorType(colorTypes);
            }

            virtual const ColorType* getColorType(const ColorTypeMap& colorTypes) const override{

                std::vector<const ColorType*> types;
                if(_colorType != nullptr){
                    return _colorType;
                }

                for (const auto& color : _colors) {
                    types.push_back(color->getColorType(colorTypes));
                }

                for (auto elem : colorTypes) {
                    auto* pt = dynamic_cast<const ProductType*>(elem.second);
                    if (pt && pt->containsTypes(types)) {
                        return pt;
                    }
                }
                assert(false);
                throw base_error("COULD NOT FIND PRODUCT TYPE");
                return nullptr;
            }
        };

        class GuardExpression : public Expression {
        public:
            virtual ~GuardExpression() {};
        };

        class CompareExpression : public GuardExpression {
        protected:
            ColorExpression_ptr _left;
            ColorExpression_ptr _right;

        public:
            CompareExpression(ColorExpression_ptr&& left, ColorExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
            size_t size() const {
                return 2;
            }

            const ColorExpression_ptr& operator[](size_t i) const {
                return (i == 0) ? _left : _right;
            }
        };

        typedef std::shared_ptr<GuardExpression> GuardExpression_ptr;

        class LessThanExpression : public CompareExpression {
        public:
            using CompareExpression::CompareExpression;
            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class LessThanEqExpression : public CompareExpression {
        public:
            using CompareExpression::CompareExpression;
            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class EqualityExpression : public CompareExpression {
        public:
            using CompareExpression::CompareExpression;
            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class InequalityExpression : public CompareExpression {
        public:
            using CompareExpression::CompareExpression;

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class LogicalExpression : public GuardExpression {
        protected:
            GuardExpression_ptr _left;
            GuardExpression_ptr _right;

        public:
            LogicalExpression(GuardExpression_ptr&& left, GuardExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}
            size_t size() const {
                return 2;
            }

            const GuardExpression_ptr& operator[](size_t i) const {
                return (i == 0) ? _left : _right;
            }

        };


        class AndExpression : public LogicalExpression {
        public:
            using LogicalExpression::LogicalExpression;
            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class OrExpression : public LogicalExpression {
        public:
            using LogicalExpression::LogicalExpression;
            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class ArcExpression : public Expression {
        public:
            ArcExpression() {}
            virtual ~ArcExpression() {}

            virtual void visit(ColorExpressionVisitor& visitor) const = 0;
            virtual uint32_t weight() const = 0;
            virtual bool is_single_color() const = 0;
        };

        typedef std::shared_ptr<ArcExpression> ArcExpression_ptr;

        class AllExpression : public Expression {
        private:
            const ColorType* _sort;
        public:
            virtual ~AllExpression() {};

            size_t size() const {
                return  _sort->size();
            }

            const ColorType* sort() const {
                return _sort;
            }

            AllExpression(const ColorType* sort) : _sort(sort)
            {
                assert(sort != nullptr);
            }

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        typedef std::shared_ptr<AllExpression> AllExpression_ptr;

        class NumberOfExpression : public ArcExpression {
        private:
            uint32_t _number;
            std::vector<ColorExpression_ptr> _color;
            AllExpression_ptr _all;
        public:
            uint32_t weight() const override {
                if (_all == nullptr)
                    return _number * _color.size();
                else
                    return _number * _all->size();
            }

            bool is_all() const {
                return (bool)_all;
            }

            bool is_single_color() const {
                return !is_all() && _color.size() == 1;
            }

            uint32_t number() const {
                return _number;
            }

            const ColorExpression_ptr& operator[](size_t i ) const {
                return _color[i];
            }

            size_t size() const {
                return _color.size();
            }

            auto begin() const {
                return _color.begin();
            }

            auto end() const {
                return _color.end();
            }

            const AllExpression_ptr& all() const {
                return _all;
            }

            NumberOfExpression(std::vector<ColorExpression_ptr>&& color, uint32_t number = 1)
                    : _number(number), _color(std::move(color)), _all(nullptr) {}
            NumberOfExpression(AllExpression_ptr&& all, uint32_t number = 1)
                    : _number(number), _color(), _all(std::move(all)) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        typedef std::shared_ptr<NumberOfExpression> NumberOfExpression_ptr;

        class AddExpression : public ArcExpression {
        private:
            std::vector<ArcExpression_ptr> _constituents;
        public:
            uint32_t weight() const override {
                uint32_t res = 0;
                for (const auto& expr : _constituents) {
                    res += expr->weight();
                }
                return res;
            }

            bool is_single_color() const {
                return _constituents.size() == 1 && _constituents[0]->is_single_color();
            }

            size_t size() const {
                return _constituents.size();
            }

            const ArcExpression_ptr& operator[](size_t i) const {
                return _constituents[i];
            }

            auto begin() const {
                return _constituents.begin();
            }

            auto end() const {
                return _constituents.end();
            }

            AddExpression(std::vector<ArcExpression_ptr> &&constituents)
                    : _constituents(std::move(constituents)) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class SubtractExpression : public ArcExpression {
        private:
            ArcExpression_ptr _left;
            ArcExpression_ptr _right;

        public:
            uint32_t weight() const override {
                return _left->weight() - _right->weight();
            }

            bool is_single_color() const {
                return false;
            }

            size_t size() const {
                return 2;
            }

            const ArcExpression_ptr& operator[](size_t i ) const {
                if(i == 0) return _left;
                else return _right;
            }


            SubtractExpression(ArcExpression_ptr&& left, ArcExpression_ptr&& right)
                    : _left(std::move(left)), _right(std::move(right)) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };

        class ScalarProductExpression : public ArcExpression {
        private:
            uint32_t _scalar;
            ArcExpression_ptr _expr;
        public:

            uint32_t weight() const override {
                return _scalar * _expr->weight();
            }

            bool is_single_color() const {
                return _expr->is_single_color();
            }

            auto scalar() const {
                return _scalar;
            }

            const ArcExpression_ptr& child() const {
                return _expr;
            }

            ScalarProductExpression(ArcExpression_ptr&& expr, uint32_t scalar)
                    : _scalar(std::move(scalar)), _expr(expr) {}

            void visit(ColorExpressionVisitor& visitor) const { visitor.accept(this); }
        };
    }
}

#endif /* COLORED_EXPRESSIONS_H */

