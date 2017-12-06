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
#include "Contexts.h"
#include "Expressions.h"

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
        
        std::ostream& generateTabs(std::ostream& out, uint32_t tabs) {

            for(uint32_t i = 0; i < tabs; i++) {
                out << "  ";
            }
            return out;
        }
        
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
                    if(! ((dynamic_cast<IdentifierExpr*>((*comp)[0].get()) && dynamic_cast<LiteralExpr*>((*comp)[1].get())) ||
                          (dynamic_cast<IdentifierExpr*>((*comp)[1].get()) && dynamic_cast<LiteralExpr*>((*comp)[0].get()))))
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
                                _conds.insert(_conds.begin(), std::make_shared<CompareConjunction>(*conj, true));
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
            if(conds.size() == 0) return BooleanCondition::getShared(K);
            if(conds.size() == 1) return conds[0];
            std::vector<Condition_ptr> cnds;
            for(auto& c : conds) tryMerge<T>(cnds, c, aggressive);
            auto res = std::make_shared<T>(cnds);
            if(res->singular()) return *res->begin();
            if(res->empty()) return BooleanCondition::getShared(K);
            return res;
        }
        
        Condition_ptr makeOr(const std::vector<Condition_ptr>& cptr) 
        { return makeLog<OrCondition,false>(cptr, true); }
        Condition_ptr makeAnd(const std::vector<Condition_ptr>& cptr) 
        { return makeLog<AndCondition,true>(cptr, true); }
        Condition_ptr makeOr(const Condition_ptr& a, const Condition_ptr& b) {  
            std::vector<Condition_ptr> cnds{a,b};
            return makeLog<OrCondition,false>(cnds, true); 
        }
        Condition_ptr makeAnd(const Condition_ptr& a, const Condition_ptr& b) 
        {             
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
        
        /******************** To String ********************/

        void LiteralExpr::toString(std::ostream& out) const {
            out << _value;
        }

        void IdentifierExpr::toString(std::ostream& out) const {
            out << _name;
        }

        void NaryExpr::toString(std::ostream& ss) const {
            ss << "(";
            _exprs[0]->toString(ss);
            for(size_t i = 1; i < _exprs.size(); ++i)
            {
                ss << " " << op() << " ";
                _exprs[i]->toString(ss);
            }
            ss << ")";
        }

        void CommutativeExpr::toString(std::ostream& ss) const {
            ss << "( " << _constant;
            for(auto& i : _ids)
                ss << " " << op() << " " << i.second;
            for(auto& e : _exprs)
            {
                ss << " " << op() << " ";
                e->toString(ss);
            }
        }


        void MinusExpr::toString(std::ostream& out) const {
            out << "-";
            _expr->toString(out);
        }

        void SimpleQuantifierCondition::toString(std::ostream& out) const {
            out << op() << " ";
            _cond->toString(out);
        }
        
        void UntilCondition::toString(std::ostream& out) const {
            out << op() << " (";
            _cond1->toString(out);
            out << " U ";
            _cond2->toString(out);
            out << ")";
        }
        
        void LogicalCondition::toString(std::ostream& out) const {
            out << "(";
            _conds[0]->toString(out);
            for(size_t i = 1; i < _conds.size(); ++i)
            {
                out << " " << op() << " ";
                _conds[i]->toString(out);
            }
            out << ")";
        }
        
        void CompareConjunction::toString(std::ostream& out) const {
            if(_negated) out << "(not";
            bool first = true;
            for(auto& c : _constraints)
            {
                if(!first) out << " and ";
                if(c._lower != 0) 
                    out << "(" << c._lower << " <= " << c._name << ")";
                if(c._lower != 0 && c._upper != std::numeric_limits<uint32_t>::max()) 
                    out << " and ";
                if(c._upper != std::numeric_limits<uint32_t>::max()) 
                    out << "(" << c._upper << " >= " << c._name << ")";
                first = false;
            }
            if(_negated) out << ")";
        }

        void CompareCondition::toString(std::ostream& out) const {
            out << "(";
            _expr1->toString(out);
            out << " " << op() << " ";
            _expr2->toString(out);
            out <<")";
        }

        void NotCondition::toString(std::ostream& out) const {
            out << "(not ";
            _cond->toString(out);
            out << ")";
        }

        void BooleanCondition::toString(std::ostream& out) const {
            if (_value)
                out << "true";
            else
                out << "false";
        }

        void DeadlockCondition::toString(std::ostream& out) const {
            out << "deadlock";
        }

        /******************** To TAPAAL Query ********************/

        void SimpleQuantifierCondition::toTAPAALQuery(std::ostream& out,TAPAALConditionExportContext& context) const {
            out << op() << " ";
            _cond->toTAPAALQuery(out,context);
        }
        
        void UntilCondition::toTAPAALQuery(std::ostream& out,TAPAALConditionExportContext& context) const {
            out << op() << " (";
            _cond1->toTAPAALQuery(out, context);
            out << " U ";
            _cond2->toTAPAALQuery(out,context);
            out << ")";
        }
        
        void LogicalCondition::toTAPAALQuery(std::ostream& out,TAPAALConditionExportContext& context) const {
            out << "(";
            _conds[0]->toTAPAALQuery(out, context);
            for(size_t i = 1; i < _conds.size(); ++i)
            {
                out << " " << op() << " ";
                _conds[i]->toTAPAALQuery(out, context);
            }
            out << ")";
        }
        
        void CompareConjunction::toTAPAALQuery(std::ostream& out,TAPAALConditionExportContext& context) const {
            if(_negated) out << "(!";
            bool first = true;
            for(auto& c : _constraints)
            {
                if(!first) out << " and ";
                if(c._lower != 0) 
                    out << "(" << c._lower << " <= " << context.netName << "." << c._name << ")";
                if(c._lower != 0 && c._upper != std::numeric_limits<uint32_t>::max()) 
                    out << " and ";
                if(c._lower != 0) 
                    out << "(" << c._upper << " >= " << context.netName << "." << c._name << ")";
                first = false;
            }
            if(_negated) out << ")";
        }

        void CompareCondition::toTAPAALQuery(std::ostream& out,TAPAALConditionExportContext& context) const {
            //If <id> <op> <literal>
            if (_expr1->type() == Expr::IdentifierExpr && _expr2->type() == Expr::LiteralExpr) {
                out << " ( " << context.netName << ".";
                _expr1->toString(out);
                out << " " << opTAPAAL() << " ";
                _expr2->toString(out);
                out << " ) ";
                //If <literal> <op> <id>
            } else if (_expr2->type() == Expr::IdentifierExpr && _expr1->type() == Expr::LiteralExpr) {
                out << " ( ";
                _expr1->toString(out);
                out << " " << sopTAPAAL() << " " << context.netName << ".";
                _expr2->toString(out);
                out << " ) ";
            } else {
                context.failed = true;
                out << " false ";
            }
        }

        void NotEqualCondition::toTAPAALQuery(std::ostream& out,TAPAALConditionExportContext& context) const {
            out << " !( ";
            CompareCondition::toTAPAALQuery(out,context);
            out << " ) ";
        }

        void NotCondition::toTAPAALQuery(std::ostream& out,TAPAALConditionExportContext& context) const {
            out << " !( ";
            _cond->toTAPAALQuery(out,context);
            out << " ) ";
        }

        void BooleanCondition::toTAPAALQuery(std::ostream& out,TAPAALConditionExportContext&) const {
            if (_value)
                out << "true";
            else
                out << "false";
        }

        void DeadlockCondition::toTAPAALQuery(std::ostream& out,TAPAALConditionExportContext&) const {
            out << "deadlock";
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

        std::string GreaterThanCondition::opTAPAAL() const {
            return ">";
        }

        std::string GreaterThanOrEqualCondition::opTAPAAL() const {
            return ">=";
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

        std::string GreaterThanCondition::sopTAPAAL() const {
            return "<=";
        }

        std::string GreaterThanOrEqualCondition::sopTAPAAL() const {
            return "<";
        }

        /******************** Context Analysis ********************/

        auto expr_cmp = [](const Expr_ptr& a, const Expr_ptr& b){
                if(dynamic_cast<LiteralExpr*>(a.get()))
                {
                    if(dynamic_cast<LiteralExpr*>(b.get())) return false;
                    else return true;
                }
                else if(auto aa = dynamic_cast<IdentifierExpr*>(a.get()))
                {
                    if(dynamic_cast<LiteralExpr*>(b.get())) return false; 
                    else if(auto bb = dynamic_cast<IdentifierExpr*>(b.get()))
                    {
                        return aa->offset() < bb->offset();
                    }
                    else return true;
                }
                return false;
            };
        
        void NaryExpr::analyze(AnalysisContext& context) {
            for(auto& e : _exprs) e->analyze(context);
            std::sort(_exprs.begin(), _exprs.end(), [](auto& a, auto& b)
            {
                auto ida = dynamic_pointer_cast<PQL::IdentifierExpr>(a);
                auto idb = dynamic_pointer_cast<PQL::IdentifierExpr>(b);
                if(ida == NULL) return false;
                if(ida && !idb) return true;
                return ida->offset() < idb->offset();
            });
        }

        void CommutativeExpr::analyze(AnalysisContext& context) {
            for(auto& i : _ids)
            {
                AnalysisContext::ResolutionResult result = context.resolve(i.second);
                if (result.success) {
                    i.first = result.offset;
                } else {
                    ExprError error("Unable to resolve identifier \"" + i.second + "\"",
                            i.second.length());
                    context.reportError(error);
                }
            }
            std::sort(_ids.begin(), _ids.end(), [](auto& a, auto& b){ return a.first < b.first; });
            if(_exprs.size() > 0)
                NaryExpr::analyze(context);
        }

        void MinusExpr::analyze(AnalysisContext& context) {
            _expr->analyze(context);
        }

        void LiteralExpr::analyze(AnalysisContext&) {
            return;
        }

        void IdentifierExpr::analyze(AnalysisContext& context) {
            AnalysisContext::ResolutionResult result = context.resolve(_name);
            if (result.success) {
                _offsetInMarking = result.offset;
            } else {
                ExprError error("Unable to resolve identifier \"" + _name + "\"",
                        _name.length());
                context.reportError(error);
            }
        }

        void SimpleQuantifierCondition::analyze(AnalysisContext& context) {
            _cond->analyze(context);
        }
        
        void UntilCondition::analyze(AnalysisContext& context) {
            _cond1->analyze(context);
            _cond2->analyze(context);
        }
        
        void LogicalCondition::analyze(AnalysisContext& context) {
            for(auto& c : _conds) c->analyze(context);
        }
        
        void CompareConjunction::analyze(AnalysisContext& context) {
            for(auto& c : _constraints){
                AnalysisContext::ResolutionResult result = context.resolve(c._name);
                if (result.success) {
                    c._place = result.offset;
                } else {
                    ExprError error("Unable to resolve identifier \"" + c._name + "\"",
                            c._name.length());
                    context.reportError(error);
                }
            }
            std::sort(std::begin(_constraints), std::end(_constraints));
        }

        void CompareCondition::analyze(AnalysisContext& context) {
            _expr1->analyze(context);
            _expr2->analyze(context);
            auto remdup = [](auto& a, auto& b){
                auto ai = a->_ids.begin();
                auto bi = b->_ids.begin();
                while(ai != a->_ids.end() && bi != b->_ids.end())
                {
                    while(ai != a->_ids.end() && ai->first < bi->first) ++ai;
                    if(ai == a->_ids.end()) break;
                    if(ai->first == bi->first)
                    {
                        ai = a->_ids.erase(ai);
                        bi = b->_ids.erase(bi);
                    }
                    else
                    {
                        ++bi;
                    }
                    if(bi == b->_ids.end() || ai == a->_ids.end()) break;
                }
            };
            if(auto p1 = dynamic_pointer_cast<PlusExpr>(_expr1))
                if(auto p2 = dynamic_pointer_cast<PlusExpr>(_expr2))
                    remdup(p1, p2);
            
            if(auto m1 = dynamic_pointer_cast<PlusExpr>(_expr1))
                if(auto m2 = dynamic_pointer_cast<PlusExpr>(_expr2))
                    remdup(m1, m2);                    
        }
        
        void NotCondition::analyze(AnalysisContext& context) {
            _cond->analyze(context);
        }

        void BooleanCondition::analyze(AnalysisContext&) {
        }

        void DeadlockCondition::analyze(AnalysisContext& c) {
            c.setHasDeadlock();
        }

        /******************** Evaluation ********************/

        int NaryExpr::evaluate(const EvaluationContext& context) const {
            int32_t r = preOp(context);
            for(size_t i = 1; i < _exprs.size(); ++i)
            {
                r = apply(r, _exprs[i]->evaluate(context));
            }
            return r;
        }
        
        int32_t NaryExpr::preOp(const EvaluationContext& context) const {
            return _exprs[0]->evaluate(context);
        }

        int32_t CommutativeExpr::preOp(const EvaluationContext& context) const {
            int32_t res = _constant;
            for(auto& i : _ids) res = this->apply(res, context.marking()[i.first]);
            return res;
        }

        int CommutativeExpr::evaluate(const EvaluationContext& context) const {
            if(_exprs.size() == 0) return preOp(context);
            return NaryExpr::evaluate(context);
        }
        
        int MinusExpr::evaluate(const EvaluationContext& context) const {
            return -(_expr->evaluate(context));
        }

        int LiteralExpr::evaluate(const EvaluationContext&) const {
            return _value;
        }

        int IdentifierExpr::evaluate(const EvaluationContext& context) const {
            assert(_offsetInMarking != -1);
            return context.marking()[_offsetInMarking];
        }

        Condition::Result SimpleQuantifierCondition::evaluate(const EvaluationContext& context) const {
	    return RUNKNOWN;
        }

        Condition::Result EGCondition::evaluate(const EvaluationContext& context) const {
            if(_cond->evaluate(context) == RFALSE) return RFALSE;
            return RUNKNOWN;
        }

        Condition::Result AGCondition::evaluate(const EvaluationContext& context) const 
        {
            if(_cond->evaluate(context) == RFALSE) return RFALSE;
            return RUNKNOWN;
        }

        Condition::Result EFCondition::evaluate(const EvaluationContext& context) const {
            if(_cond->evaluate(context) == RTRUE) return RTRUE;
            return RUNKNOWN;
        }

        Condition::Result AFCondition::evaluate(const EvaluationContext& context) const {
            if(_cond->evaluate(context) == RTRUE) return RTRUE;
            return RUNKNOWN;
        }

        
        Condition::Result UntilCondition::evaluate(const EvaluationContext& context) const {
            auto r2 = _cond2->evaluate(context);
            if(r2 != RFALSE) return r2;
            auto r1 = _cond1->evaluate(context);
            if(r1 == RFALSE)
            {
                return RFALSE;
            }
            return RUNKNOWN;
        }
        

        
        Condition::Result AndCondition::evaluate(const EvaluationContext& context) const {
            auto res = RTRUE;            
            for(auto& c : _conds)
            {
                auto r = c->evaluate(context);
                if(r == RFALSE) return RFALSE;
                else if(r == RUNKNOWN) res = RUNKNOWN;
            }
            return res;
        }

        Condition::Result OrCondition::evaluate(const EvaluationContext& context) const {
            auto res = RFALSE;            
            for(auto& c : _conds)
            {
                auto r = c->evaluate(context);
                if(r == RTRUE) return RTRUE;
                else if(r == RUNKNOWN) res = RUNKNOWN;
            }
            return res;
        }
        
        Condition::Result CompareConjunction::evaluate(const EvaluationContext& context) const{
//            auto rres = _org->evaluate(context);
            bool res = true;
            for(auto& c : _constraints)
            {
                res = res && context.marking()[c._place] <= c._upper &&
                             context.marking()[c._place] >= c._lower;
                if(!res) break;
            }
            
            return _negated xor res ? RTRUE : RFALSE;
        }
        
        Condition::Result CompareCondition::evaluate(const EvaluationContext& context) const {
            int v1 = _expr1->evaluate(context);
            int v2 = _expr2->evaluate(context);
            return apply(v1, v2) ? RTRUE : RFALSE;
        }

        Condition::Result NotCondition::evaluate(const EvaluationContext& context) const {
            auto res = _cond->evaluate(context);
            if(res != RUNKNOWN) return res == RFALSE ? RTRUE : RFALSE;
            return RUNKNOWN;
        }

        Condition::Result BooleanCondition::evaluate(const EvaluationContext&) const {
            return _value ? RTRUE : RFALSE;
        }

        Condition::Result DeadlockCondition::evaluate(const EvaluationContext& context) const {
            if (!context.net())
                return RFALSE;
            if (!context.net()->deadlocked(context.marking())) {
                return RFALSE;
            }
            return RTRUE;
        }
        
        /******************** Evaluation - save result ********************/
        Condition::Result SimpleQuantifierCondition::evalAndSet(const EvaluationContext& context) {
	    return RUNKNOWN;
        }

        Condition::Result EGCondition::evalAndSet(const EvaluationContext& context) {
            auto res = _cond->evalAndSet(context);
            if(res != RFALSE) res = RUNKNOWN;
            setSatisfied(res);
            return res;
        }

        Condition::Result AGCondition::evalAndSet(const EvaluationContext& context) {
            auto res = _cond->evalAndSet(context);
            if(res != RFALSE) res = RUNKNOWN;
            setSatisfied(res);
            return res;
        }

        Condition::Result EFCondition::evalAndSet(const EvaluationContext& context) {
            auto res = _cond->evalAndSet(context);
            if(res != RTRUE) res = RUNKNOWN;
            setSatisfied(res);
            return res;
        }

        Condition::Result AFCondition::evalAndSet(const EvaluationContext& context) {
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
            setSatisfied(_value);
            return _value ? RTRUE : RFALSE;
        }

        Condition::Result DeadlockCondition::evalAndSet(const EvaluationContext& context) {
            if (!context.net())
                return RFALSE;
            setSatisfied(context.net()->deadlocked(context.marking()));
            return isSatisfied() ? RTRUE : RFALSE;
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

        bool GreaterThanCondition::apply(int v1, int v2) const {
            return v1 > v2;
        }

        bool GreaterThanOrEqualCondition::apply(int v1, int v2) const {
            return v1 >= v2;
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
        
        /******************** Op (QuantifierCondition subclasses) ********************/
        
        std::string EXCondition::op() const {
            return "EX";
        }
        
        std::string EGCondition::op() const {
            return "EG";
        }
        
        std::string EFCondition::op() const {
            return "EF";
        }
        
        std::string AXCondition::op() const {
            return "AX";
        }
        
        std::string AGCondition::op() const {
            return "AG";
        }
        
        std::string AFCondition::op() const {
            return "AF";
        }
        
        /******************** Op (UntilCondition subclasses) ********************/

        std::string EUCondition::op() const {
            return "E";
        }
        
        std::string AUCondition::op() const {
            return "A";
        }
        
        /******************** Op (LogicalCondition subclasses) ********************/

        std::string AndCondition::op() const {
            return "and";
        }

        std::string OrCondition::op() const {
            return "or";
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

        std::string GreaterThanCondition::op() const {
            return ">";
        }

        std::string GreaterThanOrEqualCondition::op() const {
            return ">=";
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

        Expr::Types IdentifierExpr::type() const {
            return Expr::IdentifierExpr;
        }

        /******************** Distance Condition ********************/

#define MAX(v1, v2)  (v1 > v2 ? v1 : v2)
#define MIN(v1, v2)  (v1 < v2 ? v1 : v2)

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

        template<>
        uint32_t delta<GreaterThanCondition>(int v1, int v2, bool negated) {
            return delta<LessThanOrEqualCondition>(v1, v2, !negated);
        }

        template<>
        uint32_t delta<GreaterThanOrEqualCondition>(int v1, int v2, bool negated) {
            return delta<LessThanCondition>(v1, v2, !negated);
        }

        uint32_t NotCondition::distance(DistanceContext& context) const {
            context.negate();
            uint32_t retval = _cond->distance(context);
            context.negate();
            return retval;
        }

        uint32_t BooleanCondition::distance(DistanceContext& context) const {
            if (context.negated() != _value)
                return 0;
            return std::numeric_limits<uint32_t>::max();
        }

        uint32_t DeadlockCondition::distance(DistanceContext& context) const {
            return 0;
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
        
        uint32_t LogicalCondition::distance(DistanceContext& context) const {
            uint32_t d = _conds[0]->distance(context);
            for(size_t i = 1; i < _conds.size(); ++i) d = delta(d, _conds[i]->distance(context), context);
            return d;
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
                         (c._lower == 0 ? 0 : delta<GreaterThanOrEqualCondition>(pv, c._lower, neg));
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
                        else      d = MIN(d, d2);
                        first = false;
                    }
                    
                    if(c._lower != 0)
                    {
                        auto d2 = delta<GreaterThanOrEqualCondition>(pv, c._upper, neg);
                        if(first) d = d2;
                        else      d = MIN(d, d2);
                        first = false;
                    }
                }
            }
            return d;
        }

        uint32_t AndCondition::delta(uint32_t d1,
                uint32_t d2,
                const DistanceContext& context) const {
                return d1 + d2;
        }

        uint32_t OrCondition::delta(uint32_t d1,
                uint32_t d2,
                const DistanceContext& context) const {
            if (context.negated())
                return MAX(d1, d2);
            else
                return MIN(d1, d2);
        }

        struct S {
            int d;
            unsigned int p;
        };
       
        uint32_t LessThanOrEqualCondition::distance(DistanceContext& context) const {
            return _distance(context, delta<LessThanOrEqualCondition>);
        }
        
        uint32_t GreaterThanOrEqualCondition::distance(DistanceContext& context) const {
            return _distance(context, delta<GreaterThanOrEqualCondition>);
        }
       
        uint32_t LessThanCondition::distance(DistanceContext& context) const {
            return _distance(context, delta<LessThanCondition>);
        }
       
        uint32_t GreaterThanCondition::distance(DistanceContext& context) const {
            return _distance(context, delta<GreaterThanCondition>);
        }
       
        uint32_t NotEqualCondition::distance(DistanceContext& context) const {
            return _distance(context, delta<NotEqualCondition>);
        }
       
        uint32_t EqualCondition::distance(DistanceContext& context) const {
            return _distance(context, delta<EqualCondition>);
        }

        /******************** CTL Output ********************/ 
        
        void LiteralExpr::toXML(std::ostream& out,uint32_t tabs, bool tokencount) const {
            generateTabs(out,tabs) << "<integer-constant>" + std::to_string(_value) + "</integer-constant>\n";
        }
        
        void IdentifierExpr::toXML(std::ostream& out,uint32_t tabs, bool tokencount) const {
            if (tokencount) {
                generateTabs(out,tabs) << "<place>" << _name << "</place>\n";
            }
            else
            {
                generateTabs(out,tabs) << "<tokens-count>\n"; 
                generateTabs(out,tabs+1) << "<place>" << _name << "</place>\n";
                generateTabs(out,tabs) << "</tokens-count>\n";
            }
        }
        
        void PlusExpr::toXML(std::ostream& ss,uint32_t tabs, bool tokencount) const {
            if (tokencount) {
                for(auto& e : _exprs) e->toXML(ss,tabs, tokencount);
                return;
            }
            
            if(tk) {
                generateTabs(ss,tabs) << "<tokens-count>\n";
                for(auto& e : _ids) generateTabs(ss,tabs+1) << "<place>" << e.second << "</place>\n";
                for(auto& e : _exprs) e->toXML(ss,tabs+1, true);
                generateTabs(ss,tabs) << "</tokens-count>\n";
                return;
            }
            generateTabs(ss,tabs) << "<integer-sum>\n";
            generateTabs(ss,tabs+1) << "<integer-constant>" + std::to_string(_constant) + "</integer-constant>\n";
            for(auto& i : _ids)
            {
                generateTabs(ss,tabs+1) << "<tokens-count>\n"; 
                generateTabs(ss,tabs+2) << "<place>" << i.second << "</place>\n";
                generateTabs(ss,tabs+1) << "</tokens-count>\n";                
            }
            for(auto& e : _exprs) e->toXML(ss,tabs+1, tokencount);
            generateTabs(ss,tabs) << "</integer-sum>\n";
        }
        
        void SubtractExpr::toXML(std::ostream& ss,uint32_t tabs, bool tokencount) const {
            generateTabs(ss,tabs) << "<integer-difference>\n";
            for(auto& e : _exprs) e->toXML(ss,tabs+1);
            generateTabs(ss,tabs) << "</integer-difference>\n";
        }
        
        void MultiplyExpr::toXML(std::ostream& ss,uint32_t tabs, bool tokencount) const {
            generateTabs(ss,tabs) << "<integer-product>\n";
            for(auto& e : _exprs) e->toXML(ss,tabs+1);
            generateTabs(ss,tabs) << "</integer-product>\n";
        }
        
        void MinusExpr::toXML(std::ostream& out,uint32_t tabs, bool tokencount) const {
            
            generateTabs(out,tabs) << "<integer-product>\n";
            _expr->toXML(out,tabs+1);
            generateTabs(out,tabs+1) << "<integer-difference>\n" ; generateTabs(out,tabs+2) <<
                    "<integer-constant>0</integer-constant>\n" ; generateTabs(out,tabs+2) << 
                    "<integer-constant>1</integer-constant>\n" ; generateTabs(out,tabs+1) <<
                    "</integer-difference>\n" ; generateTabs(out,tabs) << "</integer-product>\n";
        }
        
        void EXCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<exists-path>\n" ; generateTabs(out,tabs+1) << "<next>\n";
            _cond->toXML(out,tabs+2);
            generateTabs(out,tabs+1) << "</next>\n" ; generateTabs(out,tabs) << "</exists-path>\n";
        }

        void AXCondition::toXML(std::ostream& out,uint32_t tabs) const {           
            generateTabs(out,tabs) << "<all-paths>\n"; generateTabs(out,tabs+1) << "<next>\n";
            _cond->toXML(out,tabs+2);            
            generateTabs(out,tabs+1) << "</next>\n"; generateTabs(out,tabs) << "</all-paths>\n";
        }
        
        void EFCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<exist-path>\n" ; generateTabs(out,tabs+1) << "<finally>\n";
            _cond->toXML(out,tabs+2);
            generateTabs(out,tabs+1) << "</finally>\n" ; generateTabs(out,tabs) << "</exist-path>\n";
        }
        
        void AFCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<all-paths>\n" ; generateTabs(out,tabs+1) << "<finally>\n";
            _cond->toXML(out,tabs+2);
            generateTabs(out,tabs+1) << "</finally>\n" ; generateTabs(out,tabs) << "</all-paths>\n";
        }
        
        void EGCondition::toXML(std::ostream& out,uint32_t tabs) const {            
            generateTabs(out,tabs) << "<exist-path>\n" ; generateTabs(out,tabs+1) << "<globally>\n";
            _cond->toXML(out,tabs+2);            
            generateTabs(out,tabs+1) <<  "</globally>\n" ; generateTabs(out,tabs) << "</exist-path>\n";
        }
        
        void AGCondition::toXML(std::ostream& out,uint32_t tabs) const {            
            generateTabs(out,tabs) << "<all-paths>\n" ; generateTabs(out,tabs+1) << "<globally>\n";
            _cond->toXML(out,tabs+2);
            generateTabs(out,tabs+1) << "</globally>\n" ; generateTabs(out,tabs) << "</all-paths>\n";
        }
        
        void EUCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<exist-path>\n" ; generateTabs(out,tabs+1) << "<until>\n" ; generateTabs(out,tabs+2) << "<before>\n";
            _cond1->toXML(out,tabs+3);
            generateTabs(out,tabs+2) << "</before>\n" ; generateTabs(out,tabs+2) << "<reach>\n";
            _cond2->toXML(out,tabs+3);
            generateTabs(out,tabs+2) << "</reach>\n" ; generateTabs(out,tabs+1) << "</until>\n" ; generateTabs(out,tabs) << "</exist-path>\n";
        }
        
        void AUCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<all-paths>\n" ; generateTabs(out,tabs+1) << "<until>\n" ; generateTabs(out,tabs+2) << "<before>\n";
            _cond1->toXML(out,tabs+3);
            generateTabs(out,tabs+2) << "</before>\n" ; generateTabs(out,tabs+2) << "<reach>\n";
            _cond2->toXML(out,tabs+3);
            generateTabs(out,tabs+2) << "</reach>\n" ; generateTabs(out,tabs+1) << "</until>\n" ; generateTabs(out,tabs) << "</all-paths>\n";
        }
        
        void AndCondition::toXML(std::ostream& out,uint32_t tabs) const {
            if(_conds.size() == 0)
            {
                BooleanCondition::TRUE_CONSTANT->toXML(out, tabs);
                return;
            }
            if(_conds.size() == 1)
            {
                _conds[0]->toXML(out, tabs);
                return;
            }
            generateTabs(out,tabs) << "<conjunction>\n";
            _conds[0]->toXML(out, tabs + 1);
            for(size_t i = 1; i < _conds.size(); ++i)
            {
                if(i + 1 == _conds.size())
                {
                    _conds[i]->toXML(out, tabs + i + 1);
                }
                else
                {
                    generateTabs(out,tabs + i) << "<conjunction>\n";
                    _conds[i]->toXML(out, tabs + i + 1);
                }
            }
            for(size_t i = _conds.size() - 1; i > 1; --i)
            {
                generateTabs(out,tabs + i) << "</conjunction>\n";                
            }
            generateTabs(out,tabs) << "</conjunction>\n";              
        }
        
        void OrCondition::toXML(std::ostream& out,uint32_t tabs) const {
            if(_conds.size() == 0)
            {
                BooleanCondition::FALSE_CONSTANT->toXML(out, tabs);
                return;
            }
            if(_conds.size() == 1)
            {
                _conds[0]->toXML(out, tabs);
                return;
            }
            generateTabs(out,tabs) << "<disjunction>\n";
            _conds[0]->toXML(out, tabs + 1);
            for(size_t i = 1; i < _conds.size(); ++i)
            {
                if(i + 1 == _conds.size())
                {
                    _conds[i]->toXML(out, tabs + i + 1);
                }
                else
                {
                    generateTabs(out,tabs + i) << "<disjunction>\n";
                    _conds[i]->toXML(out, tabs + i + 1);
                }
            }
            for(size_t i = _conds.size() - 1; i > 1; --i)
            {
                generateTabs(out,tabs + i) << "</disjunction>\n";                
            }
            generateTabs(out,tabs) << "</disjunction>\n";               
        }

        void CompareConjunction::toXML(std::ostream& out, uint32_t tabs) const
        {
            if(_negated) generateTabs(out,tabs++) << "<negation>";
            if(_constraints.size() == 0) BooleanCondition::TRUE_CONSTANT->toXML(out, tabs);
            else
            {
                generateTabs(out,tabs) << "<conjunction>\n";
                for(auto& c : _constraints)
                {
                    generateTabs(out,tabs+1) << "<integer-le>\n";
                    generateTabs(out,tabs+2) << "<tokens-count>\n";
                    generateTabs(out,tabs+2) << c._place << "\n";
                    generateTabs(out,tabs+2) << "<tokens-count>\n";
                    generateTabs(out,tabs+1) << "</integer-le>\n";  
                    generateTabs(out,tabs+1) << "<integer-gt>\n";
                    generateTabs(out,tabs+2) << "<tokens-count>\n";
                    generateTabs(out,tabs+2) << c._place << "\n";
                    generateTabs(out,tabs+2) << "<tokens-count>\n";
                    generateTabs(out,tabs+1) << "</integer-gt>\n";                      
                }
                generateTabs(out,tabs) << "</conjunction>\n";
            }
            if(_negated) generateTabs(out,--tabs) << "<negation>";
        }

        void EqualCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<integer-eq>\n";
            _expr1->toXML(out,tabs+1);
            _expr2->toXML(out,tabs+1);
            generateTabs(out,tabs) << "</integer-eq>\n";  
        }
        
        void NotEqualCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<integer-ne>\n";
            _expr1->toXML(out,tabs+1);
            _expr2->toXML(out,tabs+1);
            generateTabs(out,tabs) << "</integer-ne>\n";  
        }
        
        void LessThanCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<integer-lt>\n";
            _expr1->toXML(out,tabs+1);
            _expr2->toXML(out,tabs+1);
            generateTabs(out,tabs) << "</integer-lt>\n";  
        }
        
        void LessThanOrEqualCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<integer-le>\n";
            _expr1->toXML(out,tabs+1);
            _expr2->toXML(out,tabs+1);
            generateTabs(out,tabs) << "</integer-le>\n";  
        }
        
        void GreaterThanCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<integer-gt>\n";
            _expr1->toXML(out,tabs+1);
            _expr2->toXML(out,tabs+1);
            generateTabs(out,tabs) << "</integer-gt>\n";  
        }
        
        void GreaterThanOrEqualCondition::toXML(std::ostream& out,uint32_t tabs) const {
            
            generateTabs(out,tabs) << "<integer-ge>\n";
            _expr1->toXML(out,tabs+1);
            _expr2->toXML(out,tabs+1);
            generateTabs(out,tabs) << "</integer-ge>\n";  
        }
        
        void NotCondition::toXML(std::ostream& out,uint32_t tabs) const {
            
            generateTabs(out,tabs) << "<negation>\n";
            _cond->toXML(out,tabs+1);
            generateTabs(out,tabs) << "</negation>\n";  
        }
        
        void BooleanCondition::toXML(std::ostream& out,uint32_t tabs) const {            
            generateTabs(out,tabs) << "<" << 
                    (_value ? "true" : "false") 
                    << "/>\n"; 
        }
        
        void DeadlockCondition::toXML(std::ostream& out,uint32_t tabs) const {
            generateTabs(out,tabs) << "<deadlock/>\n"; 
        }
        
        /******************** Query Simplification ********************/       
        
        Member LiteralExpr::constraint(SimplificationContext& context) const {
            return Member(_value);
        }
        
        Member memberForPlace(size_t p, SimplificationContext& context) 
        {
            std::vector<int> row(context.net()->numberOfTransitions(), 0);
            row.shrink_to_fit();
            for (size_t t = 0; t < context.net()->numberOfTransitions(); t++) {
                row[t] = context.net()->outArc(t, p) - context.net()->inArc(p, t);
            }
            return Member(std::move(row), context.marking()[p]);            
        }
        
        Member IdentifierExpr::constraint(SimplificationContext& context) const {
            return memberForPlace(_offsetInMarking, context);
        }
        
        Member CommutativeExpr::commutativeCons(int constant, SimplificationContext& context, std::function<void(Member& a, Member b)> op) const
        {
            Member res;
            bool first = true;
            if(_constant != constant)
            {
                first = false;
                res = Member(_constant);
            }
            
            for(auto& i : _ids)
            {
                if(first) res = memberForPlace(i.first, context);
                else op(res, memberForPlace(i.first, context));
                first = false;
            }

            for(auto& e : _exprs)
            {
                if(first) res = e->constraint(context);
                else op(res, e->constraint(context));
                first = false;
            }
            return res;            
        }
        
        Member PlusExpr::constraint(SimplificationContext& context) const {
            return commutativeCons(0, context, [](auto& a , auto b){ a += b;});
        }
        
        Member SubtractExpr::constraint(SimplificationContext& context) const {
            Member res = _exprs[0]->constraint(context);
            for(size_t i = 1; i < _exprs.size(); ++i) res -= _exprs[i]->constraint(context);
            return res;
        }
        
        Member MultiplyExpr::constraint(SimplificationContext& context) const {
            return commutativeCons(1, context, [](auto& a , auto b){ a *= b;});
        }
        
        Member MinusExpr::constraint(SimplificationContext& context) const {
            Member neg(-1);
            return _expr->constraint(context) *= neg;
        }
        
        Retval simplifyEX(Retval& r, SimplificationContext& context) {
            auto st = context.getLpTimeout();
            context.setLpTimeout(1);
            if(r.formula->isTriviallyTrue() || !r.neglps->satisfiable(context)) {
                context.setLpTimeout(st);
                return Retval(std::make_shared<NotCondition>(
                        std::make_shared<DeadlockCondition>()));
            } else if(r.formula->isTriviallyFalse() || !r.lps->satisfiable(context)) {
                context.setLpTimeout(st);
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } else {
                context.setLpTimeout(st);
                return Retval(std::make_shared<EXCondition>(r.formula));
            }
        }
        
        Retval simplifyAX(Retval& r, SimplificationContext& context) {
            auto st = context.getLpTimeout();
            context.setLpTimeout(1);
            if(r.formula->isTriviallyTrue() || !r.neglps->satisfiable(context)){
                context.setLpTimeout(st);
                return Retval(BooleanCondition::TRUE_CONSTANT);
            } else if(r.formula->isTriviallyFalse() || !r.lps->satisfiable(context)){
                context.setLpTimeout(st);
                return Retval(std::make_shared<DeadlockCondition>());
            } else{
                context.setLpTimeout(st);
                return Retval(std::make_shared<AXCondition>(r.formula));
            }
        }
        
        Retval simplifyEF(Retval& r, SimplificationContext& context){
            auto st = context.getLpTimeout();
            context.setLpTimeout(1);
            if(r.formula->isTriviallyTrue() || !r.neglps->satisfiable(context)){
                context.setLpTimeout(st);
                return Retval(BooleanCondition::TRUE_CONSTANT);
            } else if(r.formula->isTriviallyFalse() || !r.lps->satisfiable(context)){
                context.setLpTimeout(st);
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } else {
                context.setLpTimeout(st);
                return Retval(std::make_shared<EFCondition>(r.formula));
            }
        }
        
        Retval simplifyAF(Retval& r, SimplificationContext& context){
            auto st = context.getLpTimeout();
            context.setLpTimeout(1);
            if(r.formula->isTriviallyTrue() || !r.neglps->satisfiable(context)){
                context.setLpTimeout(st);
                return Retval(BooleanCondition::TRUE_CONSTANT);
            } else if(r.formula->isTriviallyFalse() || !r.lps->satisfiable(context)){
                context.setLpTimeout(st);
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } else {
                context.setLpTimeout(st);
                return Retval(std::make_shared<AFCondition>(r.formula));
            }
        }
        
        Retval simplifyEG(Retval& r, SimplificationContext& context){
            auto st = context.getLpTimeout();
            context.setLpTimeout(1);
            if(r.formula->isTriviallyTrue() || !r.neglps->satisfiable(context)){
                context.setLpTimeout(st);
                return Retval(BooleanCondition::TRUE_CONSTANT);
            } else if(r.formula->isTriviallyFalse() || !r.lps->satisfiable(context)){
                context.setLpTimeout(st);
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } else {
                context.setLpTimeout(st);
                return Retval(std::make_shared<EGCondition>(r.formula));
            }
        }
        
        Retval simplifyAG(Retval& r, SimplificationContext& context){
            auto st = context.getLpTimeout();
            context.setLpTimeout(1);
            if(r.formula->isTriviallyTrue() || !r.neglps->satisfiable(context)){
                context.setLpTimeout(st);
                return Retval(BooleanCondition::TRUE_CONSTANT);
            } else if(r.formula->isTriviallyFalse() || !r.lps->satisfiable(context)){
                context.setLpTimeout(st);
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } else {
                context.setLpTimeout(st);
                return Retval(std::make_shared<AGCondition>(r.formula));
            }
        }
        
        Retval EXCondition::simplify(SimplificationContext& context) const {
            Retval r = _cond->simplify(context);
            return context.negated() ? simplifyAX(r, context) : simplifyEX(r, context);
        }
        
        Retval AXCondition::simplify(SimplificationContext& context) const {
            Retval r = _cond->simplify(context);
            return context.negated() ? simplifyEX(r, context) : simplifyAX(r, context);
        }  
        
        Retval EFCondition::simplify(SimplificationContext& context) const {
            Retval r = _cond->simplify(context);
            return context.negated() ? simplifyAG(r, context) : simplifyEF(r, context);  
        }
        
        Retval AFCondition::simplify(SimplificationContext& context) const {
            Retval r = _cond->simplify(context);
            return context.negated() ? simplifyEG(r, context) : simplifyAF(r, context);  
        }
        
        Retval EGCondition::simplify(SimplificationContext& context) const {
            Retval r = _cond->simplify(context);
            return context.negated() ? simplifyAF(r, context) : simplifyEG(r, context);  
        }
        
        Retval AGCondition::simplify(SimplificationContext& context) const {
            Retval r = _cond->simplify(context);
            return context.negated() ? simplifyEF(r, context) : simplifyAG(r, context);  
        }
        
        Retval EUCondition::simplify(SimplificationContext& context) const {
            // cannot push negation any further
            bool neg = context.negated();
            context.setNegate(false);
            Retval r2 = _cond2->simplify(context);
            if(r2.formula->isTriviallyTrue() || !r2.neglps->satisfiable(context))
            {
                context.setNegate(neg);
                return neg ? 
                            Retval(BooleanCondition::FALSE_CONSTANT) :
                            Retval(BooleanCondition::TRUE_CONSTANT);
            }
            else if(r2.formula->isTriviallyFalse() || !r2.lps->satisfiable(context))
            {
                context.setNegate(neg);
                return neg ? 
                            Retval(BooleanCondition::TRUE_CONSTANT) :
                            Retval(BooleanCondition::FALSE_CONSTANT);                
            }
            Retval r1 = _cond1->simplify(context);
            context.setNegate(neg);
            
            if(context.negated()){
                if(r1.formula->isTriviallyTrue() || !r1.neglps->satisfiable(context)){
                    return Retval(std::make_shared<NotCondition>(
                            std::make_shared<EFCondition>(r2.formula)));
                } else if(r1.formula->isTriviallyFalse() || !r1.lps->satisfiable(context)){
                    return Retval(std::make_shared<NotCondition>(r2.formula));
                } else {
                    return Retval(std::make_shared<NotCondition>(
                            std::make_shared<EUCondition>(r1.formula, r2.formula)));
                }
            } else {
                if(r1.formula->isTriviallyTrue() || !r1.neglps->satisfiable(context)){
                    return Retval(std::make_shared<EFCondition>(r2.formula));
                } else if(r1.formula->isTriviallyFalse() || !r1.lps->satisfiable(context)){
                    return r2;
                } else {
                    return Retval(std::make_shared<EUCondition>(r1.formula, r2.formula));
                }
            }
        }
        
        Retval AUCondition::simplify(SimplificationContext& context) const {
            // cannot push negation any further
            bool neg = context.negated();
            context.setNegate(false);
            Retval r2 = _cond2->simplify(context);
            if(r2.formula->isTriviallyTrue() || !r2.neglps->satisfiable(context))
            {
                context.setNegate(neg);
                return neg ? 
                            Retval(BooleanCondition::FALSE_CONSTANT) :
                            Retval(BooleanCondition::TRUE_CONSTANT);
            }
            else if(r2.formula->isTriviallyFalse() || !r2.lps->satisfiable(context))
            {
                context.setNegate(neg);
                return neg ? 
                            Retval(BooleanCondition::TRUE_CONSTANT) :
                            Retval(BooleanCondition::FALSE_CONSTANT);                
            }
            Retval r1 = _cond1->simplify(context);
            context.setNegate(neg);
            
            if(context.negated()){
                if(r1.formula->isTriviallyTrue() || !r1.neglps->satisfiable(context)){
                    return Retval(std::make_shared<NotCondition>(
                            std::make_shared<AFCondition>(r2.formula)));
                } else if(r1.formula->isTriviallyFalse() || !r1.lps->satisfiable(context)){
                    return Retval(std::make_shared<NotCondition>(r2.formula));
                } else {
                    return Retval(std::make_shared<NotCondition>(
                            std::make_shared<AUCondition>(r1.formula, r2.formula)));
                }
            } else {
                if(r1.formula->isTriviallyTrue() || !r1.neglps->satisfiable(context)){
                    return Retval(std::make_shared<AFCondition>(r2.formula));
                } else if(r1.formula->isTriviallyFalse() || !r1.lps->satisfiable(context)){
                    return r2;
                } else {
                    return Retval(std::make_shared<AUCondition>(r1.formula, r2.formula));
                }
            }
        }
        
        Retval LogicalCondition::simplifyAnd(SimplificationContext& context) const {

            std::vector<Condition_ptr> conditions;
            AbstractProgramCollection_ptr lps = nullptr;
            std::vector<AbstractProgramCollection_ptr>  neglps;
            for(auto& c : _conds)
            {
                auto r = c->simplify(context);
                if(r.formula->isTriviallyFalse())
                {
                    return Retval(BooleanCondition::FALSE_CONSTANT);
                }
                else if(r.formula->isTriviallyTrue())
                {
                    continue;
                }
                
                conditions.push_back(r.formula);
                if(lps == nullptr) lps = r.lps;
                else lps = std::make_shared<MergeCollection>(lps, r.lps);
                neglps.push_back(r.neglps);
            }
            
            if(conditions.size() == 0)
            {
                return Retval(BooleanCondition::TRUE_CONSTANT);
            }


            try {
                if(!context.timeout() && !lps->satisfiable(context))
                {
                    return Retval(BooleanCondition::FALSE_CONSTANT);
                }           
             }
             catch(std::bad_alloc& e)
             {
                // we are out of memory, deal with it.
                std::cout<<"Query reduction: memory exceeded during LPS merge."<<std::endl;
             }
            
            // Lets try to see if the r1 AND r2 can ever be false at the same time
            // If not, then we know that r1 || r2 must be true.
            // we check this by checking if !r1 && !r2 is unsat
            
            return Retval(
                    makeAnd(conditions), 
                    std::move(lps),
                    std::make_shared<UnionCollection>(std::move(neglps)));
        }
        
        Retval LogicalCondition::simplifyOr(SimplificationContext& context) const {

            std::vector<Condition_ptr> conditions;
            std::vector<AbstractProgramCollection_ptr> lps;
            AbstractProgramCollection_ptr  neglps = nullptr;
            for(auto& c : _conds)
            {
                auto r = c->simplify(context);
                assert(r.neglps);
                assert(r.lps);

                if(r.formula->isTriviallyTrue())
                {
                    return Retval(BooleanCondition::TRUE_CONSTANT);
                }
                else if(r.formula->isTriviallyFalse())
                {
                    continue;
                }
                conditions.push_back(r.formula);
                lps.push_back(r.lps);
                if(neglps == nullptr) neglps = r.neglps;
                else neglps = std::make_shared<MergeCollection>(neglps, r.neglps);
            }
            
            if(conditions.size() == 0)
            {
                return Retval(BooleanCondition::FALSE_CONSTANT);
            }

            try {
               if(!context.timeout() && !neglps->satisfiable(context))
               {
                   return Retval(BooleanCondition::TRUE_CONSTANT);
               }           
            }
            catch(std::bad_alloc& e)
            {
               // we are out of memory, deal with it.
               std::cout<<"Query reduction: memory exceeded during LPS merge."<<std::endl;
            }

            // Lets try to see if the r1 AND r2 can ever be false at the same time
            // If not, then we know that r1 || r2 must be true.
            // we check this by checking if !r1 && !r2 is unsat
          
            return Retval(
                    makeOr(conditions), 
                    std::make_shared<UnionCollection>(std::move(lps)), 
                    std::move(neglps));            
        }
        
        Retval AndCondition::simplify(SimplificationContext& context) const {
            if(context.timeout())
            {
                if(context.negated()) 
                    return Retval(std::make_shared<NotCondition>(
                            makeAnd(_conds)));
                else                  
                    return Retval(
                            makeAnd(_conds));
            }

            if(context.negated())
                return simplifyOr(context);
            else
                return simplifyAnd(context);
            
        }
        
        Retval OrCondition::simplify(SimplificationContext& context) const {            
            if(context.timeout())
            {
                if(context.negated()) 
                    return Retval(std::make_shared<NotCondition>(
                            makeOr(_conds)));
                else                 
                    return Retval(makeOr(_conds));
            }
            if(context.negated())
                return simplifyAnd(context);
            else
                return simplifyOr(context);
        }
        
        Retval CompareConjunction::simplify(SimplificationContext& context) const {
            if(context.timeout())
            {
                return Retval(std::make_shared<CompareConjunction>(*this, context.negated()));
            }
            AbstractProgramCollection_ptr lps = nullptr;
            std::vector<AbstractProgramCollection_ptr>  neglps;
            auto neg = context.negated() != _negated;
            std::vector<cons_t> nconstraints;
//            std::vector<Condition_ptr> conditions;
            for(auto& c : _constraints) 
            {
                nconstraints.push_back(c);
                if(c._lower != 0 /*&& !context.timeout()*/ )
                {
//                    conditions.push_back(std::make_shared<GreaterThanOrEqualCondition>(std::make_shared<IdentifierExpr>(c._name, c._place), std::make_shared<LiteralExpr>(c._lower)));
                    auto m2 = memberForPlace(c._place, context);
                    Member m1(c._lower);
                    // test for trivial comparison
                    Trivial eval = m1 <= m2;
                    if(eval != Trivial::Indeterminate) {
                        if(eval == Trivial::False)
                            return Retval(BooleanCondition::getShared(neg));
                        else
                            nconstraints.back()._lower = 0;
                    } else { // if no trivial case
                        int constant = m2.constant() - m1.constant();
                        m1 -= m2;
                        m2 = m1;
                        auto lp = std::make_shared<SingleProgram>(context.cache(), std::move(m1), constant, Simplification::OP_LE);
                        auto nlp = std::make_shared<SingleProgram>(context.cache(), std::move(m2), constant, Simplification::OP_GT);
                        if(lps == nullptr) lps = lp;
                        else lps = std::make_shared<MergeCollection>(lps, lp);
                        neglps.push_back(nlp);
                   }
                }
         
                if(c._upper != std::numeric_limits<uint32_t>::max() /*&& !context.timeout()*/)
                {
//                    conditions.push_back(std::make_shared<GreaterThanOrEqualCondition>(std::make_shared<LiteralExpr>(c._upper), std::make_shared<IdentifierExpr>(c._name, c._place)));
                    auto m1 = memberForPlace(c._place, context);
                    Member m2(c._upper);
                    // test for trivial comparison
                    Trivial eval = m1 <= m2;
                    if(eval != Trivial::Indeterminate) {
                        if(eval == Trivial::False)
                            return Retval(BooleanCondition::getShared(neg));
                        else
                            nconstraints.back()._upper = std::numeric_limits<uint32_t>::max();
                    } else { // if no trivial case
                        int constant = m2.constant() - m1.constant();
                        m1 -= m2;
                        m2 = m1;
                        auto lp = std::make_shared<SingleProgram>(context.cache(), std::move(m1), constant, Simplification::OP_LE);
                        auto nlp = std::make_shared<SingleProgram>(context.cache(), std::move(m2), constant, Simplification::OP_GT);
                        if(lps == nullptr) lps = lp;
                        else lps = std::make_shared<MergeCollection>(lps, lp);
                        neglps.push_back(nlp);
                   }
                }
                
                assert(nconstraints.size() > 0);
                if(nconstraints.back()._lower == 0 && nconstraints.back()._upper == std::numeric_limits<uint32_t>::max())
                    nconstraints.pop_back();

                assert(nconstraints.size() <= neglps.size()*2);
            }
            
            if(lps == nullptr && !context.timeout()) 
            {
                return Retval(BooleanCondition::getShared(!neg));
            }
            
            try {
                if(!context.timeout() && lps &&  !lps->satisfiable(context))
                {
                    return Retval(BooleanCondition::getShared(neg));
                }           
             }
             catch(std::bad_alloc& e)
             {
                // we are out of memory, deal with it.
                std::cout<<"Query reduction: memory exceeded during LPS merge."<<std::endl;
             }
            // Lets try to see if the r1 AND r2 can ever be false at the same time
            // If not, then we know that r1 || r2 must be true.
            // we check this by checking if !r1 && !r2 is unsat
            auto pt = context.getLpTimeout();
            context.setLpTimeout(1);
            try {
                // remove trivial rules from neglp
                int ncnt = neglps.size() - 1;
                for(int i = nconstraints.size() - 1; i >= 0; --i) 
                {
                    if(context.timeout()) break;
                    assert(ncnt >= 0);
                    size_t cnt = 0;
                    auto& c = nconstraints[i];
                    if(c._lower != 0) ++cnt;
                    if(c._upper != std::numeric_limits<uint32_t>::max()) ++cnt;
                    for(size_t j = 0; j < cnt ; ++j)
                    {
                        assert(ncnt >= 0);
                        if(!neglps[ncnt]->satisfiable(context))
                        {
                            if(j == 1 || c._upper == std::numeric_limits<uint32_t>::max())
                                c._lower = 0;
                            else if(j == 0)
                                c._upper = std::numeric_limits<uint32_t>::max();
                            neglps.erase(neglps.begin() + ncnt);
                        }
                        if(c._upper == std::numeric_limits<uint32_t>::max() && c._lower == 0) 
                            nconstraints.erase(nconstraints.begin() + i);
                        --ncnt;
                    }
                }
            }
            catch(std::bad_alloc& e)
            {
               // we are out of memory, deal with it.
               std::cout<<"Query reduction: memory exceeded during LPS merge."<<std::endl;
            }            
            context.setLpTimeout(pt);
            if(nconstraints.size() == 0)
            {
                return Retval(BooleanCondition::getShared(!neg));                
            }

            
            Condition_ptr rc = [&]() -> Condition_ptr {
                if(nconstraints.size() == 1)
                {
                    auto& c = nconstraints[0];
                    auto id = std::make_shared<IdentifierExpr>(c._name, c._place);
                    auto ll = std::make_shared<LiteralExpr>(c._lower);
                    auto lu = std::make_shared<LiteralExpr>(c._upper);
                    if     (c._lower == c._upper && !neg) return std::make_shared<EqualCondition>(id, lu);
                    else if(c._lower == c._upper &&  neg) return std::make_shared<NotEqualCondition>(id, lu);
                    else
                    {
                        if(c._lower != 0 && c._upper != std::numeric_limits<uint32_t>::max())
                        {
                            if(neg) return makeOr(std::make_shared<LessThanCondition>(id, ll),std::make_shared<GreaterThanCondition>(id, lu));
                            else    return makeAnd(std::make_shared<GreaterThanOrEqualCondition>(id, ll),std::make_shared<LessThanOrEqualCondition>(id, lu));
                        }
                        else if(c._lower != 0)
                        {
                            if(neg) return std::make_shared<LessThanCondition>(id, ll);
                            else    return std::make_shared<GreaterThanOrEqualCondition>(id, ll);                       
                        }
                        else
                        {
                            if(neg) return std::make_shared<GreaterThanCondition>(id, lu);
                            else    return std::make_shared<LessThanOrEqualCondition>(id, lu);                        
                        }
                    }
                }
                else
                {
                    return std::make_shared<CompareConjunction>(std::move(nconstraints), context.negated() != _negated);
                }
            }();
            

            if(!neg)
            {
                return Retval(
                    rc, 
                    std::move(lps),
                    std::make_shared<UnionCollection>(std::move(neglps)));
            }
            else
            {
                return Retval(
                    rc, 
                    std::make_shared<UnionCollection>(std::move(neglps)),
                    std::move(lps));                
            }
        }

        Retval EqualCondition::simplify(SimplificationContext& context) const {
            
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            std::shared_ptr<AbstractProgramCollection> lps, neglps;
            if (!context.timeout() && m1.canAnalyze() && m2.canAnalyze()) {
                if ((m1.isZero() && m2.isZero()) || m1.substrationIsZero(m2)) {
                    return Retval(BooleanCondition::getShared(
                        context.negated() ? (m1.constant() != m2.constant()) : (m1.constant() == m2.constant())));
                } else {
                    int constant = m2.constant() - m1.constant();
                    m1 -= m2;
                    m2 = m1;
                    neglps = 
                            std::make_shared<UnionCollection>(
                            std::make_shared<SingleProgram>(context.cache(), std::move(m1), constant, Simplification::OP_GT),
                            std::make_shared<SingleProgram>(context.cache(), std::move(m2), constant, Simplification::OP_LT));
                    Member m3 = m2;
                    lps = std::make_shared<SingleProgram>(context.cache(), std::move(m3), constant, Simplification::OP_EQ);
                    
                    if(context.negated()) lps.swap(neglps);
                }
            } else {
                lps = std::make_shared<SingleProgram>();
                neglps = std::make_shared<SingleProgram>();
            }
            
            if (!context.timeout() && !lps->satisfiable(context)) {
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } 
            else if(!context.timeout() && !neglps->satisfiable(context))
            {
                return Retval(BooleanCondition::TRUE_CONSTANT);            
            } 
            else {
                if (context.negated()) {
                    return Retval(std::make_shared<NotEqualCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                } else {
                    return Retval(std::make_shared<EqualCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                }
            }
        }
        
        Retval NotEqualCondition::simplify(SimplificationContext& context) const {
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            std::shared_ptr<AbstractProgramCollection> lps, neglps;
            if (!context.timeout() && m1.canAnalyze() && m2.canAnalyze()) {
                if ((m1.isZero() && m2.isZero()) || m1.substrationIsZero(m2)) {
                    return Retval(std::make_shared<BooleanCondition>(
                        context.negated() ? (m1.constant() == m2.constant()) : (m1.constant() != m2.constant())));
                } else{ 
                    int constant = m2.constant() - m1.constant();
                    m1 -= m2;
                    m2 = m1;
                    lps = 
                            std::make_shared<UnionCollection>(
                            std::make_shared<SingleProgram>(context.cache(), std::move(m1), constant, Simplification::OP_GT),
                            std::make_shared<SingleProgram>(context.cache(), std::move(m2), constant, Simplification::OP_LT));
                    Member m3 = m2;
                    neglps = std::make_shared<SingleProgram>(context.cache(), std::move(m3), constant, Simplification::OP_EQ); 
                    
                    if(context.negated()) lps.swap(neglps);
                }
            } else {
                lps = std::make_shared<SingleProgram>();
                neglps = std::make_shared<SingleProgram>();
            }
            if (!context.timeout() && !lps->satisfiable(context)) {
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } 
            else if(!context.timeout() && !neglps->satisfiable(context))
            {
                return Retval(BooleanCondition::TRUE_CONSTANT);            
            }
            else {
                if (context.negated()) {
                    return Retval(std::make_shared<EqualCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                } else {
                    return Retval(std::make_shared<NotEqualCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                }                         
            }
        }
            
        Retval LessThanCondition::simplify(SimplificationContext& context) const {
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            AbstractProgramCollection_ptr lps, neglps;
            if (!context.timeout() && m1.canAnalyze() && m2.canAnalyze()) {
                // test for trivial comparison
                Trivial eval = context.negated() ? m1 >= m2 : m1 < m2;
                if(eval != Trivial::Indeterminate) {
                    return Retval(BooleanCondition::getShared(eval == Trivial::True));
                } else { // if no trivial case
                    int constant = m2.constant() - m1.constant();
                    m1 -= m2;
                    m2 = m1;
                    lps = std::make_shared<SingleProgram>(context.cache(), std::move(m1), constant, (context.negated() ? Simplification::OP_GE : Simplification::OP_LT));
                    neglps = std::make_shared<SingleProgram>(context.cache(), std::move(m2), constant, (!context.negated() ? Simplification::OP_GE : Simplification::OP_LT));
                }
            } else {
                lps = std::make_shared<SingleProgram>();
                neglps = std::make_shared<SingleProgram>();
            }
            
            if (!context.timeout() && !lps->satisfiable(context)) {
                return Retval(BooleanCondition::FALSE_CONSTANT);
            }
            else if(!context.timeout() && !neglps->satisfiable(context))
            {
                return Retval(BooleanCondition::TRUE_CONSTANT);                
            }
            else {
                if (context.negated()) {
                    return Retval(std::make_shared<GreaterThanOrEqualCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                } else {
                    return Retval(std::make_shared<LessThanCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                }                         
            }
        }        
        
        Retval LessThanOrEqualCondition::simplify(SimplificationContext& context) const {
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            
            AbstractProgramCollection_ptr lps, neglps;
            if (!context.timeout() && m1.canAnalyze() && m2.canAnalyze()) {
                // test for trivial comparison
                Trivial eval = context.negated() ? m1 > m2 : m1 <= m2;
                if(eval != Trivial::Indeterminate) {
                    return Retval(BooleanCondition::getShared(eval == Trivial::True));
                } else { // if no trivial case
                    int constant = m2.constant() - m1.constant();
                    m1 -= m2;
                    m2 = m1;
                    lps = std::make_shared<SingleProgram>(context.cache(), std::move(m1), constant, (context.negated() ? Simplification::OP_GT : Simplification::OP_LE));
                    neglps = std::make_shared<SingleProgram>(context.cache(), std::move(m2), constant, (context.negated() ? Simplification::OP_LE : Simplification::OP_GT));
                }
            } else {
                lps = std::make_shared<SingleProgram>();
                neglps = std::make_shared<SingleProgram>();
            }
            
            assert(lps);
            assert(neglps);

            if(!context.timeout() && !neglps->satisfiable(context)){
                return Retval(BooleanCondition::TRUE_CONSTANT);
            } else if (!context.timeout() && !lps->satisfiable(context)) {
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } else {
                if (context.negated()) {
                    return Retval(std::make_shared<GreaterThanCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                } else {
                    return Retval(std::make_shared<LessThanOrEqualCondition>(_expr1, _expr2), 
                            std::move(lps), std::move(neglps));
                }                         
            }
        }
        
        Retval GreaterThanCondition::simplify(SimplificationContext& context) const {
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            
            AbstractProgramCollection_ptr lps, neglps;
            if (!context.timeout() && m1.canAnalyze() && m2.canAnalyze()) {
                // test for trivial comparison
                Trivial eval = context.negated() ? m1 <= m2 : m1 > m2;
                if(eval != Trivial::Indeterminate) {
                    return Retval(BooleanCondition::getShared(eval == Trivial::True));
                } else { // if no trivial case
                    int constant = m2.constant() - m1.constant();
                    m1 -= m2;
                    m2 = m1;
                    lps = std::make_shared<SingleProgram>(context.cache(), std::move(m1), constant, (context.negated() ? Simplification::OP_LE : Simplification::OP_GT));
                    neglps = std::make_shared<SingleProgram>(context.cache(), std::move(m2), constant, (context.negated() ? Simplification::OP_GT : Simplification::OP_LE));
                }
            } else {
                lps = std::make_shared<SingleProgram>();
                neglps = std::make_shared<SingleProgram>();
            }
            
            if(!context.timeout() && !neglps->satisfiable(context)) {
                return Retval(BooleanCondition::TRUE_CONSTANT);
            }else if(!context.timeout() && !lps->satisfiable(context)) {
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } else {
                if (context.negated()) {
                    return Retval(std::make_shared<LessThanOrEqualCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                } else {
                    return Retval(std::make_shared<GreaterThanCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                }                         
            }
        }
        
        Retval GreaterThanOrEqualCondition::simplify(SimplificationContext& context) const {  
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            
            AbstractProgramCollection_ptr lps, neglps;
            if (!context.timeout() && m1.canAnalyze() && m2.canAnalyze()) {
                // test for trivial comparison
                Trivial eval = context.negated() ? m1 < m2 : m1 >= m2;
                if(eval != Trivial::Indeterminate) {
                    return Retval(BooleanCondition::getShared(eval == Trivial::True));
                } else { // if no trivial case
                    int constant = m2.constant() - m1.constant();
                    m1 -= m2;
                    m2 = m1;
                    lps = std::make_shared<SingleProgram>(context.cache(), std::move(m1), constant, (context.negated() ? Simplification::OP_LT : Simplification::OP_GE));
                    neglps = std::make_shared<SingleProgram>(context.cache(), std::move(m2), constant, (!context.negated() ? Simplification::OP_LT : Simplification::OP_GE));
                }
            } else {
                lps = std::make_shared<SingleProgram>();
                neglps = std::make_shared<SingleProgram>();
            }
            if (!context.timeout() && !lps->satisfiable(context)) 
            {
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } 
            else if(!context.timeout() && !neglps->satisfiable(context)) // EXPERIMENTAL
            {
                return Retval(BooleanCondition::TRUE_CONSTANT);
            }
            else {
                if (context.negated()) {
                    return Retval(std::make_shared<LessThanCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                } else {
                    return Retval(std::make_shared<GreaterThanOrEqualCondition>(_expr1, _expr2), std::move(lps), std::move(neglps));
                }                         
            }
        }
        
        Retval NotCondition::simplify(SimplificationContext& context) const {
            context.negate();
            Retval r = _cond->simplify(context);
            context.negate();
            return r;
        }
        
        Retval BooleanCondition::simplify(SimplificationContext& context) const {
            if (context.negated()) {
                return Retval(getShared(!_value));
            } else {
                return Retval(getShared(_value));
            }
        }
        
        Retval DeadlockCondition::simplify(SimplificationContext& context) const {
            if (context.negated()) {
                return Retval(std::make_shared<NotCondition>(DeadlockCondition::DEADLOCK));
            } else {
                return Retval(DeadlockCondition::DEADLOCK);
            }
        }
        
        /******************** Check if query is a reachability query ********************/
        
        bool EXCondition::isReachability(uint32_t depth) const {
            return false;
        }
        
        bool EGCondition::isReachability(uint32_t depth) const {
            return false;
        }
        
        bool EFCondition::isReachability(uint32_t depth) const {
            return depth > 0 ? false : _cond->isReachability(depth + 1);
        }
        
        bool AXCondition::isReachability(uint32_t depth) const {
            return false;
        }
        
        bool AGCondition::isReachability(uint32_t depth) const {
            return depth > 0 ? false : _cond->isReachability(depth + 1);
        }
        
        bool AFCondition::isReachability(uint32_t depth) const {
            return false;
        }
        
        bool UntilCondition::isReachability(uint32_t depth) const {
            return false;
        }
        
        bool LogicalCondition::isReachability(uint32_t depth) const {
            if(depth == 0) return false;
            bool reachability = true;
            for(auto& c : _conds)
            {
                reachability = reachability && c->isReachability(depth + 1);
                if(!reachability) break;
            }
            return reachability;
        }
        
        bool CompareCondition::isReachability(uint32_t depth) const {
            return depth > 0;
        }
        
        bool NotCondition::isReachability(uint32_t depth) const {
            return _cond->isReachability(depth);
        }
        
        bool BooleanCondition::isReachability(uint32_t depth) const {
            return depth > 0;
        }
        
        bool DeadlockCondition::isReachability(uint32_t depth) const {
            return depth > 0;
        }
        
        /******************** Check if query is an upper bound query ********************/
        
        bool SimpleQuantifierCondition::isUpperBound() {
            return _cond->isUpperBound();
        }
        
        bool UntilCondition::isUpperBound() {
            return false;
        }
        
        bool LogicalCondition::isUpperBound() {
            if (placeNameForBound().size() != 0) {
                return true;
            } else {
                bool upper_bound = true;
                for(auto& c : _conds)
                {
                    upper_bound = upper_bound && c->isUpperBound();
                    if(!upper_bound) break;
                }
                return upper_bound;
            }
        }
       
        bool CompareCondition::isUpperBound() {
            return placeNameForBound().size() > 0;
        }
        
        bool NotCondition::isUpperBound() {
            return false;
        }
        
        bool BooleanCondition::isUpperBound() {
            return false;
        }
        
        bool DeadlockCondition::isUpperBound() {
            return false;
        }
        
        
        /******************** Prepare Reachability Queries ********************/
        
        Condition_ptr EXCondition::prepareForReachability(bool negated) const {
            return NULL;
        }
        
        Condition_ptr EGCondition::prepareForReachability(bool negated) const {
            return NULL;
        }
        
        Condition_ptr EFCondition::prepareForReachability(bool negated) const {
            _cond->setInvariant(negated);
            return _cond;
        }
        
        Condition_ptr AXCondition::prepareForReachability(bool negated) const {
            return NULL;
        }
        
        Condition_ptr AGCondition::prepareForReachability(bool negated) const {
            Condition_ptr cond = std::make_shared<NotCondition>(_cond);
            cond->setInvariant(!negated);
            return cond;
        }
        
        Condition_ptr AFCondition::prepareForReachability(bool negated) const {
            return NULL;
        }
        
        Condition_ptr UntilCondition::prepareForReachability(bool negated) const {
            return NULL;
        }
        
        Condition_ptr LogicalCondition::prepareForReachability(bool negated) const {
            return NULL;
        }

        Condition_ptr CompareConjunction::prepareForReachability(bool negated) const {
            return NULL;
        }
        
        Condition_ptr CompareCondition::prepareForReachability(bool negated) const {
            return NULL;
        }
        
        Condition_ptr NotCondition::prepareForReachability(bool negated) const {
            return _cond->prepareForReachability(!negated);
        }
        
        Condition_ptr BooleanCondition::prepareForReachability(bool negated) const {
            return NULL;
        }
        
        Condition_ptr DeadlockCondition::prepareForReachability(bool negated) const {
            return NULL;
        }

/******************** Prepare CTL Queries ********************/


        Condition_ptr initialMarkingRW(std::function<Condition_ptr ()> func, negstat_t& stats, const EvaluationContext& context, bool nested, bool negated)
        {
            auto res = func();
            if(!nested)
            {
                auto e = res->evaluate(context);
                if(e != Condition::RUNKNOWN) 
                {
                    return BooleanCondition::getShared(e);
                }
            }
            return res;            
        }
        
        Condition_ptr EGCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            ++stats[0];
            return AFCondition(std::make_shared<NotCondition>(_cond)).pushNegation(stats, context, nested, !negated);
        }

        Condition_ptr AGCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            ++stats[1];
            return EFCondition(std::make_shared<NotCondition>(_cond)).pushNegation(stats, context, nested, !negated);
        }
        
        Condition_ptr EXCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            auto a = _cond->pushNegation(stats, context, true, negated);
            if(negated)
            {
                ++stats[2];
                return AXCondition(a).pushNegation(stats, context, nested, false);
            }
            else
            {
                if(a == BooleanCondition::FALSE_CONSTANT) 
                { ++stats[3]; return a;}
                if(a == BooleanCondition::TRUE_CONSTANT)  
                { ++stats[4]; return std::make_shared<NotCondition>(DeadlockCondition::DEADLOCK); }
                a = std::make_shared<EXCondition>(a);
            }
            return a;
            }, stats, context, nested, negated);
        }

        Condition_ptr AXCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            auto a = _cond->pushNegation(stats, context, true, negated);
            if(negated)
            {
                ++stats[5];
                return EXCondition(a).pushNegation(stats, context, nested, false);
            }
            else
            {
                if(a == BooleanCondition::TRUE_CONSTANT) 
                { ++stats[6]; return a;}
                if(a == BooleanCondition::FALSE_CONSTANT)  
                { ++stats[7]; return DeadlockCondition::DEADLOCK; }
                a = std::make_shared<AXCondition>(a);
            }
            return a;
            }, stats, context, nested, negated);
        }

        
        Condition_ptr EFCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            auto a = _cond->pushNegation(stats, context, true, false);

            if(auto cond = dynamic_cast<NotCondition*>(a.get()))
            {
                if((*cond)[0] == DeadlockCondition::DEADLOCK)
                {
                    ++stats[8];
                    return a->pushNegation(stats, context, nested, negated);
                }                
            }

            if(!a->isTemporal())
            {
                auto res = std::make_shared<EFCondition>(a);
                if(negated) return std::make_shared<NotCondition>(res);
                return res;
            }
            
            if( dynamic_cast<EFCondition*>(a.get()))
            {
                ++stats[9];
                if(negated) a = std::make_shared<NotCondition>(a);
                return a;
            }
            else if(auto cond = dynamic_cast<AFCondition*>(a.get()))
            {
                ++stats[10];
                a = EFCondition((*cond)[0]).pushNegation(stats, context, nested, negated);
                return a;
            }
            else if(auto cond = dynamic_cast<EUCondition*>(a.get()))
            {
                ++stats[11];
                a = EFCondition((*cond)[1]).pushNegation(stats, context, nested, negated);
                return a;
            }
            else if(auto cond = dynamic_cast<AUCondition*>(a.get()))
            {
                ++stats[12];
                a = EFCondition((*cond)[1]).pushNegation(stats, context, nested, negated);
                return a;
            }
            else if(auto cond = dynamic_cast<OrCondition*>(a.get()))
            {
                if(!cond->isTemporal())
                {
                    Condition_ptr b = std::make_shared<EFCondition>(a);
                    if(negated) b = std::make_shared<NotCondition>(b);
                    return b;
                }
                ++stats[13];
                std::vector<Condition_ptr> pef, atomic;
                for(auto& i : *cond) 
                {
                    pef.push_back(std::make_shared<EFCondition>(i));
                }
                a = makeOr(pef)->pushNegation(stats, context, nested, negated);
                return a;
            }
            else
            {        
                Condition_ptr b = std::make_shared<EFCondition>(a);
                if(negated) b = std::make_shared<NotCondition>(b);
                return b;
            }
            }, stats, context, nested, negated);
        }


        Condition_ptr AFCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            auto a = _cond->pushNegation(stats, context, true, false);
            if(auto cond = dynamic_cast<NotCondition*>(a.get()))
            {
                if((*cond)[0] == DeadlockCondition::DEADLOCK)
                {
                    ++stats[14];
                    return a->pushNegation(stats, context, nested, negated);
                }                
            }
            
            if(dynamic_cast<AFCondition*>(a.get()))
            {
                ++stats[15];
                if(negated) return std::make_shared<NotCondition>(a);
                return a;

            }
            else if(dynamic_cast<EFCondition*>(a.get()))
            {
                ++stats[16];
                if(negated) return std::make_shared<NotCondition>(a);
                return a;
            }
            else if(auto cond = dynamic_cast<OrCondition*>(a.get()))
            {

                std::vector<Condition_ptr> pef, npef;
                for(auto& i : *cond)
                {
                    if(dynamic_cast<EFCondition*>(i.get()))
                    {
                        pef.push_back(i);
                    }
                    else
                    {
                        npef.push_back(i);
                    }
                }
                if(pef.size() > 0)
                {
                    stats[17] += pef.size();
                    pef.push_back(std::make_shared<AFCondition>(makeOr(npef)));
                    return makeOr(pef)->pushNegation(stats, context, nested, negated);
                }
            }
            else if(auto cond = dynamic_cast<AUCondition*>(a.get()))
            {
                ++stats[18];
                return AFCondition((*cond)[1]).pushNegation(stats, context, nested, negated);
            }
            auto b = std::make_shared<AFCondition>(a);
            if(negated) return std::make_shared<NotCondition>(b);
            return b;
            }, stats, context, nested, negated);
        }

        
