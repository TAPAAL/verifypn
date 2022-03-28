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
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/PQL/Expressions.h"
#include "utils/errors.h"
#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/PQL/MutatingVisitor.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
#include "PetriEngine/PQL/Evaluation.h"
#include "PetriEngine/PQL/QueryPrinter.h"

#include <sstream>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <set>
#include <cmath>
#include <numeric>

using namespace PetriEngine::Simplification;

namespace PetriEngine {
    namespace PQL {

        /** FOR COMPILING AND CONSTRUCTING LOGICAL OPERATORS **/
        template<typename T>
        void tryMerge(std::vector<Condition_ptr>& _conds, const Condition_ptr& ptr, bool aggressive = false)
        {
            if(auto lor = std::dynamic_pointer_cast<T>(ptr))
            {
                for(auto& c : *lor) tryMerge<T>(_conds, c, aggressive);
            }
            else if (!aggressive)
            {
                _conds.emplace_back(ptr);
            }
            else if (auto comp = std::dynamic_pointer_cast<CompareCondition>(ptr))
            {
                if((std::is_same<T, AndCondition>::value && std::dynamic_pointer_cast<NotEqualCondition>(ptr)) ||
                   (std::is_same<T, OrCondition>::value && std::dynamic_pointer_cast<EqualCondition>(ptr)))
                {
                    _conds.emplace_back(ptr);
                }
                else
                {
                    if(! ((dynamic_cast<UnfoldedIdentifierExpr*>((*comp)[0].get()) && (*comp)[1]->placeFree()) ||
                          (dynamic_cast<UnfoldedIdentifierExpr*>((*comp)[1].get()) && (*comp)[0]->placeFree())))
                    {
                        _conds.emplace_back(ptr);
                        return;
                    }

                    std::vector<Condition_ptr> cnds{ptr};
                    auto cmp = std::make_shared<CompareConjunction>(cnds, std::is_same<T, OrCondition>::value);
                    tryMerge<T>(_conds, cmp, aggressive);
                }
            }
            else if (auto conj = std::dynamic_pointer_cast<CompareConjunction>(ptr))
            {
                if((std::is_same<T, OrCondition>::value  && ( conj->isNegated() || conj->singular())) ||
                   (std::is_same<T, AndCondition>::value && (!conj->isNegated() || conj->singular())))
                {
                    if(auto lc = std::dynamic_pointer_cast<CompareConjunction>(_conds.size() == 0 ? nullptr : _conds[0]))
                    {
                        if(lc->isNegated() == std::is_same<T, OrCondition>::value)
                        {
                            auto cpy = std::make_shared<CompareConjunction>(*lc);
                            cpy->merge(*conj);
                            _conds[0] = cpy;
                        }
                        else
                        {
                            if(conj->isNegated() == std::is_same<T, OrCondition>::value)
                                _conds.insert(_conds.begin(), conj);
                            else
                            {
                                auto next = std::make_shared<CompareConjunction>(std::is_same<T, OrCondition>::value);
                                next->merge(*conj);
                                _conds.insert(_conds.begin(), next);
                            }
                        }
                    }
                    else
                    {
                        _conds.insert(_conds.begin(), conj);
                    }
                }
                else
                {
                    _conds.emplace_back(ptr);
                }
            }
            else
            {
                _conds.emplace_back(ptr);
            }

        }

        template<typename T, bool K>
        Condition_ptr makeLog(const std::vector<Condition_ptr>& conds, bool aggressive)
        {
            if(conds.size() == 0)
                return BooleanCondition::getShared(K);
            if(conds.size() == 1) return conds[0];

            std::vector<Condition_ptr> cnds;
            for(auto& c : conds) tryMerge<T>(cnds, c, aggressive);
            auto res = std::make_shared<T>(cnds);
            if(res->singular()) return *res->begin();
            if(res->empty())
                return BooleanCondition::getShared(K);
            return res;
        }


        Condition_ptr makeOr(const std::vector<Condition_ptr>& cptr) {
            return makeLog<OrCondition,false>(cptr, true);
        }

