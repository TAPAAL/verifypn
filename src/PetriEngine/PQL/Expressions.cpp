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

        /******************** Evaluation ********************/

        int NaryExpr::evaluate(const EvaluationContext& context) {
            int32_t r = preOp(context);
            for(size_t i = 1; i < _exprs.size(); ++i)
            {
                r = apply(r, _exprs[i]->evalAndSet(context));
            }
            return r;
        }

        int32_t NaryExpr::preOp(const EvaluationContext& context) const {
            return _exprs[0]->evaluate(context);
        }

        int32_t CommutativeExpr::preOp(const EvaluationContext& context) const {
            int32_t res = _constant;
            for(auto& i : _ids) res = this->apply(res, context.marking()[i.first]);
            if(_exprs.size() > 0) res = this->apply(res, _exprs[0]->evalAndSet(context));
            return res;
        }

        int CommutativeExpr::evaluate(const EvaluationContext& context) {
            if(_exprs.size() == 0) return preOp(context);
            return NaryExpr::evaluate(context);
        }

        int MinusExpr::evaluate(const EvaluationContext& context) {
            return -(_expr->evaluate(context));
        }

        int LiteralExpr::evaluate(const EvaluationContext&) {
            return _value;
        }

        int UnfoldedIdentifierExpr::evaluate(const EvaluationContext& context) {
            assert(_offsetInMarking != -1);
            return context.marking()[_offsetInMarking];
        }

        Condition::Result SimpleQuantifierCondition::evaluate(const EvaluationContext& context) {
            return RUNKNOWN;
        }

        Condition::Result ControlCondition::evaluate(const EvaluationContext& context) {
            return RUNKNOWN;
        }

        Condition::Result ACondition::evaluate(const EvaluationContext& context) {
            //if (_cond->evaluate(context) == RFALSE) return RFALSE;
            return RUNKNOWN;
        }

        Condition::Result ECondition::evaluate(const EvaluationContext& context) {
            //if (_cond->evaluate(context) == RTRUE) return RTRUE;
            return RUNKNOWN;
        }

        Condition::Result FCondition::evaluate(const EvaluationContext& context) {
            //if (_cond->evaluate(context) == RTRUE) return RTRUE;
            return RUNKNOWN;
        }

        Condition::Result GCondition::evaluate(const EvaluationContext& context) {
            //if (_cond->evaluate(context) == RFALSE) return RFALSE;
            return RUNKNOWN;
        }