/*        template<typename T, typename E>
        Condition_ptr pushAg(Condition_ptr& a, Condition_ptr& b, bool negated)
        {
            if(auto cond = dynamic_cast<NotCondition*>(a.get()))
            {
                if(auto c2 = dynamic_cast<EFCondition*>((*cond)[0].get()))
                {
                    Condition_ptr af = std::make_shared<T>(b);
                    return makeOr(
                            b,
                            makeAnd(
                                a,
                                af)
                            , true)->pushNegation(negated, stats);
                }
            }
            else if(auto cond = dynamic_cast<AndCondition*>(a.get()))
            {
                std::vector<Condition_ptr> ag, nag;
                for(auto c : *cond)
                {
                    if(auto n = dynamic_cast<NotCondition*>(c.get()))
                    {
                        if(auto ef = dynamic_cast<EFCondition*>((*n)[0].get()))
                        {
                            ag.push_back(c);
                        }
                        else
                        {
                            nag.push_back(c);
                        }
                    }
                    else
                    {
                        nag.push_back(c);                        
                    }
                }
                if(ag.size() > 0)
                {
                    if(nag.size() == 0)
                    {
                        ag.push_back(std::make_shared<T>(b));
                    }
                    else
                    {
                        ag.push_back(std::make_shared<E>(makeAnd(nag), b));                        
                    }
                    return makeOr(
                            b,
                            makeAnd(ag))->pushNegation(negated, stats);
                }
            }
            return nullptr;            
        }
  */      
        
        Condition_ptr AUCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            auto b = _cond2->pushNegation(stats, context, true, false);
            auto a = _cond1->pushNegation(stats, context, true, false);
            if(auto cond = dynamic_cast<NotCondition*>(b.get()))
            {
                if((*cond)[0] == DeadlockCondition::DEADLOCK)
                {
                    ++stats[19];
                    return b->pushNegation(stats, context, nested, negated);
                }
            }
            else if(a == DeadlockCondition::DEADLOCK)
            {
                ++stats[20];
                return b->pushNegation(stats, context, nested, negated);
            }
            else if(auto cond = dynamic_cast<NotCondition*>(a.get()))
            {
                if((*cond)[0] == DeadlockCondition::DEADLOCK)
                {
                    ++stats[21];
                    return AFCondition(b).pushNegation(stats, context, nested, negated);
                }
            }
            
            if(auto cond = dynamic_cast<AFCondition*>(b.get()))
            {
                ++stats[22];
                return cond->pushNegation(stats, context, nested, negated);
            }
            else if(dynamic_cast<EFCondition*>(b.get()))
            {
                ++stats[23];
                if(negated) return std::make_shared<NotCondition>(b);
                return b;
            }
            else if(auto cond = dynamic_cast<OrCondition*>(b.get()))
            {
                std::vector<Condition_ptr> pef, npef;
                for(auto& i : *cond)
                {
                    if(dynamic_cast<EFCondition*>(i.get()))
                    {
                        pef.push_back(i);
                    }
                    else
                    {
                        npef.push_back(i);
                    }
                }
                if(pef.size() > 0)
                {
                    stats[24] += pef.size();
                    if(npef.size() != 0)
                    {
                        pef.push_back(std::make_shared<AUCondition>(_cond1, makeOr(npef)));
                    }
                    else
                    {
                        ++stats[23];
                        --stats[24];
                    }
                    return makeOr(pef)->pushNegation(stats, context, nested, negated);
                }
            }
            
            /*auto pushag = pushAg<AFCondition, AUCondition>(a, b, negated);
            if(pushag != nullptr) return pushag;*/
            auto c = std::make_shared<AUCondition>(a, b);
            if(negated) return std::make_shared<NotCondition>(c);
            return c;
            }, stats, context, nested, negated);
        }


        Condition_ptr EUCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            auto b = _cond2->pushNegation(stats, context, true, false);
            auto a = _cond1->pushNegation(stats, context, true, false);

            if(auto cond = dynamic_cast<NotCondition*>(b.get()))
            {
                if((*cond)[0] == DeadlockCondition::DEADLOCK)
                {
                    ++stats[25];
                    return b->pushNegation(stats, context, nested, negated);
                }
            }
            else if(a == DeadlockCondition::DEADLOCK)
            {
                ++stats[26];
                return b->pushNegation(stats, context, nested, negated);
            }
            else if(auto cond = dynamic_cast<NotCondition*>(a.get()))
            {
                if((*cond)[0] == DeadlockCondition::DEADLOCK)
                {
                    ++stats[27];
                    return EFCondition(b).pushNegation(stats, context, nested, negated);
                }
            }
            
            if(dynamic_cast<EFCondition*>(b.get()))
            {
                ++stats[28];
                if(negated) return std::make_shared<NotCondition>(b);
                return b;
            }
            else if(auto cond = dynamic_cast<OrCondition*>(b.get()))
            {
                std::vector<Condition_ptr> pef, npef;
                for(auto& i : *cond)
                {
                    if(dynamic_cast<EFCondition*>(i.get()))
                    {
                        pef.push_back(i);
                    }
                    else
                    {
                        npef.push_back(i);
                    }
                }
                if(pef.size() > 0)
                {
                    stats[29] += pef.size();
                    if(npef.size() != 0)
                    {
                        pef.push_back(std::make_shared<EUCondition>(_cond1, makeOr(npef)));
                        ++stats[28];
                        --stats[29];
                    }
                    return makeOr(pef)->pushNegation(stats, context, nested, negated);
                }
            }
            /*auto pushag = pushAg<EFCondition, EUCondition>(a, b, negated);
            if(pushag != nullptr) return pushag;*/
            auto c = std::make_shared<EUCondition>(a, b);
            if(negated) return std::make_shared<NotCondition>(c);
            return c;
            }, stats, context, nested, negated);
        }

        
        Condition_ptr pushAnd(const std::vector<Condition_ptr>& _conds, negstat_t& stats, const EvaluationContext& context, bool nested, bool negate_children)
        {
            std::vector<Condition_ptr> nef, other;            
            for(auto& c : _conds)
            {
                auto n = c->pushNegation(stats, context, nested, negate_children);
                if(n->isTriviallyFalse()) return n;
                if(n->isTriviallyTrue()) continue;
                if(auto neg = dynamic_cast<NotCondition*>(n.get()))
                {
                    if(auto ef = dynamic_cast<EFCondition*>((*neg)[0].get()))
                    {
                        nef.push_back((*ef)[0]);
                    }
                    else
                    {
                        other.emplace_back(n);
                    }
                }
                else
                {
                    other.emplace_back(n);
                }
            }         
            if(nef.size() + other.size() == 0) return BooleanCondition::TRUE_CONSTANT;
            if(nef.size() + other.size() == 1) 
            { 
                return nef.size() == 0 ? 
                    other[0] : 
                    std::make_shared<NotCondition>(std::make_shared<EFCondition>(nef[0]));
            }
            if(nef.size() != 0) other.push_back(
                    std::make_shared<NotCondition>(
                    std::make_shared<EFCondition>(
                    makeOr(nef)))); 
            if(other.size() == 1) return other[0];
            auto res = makeAnd(other);
            return res;
        }
        
        Condition_ptr pushOr(const std::vector<Condition_ptr>& _conds, negstat_t& stats, const EvaluationContext& context, bool nested, bool negate_children)
        {
            std::vector<Condition_ptr> nef, other;       
            for(auto& c : _conds)
            {
                auto n = c->pushNegation(stats, context, nested, negate_children);
                if(n->isTriviallyTrue()) return n;
                if(n->isTriviallyFalse()) continue;
                if(auto ef = dynamic_cast<EFCondition*>(n.get()))
                {
                    nef.push_back((*ef)[0]);
                }
                else
                {
                    other.emplace_back(n);
                }
            }
            if(nef.size() + other.size() == 0) return BooleanCondition::FALSE_CONSTANT;
            if(nef.size() + other.size() == 1) { return nef.size() == 0 ? other[0] : std::make_shared<EFCondition>(nef[0]);}
            if(nef.size() != 0) other.push_back(
                    std::make_shared<EFCondition>(
                    makeOr(nef))); 
            if(other.size() == 1) return other[0];
            return makeOr(other);
        }

        Condition_ptr OrCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            return negated ? pushAnd(_conds, stats, context, nested, true) :
                             pushOr (_conds, stats, context, nested, false);
            }, stats, context, nested, negated);
        }

        
        Condition_ptr AndCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            return negated ? pushOr (_conds, stats, context, nested, true) :
                             pushAnd(_conds, stats, context, nested, false);

            }, stats, context, nested, negated);
        }
        
        Condition_ptr CompareConjunction::pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated) const {
            return std::make_shared<CompareConjunction>(*this, negated);
        }

        
        Condition_ptr NotCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            if(negated) ++stats[30];
            return _cond->pushNegation(stats, context, nested, !negated);
            }, stats, context, nested, negated);
        }
        
