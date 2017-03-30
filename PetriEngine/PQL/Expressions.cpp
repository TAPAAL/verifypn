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

using namespace PetriEngine::Simplification;

namespace PetriEngine {
    namespace PQL {
        // CONSTANTS
        Condition_ptr BooleanCondition::FALSE = std::make_shared<BooleanCondition>(false);
        Condition_ptr BooleanCondition::TRUE = std::make_shared<BooleanCondition>(true);
        
        
        /******************** To String ********************/

        std::string LiteralExpr::toString() const {
            std::stringstream stream;
            stream << _value;
            return stream.str();
        }

        std::string IdentifierExpr::toString() const {
            return _name;
        }

        std::string BinaryExpr::toString() const {
            return "(" + _expr1->toString() + " " + op() + " " + _expr2->toString() + ")";
        }

        std::string MinusExpr::toString() const {
            return "-" + _expr->toString();
        }

        std::string QuantifierCondition::toString() const {
            return op() + " "  + _cond->toString();
        }
        
        std::string UntilCondition::toString() const {
            return op() + " (" + _cond1->toString() + " U " + _cond2->toString() + ")";
        }
        
        std::string LogicalCondition::toString() const {
            return "(" + _cond1->toString() + " " + op() + " " + _cond2->toString() + ")";
        }

        std::string CompareCondition::toString() const {
            return "(" + _expr1->toString() + " " + op() + " " + _expr2->toString() + ")";
        }

        std::string NotCondition::toString() const {
            return "(not " + _cond->toString() + ")";
        }

        std::string BooleanCondition::toString() const {
            if (_value)
                return "true";
            return "false";
        }

        std::string DeadlockCondition::toString() const {
            return "deadlock";
        }

        /******************** To TAPAAL Query ********************/

        std::string QuantifierCondition::toTAPAALQuery(TAPAALConditionExportContext& context) const {
            return op() + " "  + _cond->toTAPAALQuery(context);
        }
        
        std::string UntilCondition::toTAPAALQuery(TAPAALConditionExportContext& context) const {
            return op() + " (" + _cond1->toTAPAALQuery(context) + " U " + _cond2->toTAPAALQuery(context) + ")";
        }
        
        std::string LogicalCondition::toTAPAALQuery(TAPAALConditionExportContext& context) const {
            return " ( " + _cond1->toTAPAALQuery(context) + " " + op() + " " + _cond2->toTAPAALQuery(context) + " ) ";
        }

        std::string CompareCondition::toTAPAALQuery(TAPAALConditionExportContext& context) const {
            //If <id> <op> <literal>
            if (_expr1->type() == Expr::IdentifierExpr && _expr2->type() == Expr::LiteralExpr) {
                return " ( " + context.netName + "." + _expr1->toString() + " " + opTAPAAL() + " " + _expr2->toString() + " ) ";
                //If <literal> <op> <id>
            } else if (_expr2->type() == Expr::IdentifierExpr && _expr1->type() == Expr::LiteralExpr) {
                return " ( " + _expr1->toString() + " " + sopTAPAAL() + " " + context.netName + "." + _expr2->toString() + " ) ";
            } else {
                context.failed = true;
                return " false ";
            }
        }

        std::string NotEqualCondition::toTAPAALQuery(TAPAALConditionExportContext& context) const {
            return " !( " + CompareCondition::toTAPAALQuery(context) + " ) ";
        }

        std::string NotCondition::toTAPAALQuery(TAPAALConditionExportContext& context) const {
            return " !( " + _cond->toTAPAALQuery(context) + " ) ";
        }

        std::string BooleanCondition::toTAPAALQuery(TAPAALConditionExportContext&) const {
            if (_value)
                return "true";
            return "false";
        }

