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
 * File:   ArcIntervalsVisitor.h
 * Author: Peter G. Jensen
 *
 * Created on 11 February 2022, 13.27
 */

#ifndef ARCINTERVALSVISITOR_H
#define ARCINTERVALSVISITOR_H

#include "Expressions.h"


namespace PetriEngine {
    namespace Colored {

        class ArcIntervalVisitor : public ColorExpressionVisitor {
        private:
            uint32_t _index = 0;
            int32_t _modifier = 0;
            Colored::ArcIntervals& _arcIntervals;
            const PetriEngine::Colored::ColorFixpoint& _cfp;
            bool _ok = true;
        public:

            ArcIntervalVisitor(Colored::ArcIntervals& arcIntervals,const PetriEngine::Colored::ColorFixpoint& cfp)
                : _arcIntervals(arcIntervals), _cfp(cfp) {
            }

            virtual void accept(const DotConstantExpression* e)
            {
                if (_arcIntervals._intervalTupleVec.empty()) {
                    //We can add all place tokens when considering the dot constant as, that must be present
                    _arcIntervals._intervalTupleVec.push_back(_cfp.constraints);
                }
                _ok = !_cfp.constraints.empty();
            }

            virtual void accept(const VariableExpression* e)
            {
                if (_arcIntervals._intervalTupleVec.empty()){
                    //As variables does not restrict the values before the guard we include all tokens
                    _arcIntervals._intervalTupleVec.push_back(_cfp.constraints);
                }
                _ok = !_cfp.constraints.empty();
            }

            virtual void accept(const UserOperatorExpression* e)
            {
                int64_t colorId = ((int64_t)e->user_operator()->getId()) + _modifier;
                while(colorId < 0){
                    colorId += e->user_operator()->getColorType()->size();
                }
                colorId = colorId % e->user_operator()->getColorType()->size();

                if(_arcIntervals._intervalTupleVec.empty()){
                    Colored::interval_vector_t newIntervalTuple;
                    bool colorInFixpoint = false;
                    for (const auto& interval : _cfp.constraints){
                        if(interval[_index].contains(colorId)) {
                            newIntervalTuple.addInterval(interval);
                            colorInFixpoint = true;
                        }
                    }
                    _arcIntervals._intervalTupleVec.push_back(newIntervalTuple);
                    _ok = colorInFixpoint;
                } else {
                    std::vector<uint32_t> intervalsToRemove;
                    for(auto& intervalTuple : _arcIntervals._intervalTupleVec){
                        for (uint32_t i = 0; i < intervalTuple.size(); ++i){
                            if(!intervalTuple[i][_index].contains(colorId)){
                                intervalsToRemove.push_back(i);
                            }
                        }

                        for (auto i = intervalsToRemove.rbegin(); i != intervalsToRemove.rend(); ++i) {
                            intervalTuple.removeInterval(*i);
                        }
                    }
                    _ok = !_arcIntervals._intervalTupleVec[0].empty();
                }
            }

            virtual void accept(const SuccessorExpression* e)
            {
                ++_modifier;
                e->child()->visit(*this);
                --_modifier;
            }

            virtual void accept(const PredecessorExpression* e)
            {
                --_modifier;
                e->child()->visit(*this);
                ++_modifier;
            }

            virtual void accept(const TupleExpression* tup)
            {
                for (const auto& expr : *tup) {
                    expr->visit(*this);
                    if(!_ok)
                        return;
                    ++_index;
                }
            }

            virtual void accept(const LessThanExpression* lt)
            {}

            virtual void accept(const LessThanEqExpression* lte)
            {}

            virtual void accept(const EqualityExpression* eq)
            {}

            virtual void accept(const InequalityExpression* neq)
            {}

            virtual void accept(const AndExpression* andexpr)
            {}

            virtual void accept(const OrExpression* orexpr)
            {}

            virtual void accept(const AllExpression* all)
            {
                if(_arcIntervals._intervalTupleVec.empty()){
                    bool colorsInFixpoint = false;
                    Colored::interval_vector_t newIntervalTuple;
                    if(_cfp.constraints.getContainedColors() == all->sort()->size()){
                        colorsInFixpoint = true;
                        for(const auto& interval : _cfp.constraints){
                            newIntervalTuple.addInterval(interval);
                        }
                    }
                    _arcIntervals._intervalTupleVec.push_back(newIntervalTuple);
                    _ok = colorsInFixpoint;
                } else {
                    std::vector<Colored::interval_vector_t> newIntervalTupleVec;
                    for(uint32_t i = 0; i < _arcIntervals._intervalTupleVec.size(); ++i){
                        auto& intervalTuple = _arcIntervals._intervalTupleVec[i];
                        if(intervalTuple.getContainedColors() == all->sort()->size()){
                            newIntervalTupleVec.push_back(intervalTuple);
                        }
                    }
                    _arcIntervals._intervalTupleVec = std::move(newIntervalTupleVec);
                    _ok = !_arcIntervals._intervalTupleVec.empty();
                }
            }

            virtual void accept(const NumberOfExpression* e)
            {
                _index = 0;
                for (const auto& elem : *e) {
                    elem->visit(*this);
                    if(!_ok) return;
                }
            }

            virtual void accept(const AddExpression* e)
            {
                Colored::ArcIntervals old_intervals;
                std::swap(_arcIntervals, old_intervals);
                for (const auto& elem : *e) {
                    _arcIntervals = ArcIntervals();

                    elem->visit(*this);
                    if(_arcIntervals._intervalTupleVec.empty())
                        _ok = false;

                    if(!_ok)
                    {
                        std::swap(old_intervals, _arcIntervals);
                        return;
                    }

                    old_intervals._intervalTupleVec.insert(
                        old_intervals._intervalTupleVec.end(),
                            _arcIntervals._intervalTupleVec.begin(), _arcIntervals._intervalTupleVec.end());
                }
                        
                std::swap(old_intervals, _arcIntervals);
            }

            virtual void accept(const SubtractExpression* sub)
            {
                (*sub)[0]->visit(*this);
                //We ignore the restrictions imposed by the subtraction for now
                //_right->getArcIntervals(arcIntervals, cfp, &rightIndex);
            }

            virtual void accept(const ScalarProductExpression* scalar)
            {
                scalar->child()->visit(*this);
            }

            static inline bool intervals(ArcExpression& e, Colored::ArcIntervals& arcIntervals, const PetriEngine::Colored::ColorFixpoint& cfp)
            {
                ArcIntervalVisitor v(arcIntervals, cfp);
                e.visit(v);
                return v._ok;
            }
        };
    }
}


#endif /* ARCINTERVALSVISITOR_H */