/*        template<int32_t C, typename Q, typename CM>
        Condition_ptr tryDistribute(const CompareCondition& cmp, int id1 = 0, int id2 = 1)
        {
            auto lit = dynamic_pointer_cast<LiteralExpr>(cmp[id1]);
            auto plus = dynamic_pointer_cast<PlusExpr>(cmp[id2]);
            if(!lit || !plus) return nullptr;
            if(lit->value() != C) return nullptr;
            std::vector<Condition_ptr> cnd;
            for(auto& e : *plus) cnd.emplace_back(std::make_shared<CM>(lit, e));
            if(std::is_same<Q, AndCondition>::value) return makeAnd(cnd);
            if(std::is_same<Q, OrCondition>::value) return makeOr(cnd);
            return std::make_shared<Q>(cnd);            
        }
*/ 
        
        bool CompareCondition::isTrivial() const
        {
            if(auto p1 = dynamic_pointer_cast<PlusExpr>(_expr1))
                if(auto p2 = dynamic_pointer_cast<PlusExpr>(_expr2))            
                    return p1->_exprs.size() + p1->_ids.size() + p2->_exprs.size() + p2->_ids.size() == 0;

            if(auto p1 = dynamic_pointer_cast<PlusExpr>(_expr1))
                if(auto p2 = dynamic_pointer_cast<PlusExpr>(_expr2))
                    return p1->_exprs.size() + p1->_ids.size() + p2->_exprs.size() + p2->_ids.size() == 0;
            return false;
        }        
        
        Condition_ptr LessThanCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            if(isTrivial()) return BooleanCondition::getShared(evaluate(context) xor negated);                