        Condition_ptr makeOr(const Condition_ptr& a, const Condition_ptr& b) {
            std::vector<Condition_ptr> cnds{a,b};
            return makeLog<OrCondition,false>(cnds, true);
        }

        Condition_ptr makeAnd(const std::vector<Condition_ptr> &cptr) {
            return makeLog<AndCondition,true>(cptr, true);
        }

        Condition_ptr makeAnd(const Condition_ptr& a, const Condition_ptr& b) {

            std::vector<Condition_ptr> cnds{a,b};
            return makeLog<AndCondition,true>(cnds, true);
        }

        // CONSTANTS
        Condition_ptr BooleanCondition::FALSE_CONSTANT = std::make_shared<BooleanCondition>(false);
        Condition_ptr BooleanCondition::TRUE_CONSTANT = std::make_shared<BooleanCondition>(true);
        Condition_ptr DeadlockCondition::DEADLOCK = std::make_shared<DeadlockCondition>();

        Condition_ptr BooleanCondition::getShared(bool val)
        {
            if(val)
            {
                return TRUE_CONSTANT;
            }
            else
            {
                return FALSE_CONSTANT;
            }
        }

        /******************** opTAPAAL ********************/

        std::string EqualCondition::opTAPAAL() const {
            return "=";
        }

        std::string NotEqualCondition::opTAPAAL() const {
            return "=";
        } //Handled with hack in NotEqualCondition::toTAPAALQuery

        std::string LessThanCondition::opTAPAAL() const {
            return "<";
        }

        std::string LessThanOrEqualCondition::opTAPAAL() const {
            return "<=";
        }

        std::string EqualCondition::sopTAPAAL() const {
            return "=";
        }

        std::string NotEqualCondition::sopTAPAAL() const {
            return "=";
        } //Handled with hack in NotEqualCondition::toTAPAALQuery

        std::string LessThanCondition::sopTAPAAL() const {
            return ">=";
        }

        std::string LessThanOrEqualCondition::sopTAPAAL() const {
            return ">";
        }

        size_t UnfoldedUpperBoundsCondition::value(const MarkVal* marking)
        {
            size_t tmp = 0;
            for(auto& p : _places)
            {
                auto val = marking[p._place];
                p._maxed_out = (p._max <= val);
                tmp += val;
            }
            return tmp;
        }

        /******************** Op (BinaryExpr subclasses) ********************/

        std::string PlusExpr::op() const {
            return "+";
        }

        std::string SubtractExpr::op() const {
            return "-";
        }

        std::string MultiplyExpr::op() const {
            return "*";
        }

        /******************** Op (CompareCondition subclasses) ********************/

        std::string EqualCondition::op() const {
            return "==";
        }

        std::string NotEqualCondition::op() const {
            return "!=";
        }

        std::string LessThanCondition::op() const {
            return "<";
        }

        std::string LessThanOrEqualCondition::op() const {
            return "<=";
        }

        /******************** free of places ********************/

        bool NaryExpr::placeFree() const
        {
            for(auto& e : _exprs)
                if(!e->placeFree())
                    return false;
            return true;
        }

        bool CommutativeExpr::placeFree() const
        {
            if(_ids.size() > 0) return false;
            return NaryExpr::placeFree();
        }

        bool MinusExpr::placeFree() const
        {
            return _expr->placeFree();
        }

        /******************** Distance Condition ********************/
        template<>
        uint32_t delta<EqualCondition>(int v1, int v2, bool negated) {
            if (!negated)
                return std::abs(v1 - v2);
            else
                return v1 == v2 ? 1 : 0;
        }

        template<>
        uint32_t delta<NotEqualCondition>(int v1, int v2, bool negated) {
            return delta<EqualCondition>(v1, v2, !negated);
        }

        template<>
        uint32_t delta<LessThanCondition>(int v1, int v2, bool negated) {
            if (!negated)
                return v1 < v2 ? 0 : v1 - v2 + 1;
            else
                return v1 >= v2 ? 0 : v2 - v1;
        }

