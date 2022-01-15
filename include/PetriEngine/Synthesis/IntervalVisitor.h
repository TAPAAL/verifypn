
/*
 * File:   IntervalVisitor.h
 * Author: Peter G. Jensen
 *
 * Created on 15 January 2022, 14.10
 */

#ifndef INTERVALVISITOR_H
#define INTERVALVISITOR_H

#include "PetriEngine/PQL/Visitor.h"
#include <cassert>


namespace PetriEngine {
    namespace Synthesis {
        class IntervalVisitor : public PQL::Visitor {
        private:
            const std::pair<uint32_t,uint32_t>* _bounds;
            std::pair<int64_t,int64_t> _bound_tos{0,0};
            enum result_e {UNKNOWN, TRUE, FALSE};
            result_e _result_tos = UNKNOWN;
        public:
            IntervalVisitor(const std::pair<uint32_t,uint32_t>* bounds)
                    : _bounds(bounds) {}
            bool stable() const { return _result_tos != UNKNOWN; }
        protected:
            virtual void _accept(const PQL::NotCondition* element)
            {
                switch(_result_tos)
                {
                    case TRUE:
                        _result_tos = FALSE;
                        break;
                    case FALSE:
                        _result_tos = TRUE;
                        break;
                    default: // nothing
                        _result_tos = UNKNOWN;
                        break;
                }
            }

            virtual void _accept(const PQL::AndCondition* element) {
                bool stable = true;
                for(auto& c : *element)
                {
                    c->visit(*this);
                    switch(_result_tos)
                    {
                        case FALSE:
                            return;
                        case TRUE:
                            continue;
                        case UNKNOWN:
                            stable = false;
                    }
                }
                if(!stable)
                    _result_tos = UNKNOWN;
                else
                    _result_tos = TRUE;
            }

            virtual void _accept(const PQL::OrCondition* element) {
                bool stable = true;
                for(auto& c : *element)
                {
                    c->visit(*this);
                    switch(_result_tos)
                    {
                        case TRUE:
                            return;
                        case FALSE:
                            continue;
                        case UNKNOWN:
                            stable = false;
                    }
                }
                if(!stable)
                    _result_tos = UNKNOWN;
                else
                    _result_tos = FALSE;
            }

            virtual void _accept(const PQL::LessThanCondition* element) {
                (*element)[0]->visit(*this);
                auto bnds = _bound_tos;
                (*element)[1]->visit(*this);
                if(bnds.second < _bound_tos.first)
                    _result_tos = TRUE;
                else if(bnds.first >= _bound_tos.second)
                    _result_tos = FALSE;
                else
                    _result_tos = UNKNOWN;
            }

            virtual void _accept(const PQL::LessThanOrEqualCondition* element) {
                (*element)[0]->visit(*this);
                auto bnds = _bound_tos;
                (*element)[1]->visit(*this);
                if(bnds.second <= _bound_tos.first)
                    _result_tos = TRUE;
                else if(bnds.first > _bound_tos.second)
                    _result_tos = FALSE;
                else
                    _result_tos = UNKNOWN;
            }

            virtual void _accept(const PQL::EqualCondition* element) {
                (*element)[0]->visit(*this);
                auto bnds = _bound_tos;
                (*element)[1]->visit(*this);
                if(bnds.second < _bound_tos.first ||
                   _bound_tos.second < bnds.first)
                {
                    // invervals do not overlap
                    _result_tos = FALSE;
                    return;
                }
                if(bnds.first == bnds.second &&
                   _bound_tos.first == _bound_tos.second &&
                   bnds.first == _bound_tos.first)
                {
                    // exactly the same always
                    _result_tos = TRUE;
                    return;
                }
                else
                {
                    _result_tos = UNKNOWN;
                }
            }

            virtual void _accept(const PQL::NotEqualCondition* element) {
                (*element)[0]->visit(*this);
                auto bnds = _bound_tos;
                (*element)[1]->visit(*this);
                if(bnds.second < _bound_tos.first ||
                   _bound_tos.second < bnds.first)
                {
                    // invervals do not overlap
                    _result_tos = TRUE;
                    return;
                }
                if(bnds.first == bnds.second &&
                   _bound_tos.first == _bound_tos.second &&
                   bnds.first == _bound_tos.first)
                {
                    // exactly the same always
                    _result_tos = FALSE;
                    return;
                }
                else
                {
                    _result_tos = UNKNOWN;
                }
            }

            virtual void _accept(const PQL::DeadlockCondition* element) {
                _result_tos = UNKNOWN;
            }

            virtual void _accept(const PQL::CompareConjunction* element) {
                _result_tos = UNKNOWN;
            }

            virtual void _accept(const PQL::UnfoldedUpperBoundsCondition* element)
            {
                _result_tos = UNKNOWN;
            }

            virtual void _accept(const PQL::BooleanCondition *element) {
                _result_tos = element->value ? TRUE : FALSE;
            }

            // Expression
            virtual void _accept(const PQL::UnfoldedIdentifierExpr *element) {
                auto bnds = _bounds[element->offset()];
                _bound_tos = std::make_pair<int64_t,int64_t>(bnds.first, bnds.second);
            }

            virtual void _accept(const PQL::LiteralExpr *element) {
                _bound_tos = std::make_pair<int64_t,int64_t>(element->value(), element->value());
            }

            virtual void _accept(const PQL::PlusExpr *element) {
                auto bounds = std::make_pair<int64_t, int64_t>(element->constant(), element->constant());
                for(auto& p : element->places())
                {
                    auto bnds = _bounds[p.first];
                    bounds.first += bnds.first;
                    bounds.second += bnds.second;
                }
                for(auto& e : element->expressions())
                {
                    e->visit(*this);
                    bounds.first += _bound_tos.first;
                    bounds.second += _bound_tos.second;
                }
                _bound_tos = bounds;
            }

            template<typename T>
            std::pair<T,T> multiply(const std::pair<T,T> a, const std::pair<T,T> b)
            {
                auto lb = a.first * b.first;
                auto ub = lb;
                for(auto v : {a.first* b.second, a.second*b.first, a.second*b.second})
                {
                    lb = std::min(lb, v);
                    ub = std::max(ub, v);
                }
                return std::make_pair(lb,ub);
            }

            virtual void _accept(const PQL::MultiplyExpr *element) {
                auto bounds = std::make_pair<int64_t, int64_t>(element->constant(), element->constant());

                for(auto& p : element->places())
                {
                    auto bnds = std::make_pair<int64_t,int64_t>(_bounds[p.first].first, _bounds[p.first].second);
                    bounds = multiply(bounds, bnds);
                }
                for(auto& e : element->expressions())
                {
                    e->visit(*this);
                    bounds = multiply(bounds, _bound_tos);
                }
                _bound_tos = bounds;
            }

            virtual void _accept(const PQL::MinusExpr *element) {
                _bound_tos.first *= -1;
                _bound_tos.second *= -1;
                std::swap(_bound_tos.first, _bound_tos.second);
            }

            virtual void _accept(const PQL::SubtractExpr *element) {
                auto bounds = std::make_pair<int64_t, int64_t>(0,0);
                bool first = true;
                for(auto& e : element->expressions())
                {
                    e->visit(*this);
                    if(first)
                    {
                        bounds.first = _bound_tos.second;
                        bounds.second = _bound_tos.first;
                        first = false;
                    }
                    else
                    {
                        bounds.first -= _bound_tos.second;
                        bounds.second -= _bound_tos.first;
                    }
                }
                _bound_tos = bounds;
            }

        };
    }
}



#endif /* INTERVALVISITOR_H */