//            if(auto r = tryDistribute<0,OrCondition, LessThanCondition>(*this)) return r->pushNegation(stats, context, nested, negated);
            if(negated) return std::make_shared<GreaterThanOrEqualCondition>(_expr1, _expr2);
            else        return std::make_shared<LessThanCondition>(_expr1, _expr2);
            }, stats, context, nested, negated);
        }

        
        Condition_ptr GreaterThanOrEqualCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            if(isTrivial()) return BooleanCondition::getShared(evaluate(context) xor negated);                
//            if(auto r = tryDistribute<0,AndCondition, GreaterThanOrEqualCondition>(*this)) return r->pushNegation(stats, context, nested, negated);
//            if(auto r = tryDistribute<1,AndCondition, GreaterThanOrEqualCondition>(*this)) return r->pushNegation(stats, context, nested, negated);
            if(negated) return std::make_shared<LessThanCondition>(_expr1, _expr2);
            else        return std::make_shared<GreaterThanOrEqualCondition>(_expr1, _expr2);
            }, stats, context, nested, negated);
        }

        
        Condition_ptr LessThanOrEqualCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            if(isTrivial()) return BooleanCondition::getShared(evaluate(context) xor negated);                
//            if(auto r = tryDistribute<0,OrCondition, LessThanOrEqualCondition>(*this)) return r->pushNegation(stats, context, nested, negated);
//            if(auto r = tryDistribute<1,OrCondition, LessThanOrEqualCondition>(*this)) return r->pushNegation(stats, context, nested, negated);
            if(negated) return std::make_shared<GreaterThanCondition>(_expr1, _expr2);
            else        return std::make_shared<LessThanOrEqualCondition>(_expr1, _expr2);
            }, stats, context, nested, negated);
        }

        
        Condition_ptr GreaterThanCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            if(isTrivial()) return BooleanCondition::getShared(evaluate(context) xor negated);
