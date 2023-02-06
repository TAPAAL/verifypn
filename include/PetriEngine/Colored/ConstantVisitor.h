/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   ConstantVisitor.h
 * Author: pgj
 *
 * Created on 11 February 2022, 11.54
 */

#ifndef CONSTANTVISITOR_H
#define CONSTANTVISITOR_H

#include "ColorExpressionVisitor.h"
#include "Colors.h"
#include "Expressions.h"

namespace PetriEngine {
    namespace Colored {

        class ConstantVisitor : public ColorExpressionVisitor {
        private:
            uint32_t _index = 0;
            PositionColorsMap _constantMap;
            std::unordered_map<uint32_t,const Color*> _color_map;
        public:
            ConstantVisitor() {}

            virtual void accept(const DotConstantExpression*)
            {
                const Color *dotColor = &(*ColorType::dotInstance()->begin());
                _color_map[_index] = dotColor;
            }

            virtual void accept(const VariableExpression* e)
            {
            }

            virtual void accept(const UserOperatorExpression* e)
            {
                _color_map[_index] = e->user_operator();
            }

            virtual void accept(const SuccessorExpression* e)
            {
                e->child()->visit(*this);
                for(auto& constIndexPair : _color_map){
                    constIndexPair.second = &++(*constIndexPair.second);
                }
            }

            virtual void accept(const PredecessorExpression* e)
            {
                e->child()->visit(*this);
                for(auto& constIndexPair : _color_map){
                    constIndexPair.second = &--(*constIndexPair.second);
                }
            }

            virtual void accept(const TupleExpression* tup)
            {
                for (const auto& elem : *tup) {
                    elem->visit(*this);
                    ++_index;
                }
            }

            virtual void accept(const LessThanExpression* lt)
            {
            }

            virtual void accept(const LessThanEqExpression* lte)
            {
            }

            virtual void accept(const EqualityExpression* eq)
            {
            }

            virtual void accept(const InequalityExpression* neq)
            {
            }

            virtual void accept(const AndExpression* andexpr)
            {
            }

            virtual void accept(const OrExpression* orexpr)
            {
            }

            virtual void accept(const AllExpression* all)
            {
                for (auto& c : *all->sort()) {
                    _constantMap[_index].push_back(&c);
                }
            }

            virtual void accept(const NumberOfExpression* no)
            {
                for (const auto& elem : *no) {
                    _color_map.clear();
                    elem->visit(*this);
                    for(const auto& pair : _color_map) {
                        _constantMap[pair.first].push_back(pair.second);
                    }
                }
            }

            virtual void accept(const AddExpression* add)
            {
                auto indexCopy = _index;
                for (const auto& elem : *add) {
                    _index = indexCopy;
                    elem->visit(*this);
                }
                _index = indexCopy;
            }

            virtual void accept(const SubtractExpression* sub)
            {
                auto rIndex = _index;
                (*sub)[1]->visit(*this);
                _index = rIndex;
                (*sub)[0]->visit(*this);
            }

            virtual void accept(const ScalarProductExpression* scalar)
            {
                scalar->child()->visit(*this);
            }

            static inline PositionColorsMap get_constants(const ArcExpression& e) {
                ConstantVisitor v;
                e.visit(v);
                return std::move(v._constantMap);
            }

            static inline PositionColorsMap get_constants(const AllExpression& e) {
                ConstantVisitor v;
                e.visit(v);
                return std::move(v._constantMap);
            }

            static inline auto get_constants(const ColorExpression& e) {

                ConstantVisitor v;
                e.visit(v);
                return std::move(v._color_map);
            }
        };
    }
}

#endif /* CONSTANTVISITOR_H */