        template<>
        uint32_t delta<LessThanOrEqualCondition>(int v1, int v2, bool negated) {
            if (!negated)
                return v1 <= v2 ? 0 : v1 - v2;
            else
                return v1 > v2 ? 0 : v2 - v1 + 1;
        }

        uint32_t ControlCondition::distance(DistanceContext& context) const {
            throw base_error("Computing distance on a control-expression");
        }

        uint32_t NotCondition::distance(DistanceContext& context) const {
            context.negate();
            uint32_t retval = _cond->distance(context);
            context.negate();
            return retval;
        }

        uint32_t BooleanCondition::distance(DistanceContext& context) const {
            if (context.negated() != value)
                return 0;
            return std::numeric_limits<uint32_t>::max();
        }

        uint32_t DeadlockCondition::distance(DistanceContext& context) const {
            return 0;
        }

        uint32_t UnfoldedUpperBoundsCondition::distance(DistanceContext& context) const
        {
            size_t tmp = 0;
            for(auto& p : _places)
            {
                tmp += context.marking()[p._place];
            }

            return _max - tmp;
        }

        uint32_t EFCondition::distance(DistanceContext& context) const {
            return _cond->distance(context);
        }

        uint32_t EGCondition::distance(DistanceContext& context) const {
            return _cond->distance(context);
        }

        uint32_t EXCondition::distance(DistanceContext& context) const {
            return _cond->distance(context);
        }

        uint32_t EUCondition::distance(DistanceContext& context) const {
            return _cond2->distance(context);
        }

        uint32_t AFCondition::distance(DistanceContext& context) const {
            context.negate();
            uint32_t retval = _cond->distance(context);
            context.negate();
            return retval;
        }

        uint32_t AXCondition::distance(DistanceContext& context) const {
            context.negate();
            uint32_t retval = _cond->distance(context);
            context.negate();
            return retval;
        }

        uint32_t AGCondition::distance(DistanceContext& context) const {
            context.negate();
            uint32_t retval = _cond->distance(context);
            context.negate();
            return retval;
        }

        uint32_t AUCondition::distance(DistanceContext& context) const {
            context.negate();
            auto r1 = _cond1->distance(context);
            auto r2 = _cond2->distance(context);
            context.negate();
            return r1 + r2;
        }

        uint32_t CompareConjunction::distance(DistanceContext& context) const {
            uint32_t d = 0;
            auto neg = context.negated() != _negated;
            if(!neg)
            {
                for(auto& c : _constraints)
                {
                    auto pv = context.marking()[c._place];
                    d += (c._upper == std::numeric_limits<uint32_t>::max() ? 0 : delta<LessThanOrEqualCondition>(pv, c._upper, neg)) +
                         (c._lower == 0 ? 0 : delta<LessThanOrEqualCondition>(c._lower, pv, neg));
                }
            }
            else
            {
                bool first = true;
                for(auto& c : _constraints)
                {
                    auto pv = context.marking()[c._place];
                    if(c._upper != std::numeric_limits<uint32_t>::max())
                    {
                        auto d2 = delta<LessThanOrEqualCondition>(pv, c._upper, neg);
                        if(first) d = d2;
                        else      d = std::min(d, d2);
                        first = false;
                    }

                    if(c._lower != 0)
                    {
                        auto d2 = delta<LessThanOrEqualCondition>(c._upper, pv, neg);
                        if(first) d = d2;
                        else      d = std::min(d, d2);
                        first = false;
                    }
                }
            }
            return d;
        }

        uint32_t conjDistance(DistanceContext& context, const std::vector<Condition_ptr>& conds)
        {
            uint32_t val = 0;
            for(auto& c : conds)
                val += c->distance(context);
            return val;
        }

        uint32_t disjDistance(DistanceContext& context, const std::vector<Condition_ptr>& conds)
        {
            uint32_t val = std::numeric_limits<uint32_t>::max();
            for(auto& c : conds)
                val = std::min(c->distance(context), val);
            return val;
        }