//            if(auto r = tryDistribute<0,AndCondition, GreaterThanCondition>(*this)) return r->pushNegation(stats, context, nested, negated);
//            if(auto r = tryDistribute<1,AndCondition, GreaterThanCondition>(*this)) return r->pushNegation(stats, context, nested, negated);
//            if(auto r = tryDistribute<2,AndCondition, GreaterThanCondition>(*this)) return r->pushNegation(stats, context, nested, negated);
            if(negated) return std::make_shared<LessThanOrEqualCondition>(_expr1, _expr2);
            else        return std::make_shared<GreaterThanCondition>(_expr1, _expr2);
            }, stats, context, nested, negated);
        }
                
        Condition_ptr NotEqualCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            if(isTrivial()) return BooleanCondition::getShared(evaluate(context) xor negated);
            for(int i = 0; i < 2; ++i)
            {
                if(auto e = dynamic_pointer_cast<LiteralExpr>(operator [](i)))
                    if(e->value() == 0) 
                        return GreaterThanCondition(
                                operator []((i+1)%2), 
                                operator [](i)
                                    ).pushNegation(stats, context, nested, negated);
            }
            if(negated) return std::make_shared<EqualCondition>(_expr1, _expr2);
            else        return std::make_shared<NotEqualCondition>(_expr1, _expr2);
            }, stats, context, nested, negated);
        }

        
        Condition_ptr EqualCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            if(isTrivial()) return BooleanCondition::getShared(evaluate(context) xor negated);
            for(int i = 0; i < 2; ++i)
            {
                if(auto e = dynamic_pointer_cast<LiteralExpr>(operator [](i)))
                    if(e->value() == 0) 
                        return LessThanOrEqualCondition(
                                operator []((i+1)%2), 
                                operator [](i)
                                    ).pushNegation(stats, context, nested, negated);
            }
            if(negated) return std::make_shared<NotEqualCondition>(_expr1, _expr2);
            else        return std::make_shared<EqualCondition>(_expr1, _expr2);
            }, stats, context, nested, negated);
        }
                
        Condition_ptr BooleanCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            if(negated) return getShared(!_value);
            else        return getShared( _value);
            }, stats, context, nested, negated);
        }
        
        Condition_ptr DeadlockCondition::pushNegation(negstat_t& stats, const EvaluationContext& context, bool nested, bool negated) const {
            return initialMarkingRW([&]() -> Condition_ptr {
            if(negated) return std::make_shared<NotCondition>(DEADLOCK);
            else        return DEADLOCK;
            }, stats, context, nested, negated);
        }

        
        /******************** Stubborn reduction interesting transitions ********************/
        
        void PlusExpr::incr(ReducingSuccessorGenerator& generator) const { 
            for(auto& i : _ids) generator.presetOf(i.first);
            for(auto& e : _exprs) e->incr(generator);
        }
        
        void PlusExpr::decr(ReducingSuccessorGenerator& generator) const {
            for(auto& i : _ids) generator.postsetOf(i.first);
            for(auto& e : _exprs) e->decr(generator);
        }
        
        void SubtractExpr::incr(ReducingSuccessorGenerator& generator) const {
            _exprs[0]->incr(generator);
            _exprs[1]->decr(generator);
        }
        
        void SubtractExpr::decr(ReducingSuccessorGenerator& generator) const {
            _exprs[0]->decr(generator);
            _exprs[1]->incr(generator);
        }
        
        void MultiplyExpr::incr(ReducingSuccessorGenerator& generator) const {
            for(auto& i : _ids)
            {
                generator.presetOf(i.first);
                generator.postsetOf(i.first);
            }
            for(auto& e : _exprs)
            {
                e->incr(generator);
                e->decr(generator);
            }
        }
        
        void MultiplyExpr::decr(ReducingSuccessorGenerator& generator) const {
            incr(generator);
        }
        
        void MinusExpr::incr(ReducingSuccessorGenerator& generator) const {
            // TODO not implemented
        }
        
        void MinusExpr::decr(ReducingSuccessorGenerator& generator) const {
            // TODO not implemented
        }

        void LiteralExpr::incr(ReducingSuccessorGenerator& generator) const {
            // Add nothing
        }
        
        void LiteralExpr::decr(ReducingSuccessorGenerator& generator) const {
            // Add nothing
        }

        void IdentifierExpr::incr(ReducingSuccessorGenerator& generator) const {
            generator.presetOf(_offsetInMarking);
        }
        
        void IdentifierExpr::decr(ReducingSuccessorGenerator& generator) const {
             generator.postsetOf(_offsetInMarking);
        }
        
        void SimpleQuantifierCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const{
            // Not implemented
        }
        
        void UntilCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const{
            // Not implemented
        }
        
        void AndCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {
            if(!negated){               // and
                for(auto& c : _conds)
                {
                    if(!c->isSatisfied())
                    {
                        c->findInteresting(generator, negated);
                        break;
                    }
                }
            } else {                    // or
                for(auto& c : _conds) c->findInteresting(generator, negated);
            }
        }
        
        void OrCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {
            if(!negated){               // or
                for(auto& c : _conds) c->findInteresting(generator, negated);
            } else {                    // and
                for(auto& c : _conds)
                {
                    if(c->isSatisfied())
                    {
                        c->findInteresting(generator, negated);
                        break;
                    }
                }
            }
        }
        
        void CompareConjunction::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const{
             
            auto neg = negated != _negated;
            
            for(auto& c : _constraints)
            {
                auto val = generator.parent()[c._place];
                if(c._lower == c._upper)
                {
                    if(neg)
                    {
                        if(val != c._lower) continue;
                        generator.postsetOf(c._place);
                        generator.presetOf(c._place);
                    }
                    else
                    {
                        if(val == c._lower) continue;
                        if(val > c._lower) {
                            generator.postsetOf(c._place);
                        } else {
                            generator.presetOf(c._place);
                        }   
                    }
                }
                else
                {
                    if(!neg)
                    {
                        if(val < c._lower && c._lower != 0)
                        {
                            assert(!neg);
                            generator.presetOf(c._place);
                        }
                        
                        if(val > c._upper && c._upper != std::numeric_limits<uint32_t>::max())
                        {
                            assert(!neg);
                            generator.postsetOf(c._place);
                        }
                    }
                    else
                    {
                        if(val >= c._lower && c._lower != 0)
                        {
                            generator.postsetOf(c._place);
                        }
                        
                        if(val <= c._upper && c._upper != std::numeric_limits<uint32_t>::max())
                        {
                            generator.presetOf(c._place);
                        }
                    }
                }
            }
        }
        
        void EqualCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {
            if(!negated){               // equal
                if(_expr1->getEval() == _expr2->getEval()) { return; }
                if(_expr1->getEval() > _expr2->getEval()){
                    _expr1->decr(generator);
                    _expr2->incr(generator);
                } else {
                    _expr1->incr(generator);
                    _expr2->decr(generator);
                }   
            } else {                    // not equal
                if(_expr1->getEval() != _expr2->getEval()) { return; }
                _expr1->incr(generator);
                _expr2->decr(generator);
                _expr1->incr(generator);
                _expr2->decr(generator);
            }
        }
        
        void NotEqualCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {
            if(!negated){               // not equal
                if(_expr1->getEval() != _expr2->getEval()) { return; }
                _expr1->incr(generator);
                _expr2->decr(generator);
                _expr1->incr(generator);
                _expr2->decr(generator);
            } else {                    // equal
                if(_expr1->getEval() == _expr2->getEval()) { return; }
                if(_expr1->getEval() > _expr2->getEval()){
                    _expr1->decr(generator);
                    _expr2->incr(generator);
                } else {
                    _expr1->incr(generator);
                    _expr2->decr(generator);
                }   
            }
        }
        
        void LessThanCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {                
            if(!negated){               // less than
                if(_expr1->getEval() < _expr2->getEval()) { return; }
                _expr1->decr(generator);
                _expr2->incr(generator);
            } else {                    // greater than or equal
                if(_expr1->getEval() >= _expr2->getEval()) { return; }
                _expr1->incr(generator);
                _expr2->decr(generator);
            }
        }
        
        void LessThanOrEqualCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {
            if(!negated){               // less than or equal
                if(_expr1->getEval() <= _expr2->getEval()) { return; }
                _expr1->decr(generator);
                _expr2->incr(generator);
            } else {                    // greater than
                if(_expr1->getEval() > _expr2->getEval()) { return; }
                _expr1->incr(generator);
                _expr2->decr(generator);
            }
        }
        
        void GreaterThanCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {
            if(!negated){               // greater than
                if(_expr1->getEval() > _expr2->getEval()) { return; }
                _expr1->incr(generator);
                _expr2->decr(generator);
            } else {                    // less than or equal
                if(_expr1->getEval() <= _expr2->getEval()) { return; }
                _expr1->decr(generator);
                _expr2->incr(generator);
            }
        }
        
        void GreaterThanOrEqualCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {
            if(!negated){               // greater than or equal
                if(_expr1->getEval() >= _expr2->getEval()) { return; }
                _expr1->incr(generator);
                _expr2->decr(generator); 
            } else {                    // less than
                if(_expr1->getEval() < _expr2->getEval()) { return; }
                _expr1->decr(generator);
                _expr2->incr(generator);
            }
        }
        
        void NotCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {
            _cond->findInteresting(generator, !negated);
        }
        
        void BooleanCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {
            // Add nothing
        }
        
        void DeadlockCondition::findInteresting(ReducingSuccessorGenerator& generator, bool negated) const {
            if(!isSatisfied()){
                generator.postPresetOf(generator.leastDependentEnabled());
            } // else add nothing
        }
        
        