        std::string DeadlockCondition::toTAPAALQuery(TAPAALConditionExportContext&) const {
            return "deadlock";
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

        void BinaryExpr::analyze(AnalysisContext& context) {
            _expr1->analyze(context);
            _expr2->analyze(context);
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

        void QuantifierCondition::analyze(AnalysisContext& context) {
            _cond->analyze(context);
        }
        
        void UntilCondition::analyze(AnalysisContext& context) {
            _cond1->analyze(context);
            _cond2->analyze(context);
        }
        
        void LogicalCondition::analyze(AnalysisContext& context) {
            _cond1->analyze(context);
            _cond2->analyze(context);
        }

        void CompareCondition::analyze(AnalysisContext& context) {
            _expr1->analyze(context);
            _expr2->analyze(context);
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

        int BinaryExpr::evaluate(const EvaluationContext& context) const {
            int v1 = _expr1->evaluate(context);
            int v2 = _expr2->evaluate(context);
            return apply(v1, v2);
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

        bool QuantifierCondition::evaluate(const EvaluationContext& context) const {
            // Not implemented
        }
        
        bool UntilCondition::evaluate(const EvaluationContext& context) const {
            // Not implemented
        }
        
        bool LogicalCondition::evaluate(const EvaluationContext& context) const {
            bool b1 = _cond1->evaluate(context);
            bool b2 = _cond2->evaluate(context);
            return apply(b1, b2);
        }

        bool CompareCondition::evaluate(const EvaluationContext& context) const {
            int v1 = _expr1->evaluate(context);
            int v2 = _expr2->evaluate(context);
            return apply(v1, v2);
        }

        bool NotCondition::evaluate(const EvaluationContext& context) const {
            return !(_cond->evaluate(context));
        }

        bool BooleanCondition::evaluate(const EvaluationContext&) const {
            return _value;
        }

        bool DeadlockCondition::evaluate(const EvaluationContext& context) const {
            if (!context.net())
                return false;
            if (!context.net()->deadlocked(context.marking())) {
                return false;
            }
            return true;
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

        /******************** Apply (LogicalCondition subclasses) ********************/

        bool AndCondition::apply(bool b1, bool b2) const {
            return b1 && b2;
        }

        bool OrCondition::apply(bool b1, bool b2) const {
            return b1 || b2;
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

        /******************** p-free Expression ********************/

        bool BinaryExpr::pfree() const {
            return _expr1->pfree() && _expr2->pfree();
        }

        bool MinusExpr::pfree() const {
            return _expr->pfree();
        }

        bool LiteralExpr::pfree() const {
            return true;
        }

        bool IdentifierExpr::pfree() const {
            return false;
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
        
        uint32_t QuantifierCondition::distance(DistanceContext& context) const {
            // Not implemented
        }
        
        uint32_t UntilCondition::distance(DistanceContext& context) const {
            // Not implemented
        }
        
        uint32_t LogicalCondition::distance(DistanceContext& context) const {
            uint32_t d1 = _cond1->distance(context);
            uint32_t d2 = _cond2->distance(context);
            return delta(d1, d2, context);
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

        uint32_t CompareCondition::distance(DistanceContext& context) const {
            int v1 = _expr1->evaluate(context);
            int v2 = _expr2->evaluate(context);
            return delta(v1, v2, context.negated());
        }

        uint32_t EqualCondition::delta(int v1, int v2, bool negated) const {
            if (!negated)
                return v1 - v2;
            else
                return v1 == v2 ? 1 : 0;
        }

        uint32_t NotEqualCondition::delta(int v1, int v2, bool negated) const {
            if (negated)
                return v1 - v2;
            else
                return v1 == v2 ? 1 : 0;
        }

        uint32_t LessThanCondition::delta(int v1, int v2, bool negated) const {
            if (!negated)
                return v1 < v2 ? 0 : v1 - v2 + 1;
            else
                return v1 >= v2 ? 0 : v2 - v1;
        }

        uint32_t LessThanOrEqualCondition::delta(int v1, int v2, bool negated) const {
            if (!negated)
                return v1 <= v2 ? 0 : v1 - v2;
            else
                return v1 > v2 ? 0 : v2 - v1 + 1;
        }

        uint32_t GreaterThanCondition::delta(int v1, int v2, bool negated) const {
            if (!negated)
                return v1 > v2 ? 0 : v2 - v1 + 1;
            else
                return v1 <= v2 ? 0 : v1 - v2;
        }

        uint32_t GreaterThanOrEqualCondition::delta(int v1, int v2, bool negated) const {
            if (!negated)
                return v1 >= v2 ? 0 : v2 - v1;
            else
                return v1 < v2 ? 0 : v1 - v2 + 1;
        }

        /******************** Query Simplification ********************/       
        
        Member LiteralExpr::constraint(SimplificationContext context) const {
            return Member(_value);
        }
        
        Member IdentifierExpr::constraint(SimplificationContext context) const {
            // Reserve index 0 to LPsolve
            std::vector<double> row(context.net()->numberOfTransitions() + 1);
            uint32_t p = offset();
            for (size_t t = 0; t < context.net()->numberOfTransitions(); t++) {
                row[1 + t] = context.net()->outArc(t, p) - context.net()->inArc(p, t);
            }
            return Member(row, context.marking()[p]);
        }
        
        Member PlusExpr::constraint(SimplificationContext context) const {
            return _expr1->constraint(context) + _expr2->constraint(context);
        }
        
        Member SubtractExpr::constraint(SimplificationContext context) const {
            return _expr1->constraint(context) - _expr2->constraint(context);
        }
        
        Member MultiplyExpr::constraint(SimplificationContext context) const {
            return _expr1->constraint(context) * _expr2->constraint(context);
        }
        
        Member MinusExpr::constraint(SimplificationContext context) const {
            return -_expr->constraint(context);
        }
        
        Retval QuantifierCondition::simplify(SimplificationContext context) const {
            // Not implemented
        }
        
        Retval EXCondition::simplify(SimplificationContext context) const {
            // Not implemented
        }
        
        Retval AXCondition::simplify(SimplificationContext context) const {
            // Not implemented
        }
        
        Retval UntilCondition::simplify(SimplificationContext context) const {
            // Not implemented
        }
        
        Retval simplifyAnd(SimplificationContext context, Retval r1, Retval r2) {
            if(r1.formula->toString() == "false" || r2.formula->toString() == "false") {
                return Retval(std::make_shared<BooleanCondition>(false));
            } else if (r1.formula->toString() == "true") {
                return Retval(r2.formula, r2.lps);
            } else if (r2.formula->toString() == "true") {
                return Retval(r1.formula, r1.lps);
            }
            
            LinearPrograms merged = LinearPrograms::lpsMerge(r1.lps, r2.lps);
            
            if(!context.timeout() && !merged.satisfiable(context.net(), context.marking(), context.getLpTimeout())) {
                return Retval(std::make_shared<BooleanCondition>(false));
            } else {
                return Retval(std::make_shared<AndCondition>(r1.formula, r2.formula), merged); 
            }
        }
        
        Retval simplifyOr(Retval r1, Retval r2) {
            if(r1.formula->toString() == "true" || r2.formula->toString() == "true") {
                return Retval(std::make_shared<BooleanCondition>(true));
            } else if (r1.formula->toString() == "false") {
                return Retval(r2.formula, r2.lps);
            } else if (r2.formula->toString() == "false") {
                return Retval(r1.formula, r1.lps);
            } else {
                return Retval(std::make_shared<OrCondition>(r1.formula, r2.formula), 
                        LinearPrograms::lpsUnion(r1.lps, r2.lps));
            }
        }
        
        Retval AndCondition::simplify(SimplificationContext context) const {
            Retval r1 = _cond1->simplify(context);
            Retval r2 = _cond2->simplify(context);
            
            return context.negated() ? simplifyOr(r1, r2) : simplifyAnd(context, r1, r2);
        }
        
        Retval OrCondition::simplify(SimplificationContext context) const {
            Retval r1 = _cond1->simplify(context);
            Retval r2 = _cond2->simplify(context);
            
            return context.negated() ? simplifyAnd(context, r1, r2) : simplifyOr(r1, r2);
        }
        
        Retval EqualCondition::simplify(SimplificationContext context) const {
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            
            LinearPrograms lps;
            if (!context.timeout() && m1.canAnalyze && m2.canAnalyze) {
                if ((m1.isConstant() && m2.isConstant()) || m1 == m2) {
                    return Retval(std::make_shared<BooleanCondition>(
                        context.negated() ? (m1.constant != m2.constant) : (m1.constant == m2.constant)));
                } else {
                    lps.add(LinearProgram(Equation(m1, m2, (context.negated() ? "!=" : "=="))));
                }
            } else {
                lps.add(LinearProgram());
            }
            
            if (!context.timeout() && !lps.satisfiable(context.net(), context.marking(), context.getLpTimeout())) {
                return Retval(std::make_shared<BooleanCondition>(false));
            } else {
                if (context.negated()) {
                    return Retval(std::make_shared<NotEqualCondition>(_expr1, _expr2), lps);
                } else {
                    return Retval(std::make_shared<EqualCondition>(_expr1, _expr2), lps);
                }                         
            }
        }
        
        Retval NotEqualCondition::simplify(SimplificationContext context) const {
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            
            LinearPrograms lps;
            if (!context.timeout() && m1.canAnalyze && m2.canAnalyze) {
                if ((m1.isConstant() && m2.isConstant()) || m1 != m2) {
                    return Retval(std::make_shared<BooleanCondition>(
                        context.negated() ? (m1.constant == m2.constant) : (m1.constant != m2.constant)));
                } else{ 
                    lps.add(LinearProgram(Equation(m1, m2, (context.negated() ? "==" : "!="))));
                }
            } else {
                lps.add(LinearProgram());
            }
            
            if (!context.timeout() && !lps.satisfiable(context.net(), context.marking(), context.getLpTimeout())) {
                return Retval(std::make_shared<BooleanCondition>(false));
            } else {
                if (context.negated()) {
                    return Retval(std::make_shared<EqualCondition>(_expr1, _expr2), lps);
                } else {
                    return Retval(std::make_shared<NotEqualCondition>(_expr1, _expr2), lps);
                }                         
            }
        }
        
        Retval LessThanCondition::simplify(SimplificationContext context) const {
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            
            LinearPrograms lps;
            if (!context.timeout() && m1.canAnalyze && m2.canAnalyze) {
                // test for trivial comparison
                Trivial eval = context.negated() ? m1 >= m2 : m1 < m2;
                if(eval != Trivial::Indeterminate)
                    return Retval(std::make_shared<BooleanCondition>(eval == Trivial::True));
                
                // if no trivial case
                else lps.add(LinearProgram(Equation(m1, m2, (context.negated() ? ">=" : "<"))));
            } else {
                lps.add(LinearProgram());
            }
            
            if (!context.timeout() && !lps.satisfiable(context.net(), context.marking(), context.getLpTimeout())) {
                return Retval(std::make_shared<BooleanCondition>(false));
            } else {
                if (context.negated()) {
                    return Retval(std::make_shared<GreaterThanOrEqualCondition>(_expr1, _expr2), lps);
                } else {
                    return Retval(std::make_shared<LessThanCondition>(_expr1, _expr2), lps);
                }                         
            }
        }        
        
        Retval LessThanOrEqualCondition::simplify(SimplificationContext context) const {
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            
            LinearPrograms lps;
            if (!context.timeout() && m1.canAnalyze && m2.canAnalyze) {
                // test for trivial comparison
                Trivial eval = context.negated() ? m1 > m2 : m1 <= m2;
                if(eval != Trivial::Indeterminate)
                    return Retval(std::make_shared<BooleanCondition>(eval == Trivial::True));
                
                // if no trivial case
                else lps.add(LinearProgram(Equation(m1, m2, (context.negated() ? ">" : "<="))));
            } else {
                lps.add(LinearProgram());
            }
                                    
            if (!context.timeout() && !lps.satisfiable(context.net(), context.marking(), context.getLpTimeout())) {
                return Retval(std::make_shared<BooleanCondition>(false));
            } else {
                if (context.negated()) {
                    return Retval(std::make_shared<GreaterThanCondition>(_expr1, _expr2), lps);
                } else {
                    return Retval(std::make_shared<LessThanOrEqualCondition>(_expr1, _expr2), lps);
                }                         
            }
        }
        
        Retval GreaterThanCondition::simplify(SimplificationContext context) const {
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            
            LinearPrograms lps;
            if (!context.timeout() && m1.canAnalyze && m2.canAnalyze) {
                // test for trivial comparison
                Trivial eval = context.negated() ? m1 <= m2 : m1 > m2;
                if(eval != Trivial::Indeterminate)
                    return Retval(std::make_shared<BooleanCondition>(eval == Trivial::True));
                
                // if no trivial case
                else lps.add(LinearProgram(Equation(m1, m2, (context.negated() ? "<=" : ">")))); 
            } else {
                lps.add(LinearProgram());
            }
            
            if(!context.timeout() && !lps.satisfiable(context.net(), context.marking(), context.getLpTimeout())) {
                return Retval(std::make_shared<BooleanCondition>(false));
            } else {
                if (context.negated()) {
                    return Retval(std::make_shared<LessThanOrEqualCondition>(_expr1, _expr2), lps);
                } else {
                    return Retval(std::make_shared<GreaterThanCondition>(_expr1, _expr2), lps);
                }                         
            }
        }
        
        Retval GreaterThanOrEqualCondition::simplify(SimplificationContext context) const {  
            Member m1 = _expr1->constraint(context);
            Member m2 = _expr2->constraint(context);
            
            LinearPrograms lps;
            if (!context.timeout() && m1.canAnalyze && m2.canAnalyze) {
                // test for trivial comparison
                Trivial eval = context.negated() ? m1 < m2 : m1 >= m2;
                if(eval != Trivial::Indeterminate)
                    return Retval(std::make_shared<BooleanCondition>(eval == Trivial::True));
                
                // if no trivial case
                else lps.add(LinearProgram(Equation(m1, m2, (context.negated() ? "<" : ">="))));
                
            } else {
                lps.add(LinearProgram());
            }
            
            if (!context.timeout() && !lps.satisfiable(context.net(), context.marking(), context.getLpTimeout())) {
                return Retval(std::make_shared<BooleanCondition>(false));
            } else {
                if (context.negated()) {
                    return Retval(std::make_shared<LessThanCondition>(_expr1, _expr2), lps);
                } else {
                    return Retval(std::make_shared<GreaterThanOrEqualCondition>(_expr1, _expr2), lps);
                }                         
            }
        }
        
        Retval NotCondition::simplify(SimplificationContext context) const {
            context.negate();
            return _cond->simplify(context);
            context.negate();
        }
        
        Retval BooleanCondition::simplify(SimplificationContext context) const {
            if (context.negated()) {
                return Retval(std::make_shared<BooleanCondition>(!_value));
            } else {
                return Retval(std::make_shared<BooleanCondition>(_value));
            }
        }
        
        Retval DeadlockCondition::simplify(SimplificationContext context) const {
            if (context.negated()) {
                return Retval(std::make_shared<NotCondition>(std::make_shared<DeadlockCondition>()));
            } else {
                return Retval(std::make_shared<DeadlockCondition>());
            }
        }
        
        /******************** Just-In-Time Compilation ********************/
    } // PQL
} // PetriEngine