        uint32_t AndCondition::distance(DistanceContext& context) const {
            if(context.negated())
                return disjDistance(context, _conds);
            else
                return conjDistance(context, _conds);
        }

        uint32_t OrCondition::distance(DistanceContext& context) const {
            if(context.negated())
                return conjDistance(context, _conds);
            else
                return disjDistance(context, _conds);
        }


        struct S {
            int d;
            unsigned int p;
        };

        uint32_t LessThanOrEqualCondition::distance(DistanceContext& context) const {
            return _distance(context, delta<LessThanOrEqualCondition>);
        }

        uint32_t LessThanCondition::distance(DistanceContext& context) const {
            return _distance(context, delta<LessThanCondition>);
        }

        uint32_t NotEqualCondition::distance(DistanceContext& context) const {
            return _distance(context, delta<NotEqualCondition>);
        }

        uint32_t EqualCondition::distance(DistanceContext& context) const {
            return _distance(context, delta<EqualCondition>);
        }

        /********************** CONSTRUCTORS *********************************/

        void postMerge(std::vector<Condition_ptr>& conds) {
            std::sort(std::begin(conds), std::end(conds),
                    [](auto& a, auto& b) {
                        return isTemporal(a) < isTemporal(b);
                    });
        }

        AndCondition::AndCondition(std::vector<Condition_ptr>&& conds) {
            for (auto& c : conds) tryMerge<AndCondition>(_conds, c);
            postMerge(_conds);
        }

        AndCondition::AndCondition(const std::vector<Condition_ptr>& conds) {
            for (auto& c : conds) tryMerge<AndCondition>(_conds, c);
            postMerge(_conds);
        }

        AndCondition::AndCondition(Condition_ptr left, Condition_ptr right) {
            tryMerge<AndCondition>(_conds, left);
            tryMerge<AndCondition>(_conds, right);
            postMerge(_conds);
        }

        OrCondition::OrCondition(std::vector<Condition_ptr>&& conds) {
            for (auto& c : conds) tryMerge<OrCondition>(_conds, c);
            postMerge(_conds);
        }

        OrCondition::OrCondition(const std::vector<Condition_ptr>& conds) {
            for (auto& c : conds) tryMerge<OrCondition>(_conds, c);
            postMerge(_conds);
        }

        OrCondition::OrCondition(Condition_ptr left, Condition_ptr right) {
            tryMerge<OrCondition>(_conds, left);
            tryMerge<OrCondition>(_conds, right);
            postMerge(_conds);
        }


        CompareConjunction::CompareConjunction(const std::vector<Condition_ptr>& conditions, bool negated)
        {
            _negated = negated;
            merge(conditions, negated);
        }

        void CompareConjunction::merge(const CompareConjunction& other)
        {
            auto neg = _negated != other._negated;
            if(neg && other._constraints.size() > 1)
            {
                throw base_error("MERGE OF CONJUNCT AND DISJUNCT NOT ALLOWED");
            }
            auto il = _constraints.begin();
            for(auto c : other._constraints)
            {
                if(neg)
                    c.invert();

                if(c._upper == std::numeric_limits<uint32_t>::max() && c._lower == 0)
                {
                    continue;
                }
                else if (c._upper != std::numeric_limits<uint32_t>::max() && c._lower != 0 && neg)
                {
                    throw base_error("MERGE OF CONJUNCT AND DISJUNCT NOT ALLOWED");
                }

                il = std::lower_bound(_constraints.begin(), _constraints.end(), c);
                if(il == _constraints.end() || il->_place != c._place)
                {
                    il = _constraints.insert(il, c);
                }
                else
                {
                    il->_lower = std::max(il->_lower, c._lower);
                    il->_upper = std::min(il->_upper, c._upper);
                }
            }
        }