/********************** CONSTRUCTORS *********************************/

        template<typename T>
        void postMerge(std::vector<Condition_ptr>& conds) {
            std::sort(std::begin(conds), std::end(conds),
                    [](auto& a, auto& b) {
                        return a->isTemporal() == b->isTemporal() ?
                                    a->formulaSize() < b->formulaSize() : 
                                    a->isTemporal() < b->isTemporal(); 
                    });
        } 
        
        AndCondition::AndCondition(std::vector<Condition_ptr>&& conds) {
            for (auto& c : conds) tryMerge<AndCondition>(_conds, c);
            for (auto& c : _conds) _temporal = _temporal || c->isTemporal();
            postMerge<AndCondition>(_conds);
        }
        
        AndCondition::AndCondition(const std::vector<Condition_ptr>& conds) {
            for (auto& c : conds) tryMerge<AndCondition>(_conds, c);
            for (auto& c : conds) _temporal = _temporal || c->isTemporal();
            postMerge<AndCondition>(_conds);
        }
       
        AndCondition::AndCondition(Condition_ptr left, Condition_ptr right) {
            tryMerge<AndCondition>(_conds, left);
            tryMerge<AndCondition>(_conds, right);
            for (auto& c : _conds) _temporal = _temporal || c->isTemporal();
            postMerge<AndCondition>(_conds);
        }
       
        OrCondition::OrCondition(std::vector<Condition_ptr>&& conds) {
            for (auto& c : conds) tryMerge<OrCondition>(_conds, c);
            for (auto& c : conds) _temporal = _temporal || c->isTemporal();
            postMerge<AndCondition>(_conds);
        }
       
        OrCondition::OrCondition(const std::vector<Condition_ptr>& conds) {
            for (auto& c : conds) tryMerge<OrCondition>(_conds, c);
            for (auto& c : conds) _temporal = _temporal || c->isTemporal();
            postMerge<AndCondition>(_conds);
        }
       
        OrCondition::OrCondition(Condition_ptr left, Condition_ptr right) {
            tryMerge<OrCondition>(_conds, left);
            tryMerge<OrCondition>(_conds, right);
            for (auto& c : _conds) _temporal = _temporal || c->isTemporal();
            postMerge<AndCondition>(_conds);
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
                std::cout << "MERGE OF CONJUNCT AND DISJUNCT NOT ALLOWED" << std::endl;
                assert(false);
            }
            auto il = _constraints.begin();
            for(auto& c : other._constraints)
            {
                il = std::lower_bound(_constraints.begin(), _constraints.end(), c);
                cons_t c2;
                if(neg)
                {
                    if(c._upper == 0)
                    {
                        c2._lower = 1;
                    }
                    else if (c._upper == std::numeric_limits<uint32_t>::max())
                    {
                        c2._upper = c._lower + (other._negated ? -1 :  1);
                    }
                    else if(c._lower == 0)
                    {
                        c2._lower = c._upper + (other._negated ?  1 : -1);
                    }
                    else
                    {
                        std::cout << "MERGE OF CONJUNCT AND DISJUNCT NOT ALLOWED" << std::endl;
                        assert(false);
                    }
                    c2._place = c._place;
                    assert(c2._lower <= c2._upper);
                }
                else
                {
                    c2 = c;
                }
                            
                if(il == _constraints.end() || il->_place != c2._place)
                {
                    c2._name = c._name;
                    il = _constraints.insert(il, c2);
                }
                else
                {
                    il->_lower = std::max(il->_lower, c2._lower);
                    il->_upper = std::min(il->_upper, c2._upper);
                }
            }
        }
        
        void CompareConjunction::merge(const std::vector<Condition_ptr>& conditions, bool negated)
        {

            for(auto& c : conditions)
            {
                auto cmp = dynamic_cast<CompareCondition*>(c.get());
                assert(cmp);
                auto id = dynamic_cast<IdentifierExpr*>((*cmp)[0].get());
                LiteralExpr* lit = nullptr;
                bool inverted = false;
                if(!id)
                {
                    id = dynamic_cast<IdentifierExpr*>((*cmp)[1].get());
                    lit = dynamic_cast<LiteralExpr*>((*cmp)[0].get());
                    inverted = true;
                }
                else
                {
                    lit = dynamic_cast<LiteralExpr*>((*cmp)[1].get());                
                }
                assert(lit);
                assert(id);
                uint32_t val = lit->value();
                cons_t next;
                next._place = id->offset();
                auto lb = std::lower_bound(std::begin(_constraints), std::end(_constraints), next);
                if(lb == std::end(_constraints))
                {
                    next._name = id->name();
                    lb = _constraints.insert(lb, next);
                }  
                else
                {
                    assert(id->name().compare(lb->_name) == 0);
                }

                if(dynamic_cast<LessThanOrEqualCondition*>(c.get()))
                {
                    if(!negated)
                        if(inverted) lb->_lower = std::max(lb->_lower, val);
                        else         lb->_upper = std::min(lb->_upper, val);
                    else
                        if(!inverted) lb->_lower = std::max(lb->_lower, val+1);
                        else          lb->_upper = std::min(lb->_upper, val-1);                        
                }
                else if(dynamic_cast<GreaterThanOrEqualCondition*>(c.get()))
                {
                    if(!negated)
                        if(!inverted) lb->_lower = std::max(lb->_lower, val);
                        else          lb->_upper = std::min(lb->_upper, val);
                    else
                        if(inverted) lb->_lower = std::max(lb->_lower, val+1);
                        else         lb->_upper = std::min(lb->_upper, val-1);                        
                }
                else if(dynamic_cast<LessThanCondition*>(c.get()))
                {
                    if(!negated)
                        if(inverted) lb->_lower = std::max(lb->_lower, val+1);
                        else         lb->_upper = std::min(lb->_upper, val-1);
                    else
                        if(!inverted) lb->_lower = std::max(lb->_lower, val);
                        else          lb->_upper = std::min(lb->_upper, val);
                        
                }
                else if(dynamic_cast<GreaterThanCondition*>(c.get()))
                {
                    if(!negated)
                        if(!inverted) lb->_lower = std::max(lb->_lower, val+1);
                        else          lb->_upper = std::min(lb->_upper, val-1);
                    else
                        if(inverted) lb->_lower = std::max(lb->_lower, val);
                        else         lb->_upper = std::min(lb->_upper, val);                       
                }
                else if(dynamic_cast<EqualCondition*>(c.get()))
                {
                    assert(!negated);
                    lb->_lower = std::max(lb->_lower, val);
                    lb->_upper = std::min(lb->_upper, val);
                }
                else if(dynamic_cast<NotEqualCondition*>(c.get()))
                {
                    assert(negated);
                    lb->_lower = std::max(lb->_lower, val);
                    lb->_upper = std::min(lb->_upper, val);
                }
                else
                {
                    std::cout << "UNKNOWN " << std::endl;
                    assert(false);
                }
            }            
        }
       
        CommutativeExpr::CommutativeExpr(std::vector<Expr_ptr>&& exprs, int initial)
        : NaryExpr(), _constant(initial) {
            for (auto& e : exprs) {
                if (auto lit = dynamic_pointer_cast<PQL::LiteralExpr>(e))
                    _constant = this->apply(_constant, lit->value());
                else if (auto id = dynamic_pointer_cast<PQL::IdentifierExpr>(e)) {
                    _ids.emplace_back(id->offset(), id->name());
                } else {
                    _exprs.emplace_back(std::move(e));
                }
            }
        }

    } // PQL
} // PetriEngine