/*        Condition::Result XCondition::evaluate(const EvaluationContext& context) {
            return _cond->evaluate(context);
        }*/

        Condition::Result UntilCondition::evaluate(const EvaluationContext& context) {
            auto r2 = _cond2->evaluate(context);
            if(r2 != RFALSE) return r2;
            auto r1 = _cond1->evaluate(context);
            if(r1 == RFALSE)
            {
                return RFALSE;
            }
            return RUNKNOWN;
        }



        Condition::Result AndCondition::evaluate(const EvaluationContext& context) {
            auto res = RTRUE;
            for(auto& c : _conds)
            {
                auto r = c->evaluate(context);
                if(r == RFALSE) return RFALSE;
                else if(r == RUNKNOWN) res = RUNKNOWN;
            }
            return res;
        }

        Condition::Result OrCondition::evaluate(const EvaluationContext& context) {
            auto res = RFALSE;
            for(auto& c : _conds)
            {
                auto r = c->evaluate(context);
                if(r == RTRUE) return RTRUE;
                else if(r == RUNKNOWN) res = RUNKNOWN;
            }
            return res;
        }

        Condition::Result CompareConjunction::evaluate(const EvaluationContext& context){
//            auto rres = _org->evaluate(context);
            bool res = true;
            for(auto& c : _constraints)
            {
                res = res && context.marking()[c._place] <= c._upper &&
                             context.marking()[c._place] >= c._lower;
                if(!res) break;
            }
            return (_negated xor res) ? RTRUE : RFALSE;
        }

        Condition::Result CompareCondition::evaluate(const EvaluationContext& context) {
            int v1 = _expr1->evaluate(context);
            int v2 = _expr2->evaluate(context);
            return apply(v1, v2) ? RTRUE : RFALSE;
        }

        Condition::Result NotCondition::evaluate(const EvaluationContext& context) {
            auto res = _cond->evaluate(context);
            if(res != RUNKNOWN) return res == RFALSE ? RTRUE : RFALSE;
            return RUNKNOWN;
        }

        Condition::Result BooleanCondition::evaluate(const EvaluationContext&) {
            return value ? RTRUE : RFALSE;
        }

        Condition::Result DeadlockCondition::evaluate(const EvaluationContext& context) {
            if (!context.net())
                return RFALSE;
            if (!context.net()->deadlocked(context.marking())) {
                return RFALSE;
            }
            return RTRUE;
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

        Condition::Result UnfoldedUpperBoundsCondition::evaluate(const EvaluationContext& context) {
            setUpperBound(value(context.marking()));
            return _max <= _bound ? RTRUE : RUNKNOWN;
        }

        /******************** Evaluation - save result ********************/
        Condition::Result SimpleQuantifierCondition::evalAndSet(const EvaluationContext& context) {
	    return RUNKNOWN;
        }

        Condition::Result GCondition::evalAndSet(const EvaluationContext &context) {
            auto res = _cond->evalAndSet(context);
            if(res != RFALSE) res = RUNKNOWN;
            setSatisfied(res);
            return res;
        }

        Condition::Result FCondition::evalAndSet(const EvaluationContext &context) {
            auto res = _cond->evalAndSet(context);
            if(res != RTRUE) res = RUNKNOWN;
            setSatisfied(res);
            return res;
        }

        Condition::Result UntilCondition::evalAndSet(const EvaluationContext& context) {
            auto r2 = _cond2->evalAndSet(context);
            if(r2 != RFALSE) return r2;
            auto r1 = _cond1->evalAndSet(context);
            if(r1 == RFALSE) return RFALSE;
            return RUNKNOWN;
        }

        int Expr::evalAndSet(const EvaluationContext& context) {
            int r = evaluate(context);
            setEval(r);
            return r;
        }

        Condition::Result AndCondition::evalAndSet(const EvaluationContext& context) {
            Result res = RTRUE;
            for(auto& c : _conds)
            {
                auto r = c->evalAndSet(context);
                if(r == RFALSE)
                {
                    res = RFALSE;
                    break;
                }
                else if(r == RUNKNOWN)
                {
                    res = RUNKNOWN;
                }
            }
            setSatisfied(res);
            return res;
        }

        Condition::Result OrCondition::evalAndSet(const EvaluationContext& context) {
            Result res = RFALSE;
            for(auto& c : _conds)
            {
                auto r = c->evalAndSet(context);
                if(r == RTRUE)
                {
                    res = RTRUE;
                    break;
                }
                else if(r == RUNKNOWN)
                {
                    res = RUNKNOWN;
                }
            }
            setSatisfied(res);
            return res;
        }

        Condition::Result CompareConjunction::evalAndSet(const EvaluationContext& context)
        {
            auto res = evaluate(context);
            setSatisfied(res);
            return res;
        }

        Condition::Result CompareCondition::evalAndSet(const EvaluationContext& context) {
            int v1 = _expr1->evalAndSet(context);
            int v2 = _expr2->evalAndSet(context);
            bool res = apply(v1, v2);
            setSatisfied(res);
            return res ? RTRUE : RFALSE;
        }

        Condition::Result NotCondition::evalAndSet(const EvaluationContext& context) {
            auto res = _cond->evalAndSet(context);
            if(res != RUNKNOWN) res = res == RFALSE ? RTRUE : RFALSE;
            setSatisfied(res);
            return res;
        }

        Condition::Result BooleanCondition::evalAndSet(const EvaluationContext&) {
            setSatisfied(value);
            return value ? RTRUE : RFALSE;
        }

        Condition::Result DeadlockCondition::evalAndSet(const EvaluationContext& context) {
            if (!context.net())
                return RFALSE;
            setSatisfied(context.net()->deadlocked(context.marking()));
            return isSatisfied() ? RTRUE : RFALSE;
        }

        Condition::Result UnfoldedUpperBoundsCondition::evalAndSet(const EvaluationContext& context)
        {
            auto res = evaluate(context);
            setSatisfied(res);
            return res;
        }

        /******************** Range Contexts ********************/

        void SimpleQuantifierCondition::visit(Visitor& ctx) const {
            ctx.accept<decltype(this)>(this);
        }

        void ControlCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void UntilCondition::visit(Visitor &ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void ACondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void ECondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void GCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void FCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void XCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void LogicalCondition::visit(Visitor& ctx) const {
            ctx.accept<decltype(this)>(this);
        }

        void AndCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void OrCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void NotCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void CompareCondition::visit(Visitor& ctx) const {
            ctx.accept<decltype(this)>(this);
        }

        void EqualCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void NotEqualCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void CompareConjunction::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void LessThanOrEqualCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void LessThanCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void BooleanCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void DeadlockCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void StableMarkingCondition::visit(Visitor& ctx) const
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void QuasiLivenessCondition::visit(Visitor& ctx) const
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void KSafeCondition::visit(Visitor& ctx) const
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void LivenessCondition::visit(Visitor& ctx) const
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void FireableCondition::visit(Visitor& ctx) const
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void UpperBoundsCondition::visit(Visitor& ctx) const
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void UnfoldedFireableCondition::visit(Visitor& ctx) const
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void ShallowCondition::visit(Visitor &ctx) const {
            if (_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }


        void UnfoldedUpperBoundsCondition::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void LiteralExpr::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void IdentifierExpr::visit(Visitor& ctx) const
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void UnfoldedIdentifierExpr::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void CommutativeExpr::visit(Visitor& ctx) const {
            ctx.accept<decltype(this)>(this);
        }

        void MinusExpr::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void SubtractExpr::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void NaryExpr::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void PlusExpr::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        void MultiplyExpr::visit(Visitor& ctx) const
        {
            ctx.accept<decltype(this)>(this);
        }

        /******************** Mutating visitor **********************************/

        void ControlCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void UntilCondition::visit(MutatingVisitor &ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void SimpleQuantifierCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void ACondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void ECondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void GCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void FCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void XCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void LogicalCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void AndCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void OrCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void NotCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void CompareCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void EqualCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void NotEqualCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void CompareConjunction::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void LessThanOrEqualCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void LessThanCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void BooleanCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void DeadlockCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void StableMarkingCondition::visit(MutatingVisitor& ctx)
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void QuasiLivenessCondition::visit(MutatingVisitor& ctx)
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void KSafeCondition::visit(MutatingVisitor& ctx)
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void LivenessCondition::visit(MutatingVisitor& ctx)
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void FireableCondition::visit(MutatingVisitor& ctx)
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void UpperBoundsCondition::visit(MutatingVisitor& ctx)
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void UnfoldedFireableCondition::visit(MutatingVisitor& ctx)
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void ShallowCondition::visit(MutatingVisitor& ctx)
        {
            if(_compiled)
                _compiled->visit(ctx);
            else
                ctx.accept<decltype(this)>(this);
        }

        void UnfoldedUpperBoundsCondition::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void NaryExpr::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void CommutativeExpr::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void PlusExpr::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void SubtractExpr::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void MultiplyExpr::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void MinusExpr::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void LiteralExpr::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void IdentifierExpr::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        void UnfoldedIdentifierExpr::visit(MutatingVisitor& ctx)
        {
            ctx.accept<decltype(this)>(this);
        }

        /******************** Apply (BinaryExpr subclasses) ********************/

        int PlusExpr::apply(int v1, int v2) const {
            return v1 + v2;
        }

        int SubtractExpr::apply(int v1, int v2) const {
            return v1 - v2;
        }

        int MultiplyExpr::apply(int v1, int v2) const {
            return v1 * v2;
        }

        /******************** Apply (CompareCondition subclasses) ********************/

        bool EqualCondition::apply(int v1, int v2) const {
            return v1 == v2;
        }

        bool NotEqualCondition::apply(int v1, int v2) const {
            return v1 != v2;
        }

        bool LessThanCondition::apply(int v1, int v2) const {
            return v1 < v2;
        }

        bool LessThanOrEqualCondition::apply(int v1, int v2) const {
            return v1 <= v2;
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

        /******************** Expr::type() implementation ********************/

        Expr::Types PlusExpr::type() const {
            return Expr::PlusExpr;
        }

        Expr::Types SubtractExpr::type() const {
            return Expr::SubtractExpr;
        }

        Expr::Types MultiplyExpr::type() const {
            return Expr::MinusExpr;
        }

        Expr::Types MinusExpr::type() const {
            return Expr::MinusExpr;
        }

        Expr::Types LiteralExpr::type() const {
            return Expr::LiteralExpr;
        }

        Expr::Types UnfoldedIdentifierExpr::type() const {
            return Expr::IdentifierExpr;
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
                    val = (*cmp)[0]->evaluate(context);
                    inverted = true;
                }
                else
                {
                    val = (*cmp)[1]->evaluate(context);
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
                    assert(id->name().compare(lb->_name) == 0);
                    lb->intersect(next);
                }
            }
        }

        void CommutativeExpr::init(std::vector<Expr_ptr>&& exprs)
        {
            for (auto& e : exprs) {
                if (e->placeFree())
                {
                    EvaluationContext c;
                    _constant = apply(_constant, e->evaluate(c));
                }
                else if (auto id = std::dynamic_pointer_cast<PQL::UnfoldedIdentifierExpr>(e)) {
                    _ids.emplace_back(id->offset(), id->name());
                }
                else if(auto c = std::dynamic_pointer_cast<CommutativeExpr>(e))
                {
                    // we should move up plus/multiply here when possible;
                    if(c->_ids.size() == 0 && c->_exprs.size() == 0)
                    {
                        _constant = apply(_constant, c->_constant);
                    }
                    else
                    {
                        _exprs.emplace_back(std::move(e));
                    }
                } else {
                    _exprs.emplace_back(std::move(e));
                }
            }
        }

        PlusExpr::PlusExpr(std::vector<Expr_ptr>&& exprs, bool tk) : CommutativeExpr(0), tk(tk)
        {
            init(std::move(exprs));
        }

        MultiplyExpr::MultiplyExpr(std::vector<Expr_ptr>&& exprs) : CommutativeExpr(1)
        {
            init(std::move(exprs));
        }
    } // PQL
} // PetriEngine