        void CompareConjunction::merge(const std::vector<Condition_ptr>& conditions, bool negated)
        {
            for(auto& c : conditions)
            {
                auto cmp = dynamic_cast<CompareCondition*>(c.get());
                assert(cmp);
                auto id = dynamic_cast<UnfoldedIdentifierExpr*>((*cmp)[0].get());
                uint32_t val;
                bool inverted = false;
                EvaluationContext context;
                if(!id)
                {
                    id = dynamic_cast<UnfoldedIdentifierExpr*>((*cmp)[1].get());
                    val = PetriEngine::PQL::evaluate((*cmp)[0].get(), context);
                    inverted = true;
                }
                else
                {
                    val = PetriEngine::PQL::evaluate((*cmp)[1].get(), context);
                }
                assert(id);
                cons_t next;
                next._place = id->offset();

                if(dynamic_cast<LessThanOrEqualCondition*>(c.get()))
                    if(inverted) next._lower = val;
                    else         next._upper = val;
                else if(dynamic_cast<LessThanCondition*>(c.get()))
                    if(inverted) next._lower = val+1;
                    else         next._upper = val-1;
                else if(dynamic_cast<EqualCondition*>(c.get()))
                {
                    assert(!negated);
                    next._lower = val;
                    next._upper = val;
                }
                else if(dynamic_cast<NotEqualCondition*>(c.get()))
                {
                    assert(negated);
                    next._lower = val;
                    next._upper = val;
                    negated = false; // we already handled negation here!
                }
                else
                {
                    throw base_error("UNKNOWN");
                }
                if(negated)
                    next.invert();

                auto lb = std::lower_bound(std::begin(_constraints), std::end(_constraints), next);
                if(lb == std::end(_constraints) || lb->_place != next._place)
                {
                    next._name = id->name();
                    _constraints.insert(lb, next);
                }
                else
                {
                    assert(*id->name() == *lb->_name);
                    lb->intersect(next);
                }
            }
        }

        void CommutativeExpr::handle(const Expr_ptr& e)
        {
            if (e->placeFree()) {
                EvaluationContext c;
                _constant = apply(_constant, evaluate(e.get(), c));
            } else if (auto id = std::dynamic_pointer_cast<PQL::IdentifierExpr>(e)) {
                if (id->compiled()) {
                    handle(id->compiled());
                }
                else
                {
                    _exprs.emplace_back(std::move(e));
                }
            } else if (auto id = std::dynamic_pointer_cast<PQL::UnfoldedIdentifierExpr>(e)) {
                _ids.emplace_back(id->offset(), id->name());
            } else if (e->type() == type_id<MultiplyExpr>() || e->type() == type_id<PlusExpr>()) {
                auto* c = static_cast<CommutativeExpr*> (e.get());
                // TODO, we can do more arithmetic rewrites here, in particular
                // when the child only has a single non-const element.
                if (c->_ids.size() == 0 && c->_exprs.size() == 0) {
                    _constant = apply(_constant, c->_constant);
                } else if (c->type() == type()) {
                    _constant = apply(_constant, c->_constant);
                    _exprs.insert(_exprs.end(), c->_exprs.begin(), c->_exprs.end());
                    _ids.insert(_ids.end(), c->_ids.begin(), c->_ids.end());
                } else {
                    assert(e->type() != type_id<PlusExpr>());
                    _exprs.emplace_back(e);
                }
            } else {
                assert(e->type() != type_id<PlusExpr>());
                _exprs.emplace_back(e);
            }
        }

        void CommutativeExpr::init(std::vector<Expr_ptr>&& exprs)
        {
            for (auto& e : exprs) {
                handle(e);
            }
            std::sort(_ids.begin(), _ids.end(), [](auto& a, auto& b){ return a.first < b.first; });
        }

        PlusExpr::PlusExpr(std::vector<Expr_ptr>&& exprs) : CommutativeExpr(0)
        {
            init(std::move(exprs));
        }

        MultiplyExpr::MultiplyExpr(std::vector<Expr_ptr>&& exprs) : CommutativeExpr(1)
        {
            init(std::move(exprs));
        }
    } // PQL
} // PetriEngine

