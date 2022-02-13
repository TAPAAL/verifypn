/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
 *                     Peter G. Jensen <root@petergjoel.dk>
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

/*
 * File:   OutputIntervalVisitor.h
 * Author: Peter G. Jensen
 *
 * Created on 11 February 2022, 09.53
 */

#ifndef OUTPUTINTERVALVISITOR_H
#define OUTPUTINTERVALVISITOR_H

#include "Expressions.h"


namespace PetriEngine {
    namespace Colored {

        class OutputIntervalVisitor : public ColorExpressionVisitor {
        private:
            Colored::interval_vector_t _result;
            std::vector<Colored::interval_vector_t> _result_v;
            const VariableIntervalMap* _varmap = nullptr;
            const std::vector<VariableIntervalMap>& _varmap_vec;
            std::vector<const Colored::ColorType *> _colortypes;
        public:

            OutputIntervalVisitor(const std::vector<VariableIntervalMap>& varmap)
            : _varmap_vec(varmap) {
            }

            virtual void accept(const DotConstantExpression*)
            {
                _result.clear();
                Colored::interval_t interval;
                const Color *dotColor = &(*ColorType::dotInstance()->begin());
                _colortypes.emplace_back(dotColor->getColorType());
                interval.addRange(dotColor->getId(), dotColor->getId());
                _result.addInterval(interval);
            }

            virtual void accept(const VariableExpression* e)
            {
                _result.clear();
                // If we see a new variable on an out arc, it gets its full interval
                if (_varmap->count(e->variable()) == 0){
                    _result.addInterval(e->variable()->colorType->getFullInterval());
                } else {
                    for(const auto& interval : _varmap->find(e->variable())->second){
                        _result.addInterval(interval);
                    }
                }

                std::vector<const ColorType*> varColorTypes;
                e->variable()->colorType->getColortypes(varColorTypes);

                for(auto &ct : varColorTypes){
                    _colortypes.push_back(std::move(ct));
                }
            }

            virtual void accept(const UserOperatorExpression* e)
            {
                _result.clear();
                Colored::interval_t interval;

                _colortypes.emplace_back(e->user_operator()->getColorType());

                interval.addRange(e->user_operator()->getId(), e->user_operator()->getId());
                _result.addInterval(interval);
            }

            virtual void accept(const SuccessorExpression* e)
            {
                //store the number of colortyps already in colortypes vector and use that as offset when indexing it
                auto colortypesBefore = _colortypes.size();
                e->child()->visit(*this);
                _result = Colored::GuardRestrictor::shiftIntervals(_colortypes, _result, 1, colortypesBefore);
            }

            virtual void accept(const PredecessorExpression* e)
            {
                auto colortypesBefore = _colortypes.size();
                e->child()->visit(*this);
                _result = Colored::GuardRestrictor::shiftIntervals(_colortypes, _result, -1, colortypesBefore);
            }

            virtual void accept(const TupleExpression* tup)
            {
                Colored::interval_vector_t intervals;

                for(const auto& colorExp : *tup) {
                    Colored::interval_vector_t intervalHolder;
                    colorExp->visit(*this);
                    if(intervals.empty()){
                        intervals = _result;
                    } else {
                        for(const auto& nested_interval : _result){
                            Colored::interval_vector_t newIntervals;
                            for(auto& interval : intervals){
                                for(const auto& nestedRange : nested_interval._ranges) {
                                    interval.addRange(nestedRange);
                                }
                                newIntervals.addInterval(interval);
                            }
                            for(const auto& newInterval : newIntervals){
                                intervalHolder.addInterval(newInterval);
                            }
                        }
                        intervals = intervalHolder;
                    }
                }
                _result = std::move(intervals);
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
                _result.clear();
                _result.addInterval(all->sort()->getFullInterval());
            }

            virtual void accept(const NumberOfExpression* no)
            {
                _result_v.clear();
                if (!no->is_all()) {
                    for (const auto& elem : *no) {
                        for(const auto& varMap : _varmap_vec){
                            _varmap = &varMap;
                            elem->visit(*this);
                            _result_v.emplace_back(_result);
                        }
                    }
                } else {
                    no->all()->visit(*this);
                    _result_v.emplace_back(_result);
                }
            }

            virtual void accept(const AddExpression* add)
            {
                std::vector<Colored::interval_vector_t> intervalsVec;

                for (const auto& elem : *add) {
                    elem->visit(*this);
                    intervalsVec.insert(intervalsVec.end(), _result_v.begin(), _result_v.end());
                }
                _result_v = std::move(intervalsVec);
            }

            virtual void accept(const SubtractExpression* sub)
            {
                //We could maybe reduce the intervals slightly by checking if the upper or lower bound is being subtracted
                (*sub)[0]->visit(*this);
            }

            virtual void accept(const ScalarProductExpression* scalar)
            {
                scalar->child()->visit(*this);
            }

            static inline std::vector<Colored::interval_vector_t> intervals(Expression& e, const std::vector<VariableIntervalMap>& varMapVec)
            {
                OutputIntervalVisitor v(varMapVec);
                e.visit(v);
                return std::move(v._result_v);
            }
        };


    }
}


#endif /* OUTPUTINTERVALVISITOR_H */

