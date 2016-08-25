/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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

namespace PetriEngine {
    namespace PQL {

        /******************** Destructors ********************/

        BinaryExpr::~BinaryExpr() {
            if (_expr1)
                delete _expr1;
            _expr1 = NULL;
            if (_expr2)
                delete _expr2;
            _expr2 = NULL;
        }

        MinusExpr::~MinusExpr() {
            if (_expr)
                delete _expr;
            _expr = NULL;
        }

        LogicalCondition::~LogicalCondition() {
            if (_cond1)
                delete _cond1;
            _cond1 = NULL;
            if (_cond2)
                delete _cond2;
            _cond2 = NULL;
        }

        CompareCondition::~CompareCondition() {
            if (_expr1)
                delete _expr1;
            _expr1 = NULL;
            if (_expr2)
                delete _expr2;
            _expr2 = NULL;
        }

        NotCondition::~NotCondition() {
            if (_cond)
                delete _cond;
            _cond = NULL;
        }

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
                        _srcOffset,
                        _name.length());
                context.reportError(error);
            }
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

        /******************** Constraint Analysis ********************/

        void LogicalCondition::findConstraints(ConstraintAnalysisContext& context) const {
            if (!context.canAnalyze)
                return;
            _cond1->findConstraints(context);
            ConstraintAnalysisContext::ConstraintSet left = context.retval;
            context.retval.clear();
            _cond2->findConstraints(context);
            mergeConstraints(context.retval, left, context.negated);
        }

        void AndCondition::mergeConstraints(ConstraintAnalysisContext::ConstraintSet& result,
                ConstraintAnalysisContext::ConstraintSet& other,
                bool negated) const {
            if (!negated)
                result = Structures::StateConstraints::mergeAnd(result, other);
            else
                result = Structures::StateConstraints::mergeOr(result, other);
        }

        void OrCondition::mergeConstraints(ConstraintAnalysisContext::ConstraintSet& result,
                ConstraintAnalysisContext::ConstraintSet& other,
                bool negated) const {
            if (!negated)
                result = Structures::StateConstraints::mergeOr(result, other);
            else
                result = Structures::StateConstraints::mergeAnd(result, other);
        }

        void CompareCondition::findConstraints(ConstraintAnalysisContext& context) const {
            if (!context.canAnalyze)
                return;
            context.retval.clear();
            if (_expr1->type() == Expr::LiteralExpr && _expr2->type() == Expr::IdentifierExpr) {
                IdentifierExpr* id = (IdentifierExpr*) _expr2;
                LiteralExpr* literal = (LiteralExpr*) _expr1;
                addConstraints(context, literal->value(), id);
            } else if (_expr1->type() == Expr::IdentifierExpr && _expr2->type() == Expr::LiteralExpr) {
                IdentifierExpr* id = (IdentifierExpr*) _expr1;
                LiteralExpr* literal = (LiteralExpr*) _expr2;
                addConstraints(context, id, literal->value());
            } else
                context.canAnalyze = false;
        }

        void NotCondition::findConstraints(ConstraintAnalysisContext& context) const {
            if (context.canAnalyze) {
                context.negated = !context.negated;
                this->_cond->findConstraints(context);
                context.negated = !context.negated;
            }
        }

        void BooleanCondition::findConstraints(ConstraintAnalysisContext& context) const {
            if (context.canAnalyze) {
                context.retval.clear();
                if (context.negated != _value) {
                    Structures::StateConstraints* s = new Structures::StateConstraints(context.net());
                    context.retval.push_back(s);
                }
            }
        }

        void DeadlockCondition::findConstraints(ConstraintAnalysisContext& context) const {
            //TODO: Come up with an over-approximation
            context.canAnalyze = false;
            context.retval.clear();
        }

        /******************** CompareCondition::addConstraints ********************/


        void EqualCondition::addConstraints(ConstraintAnalysisContext& context, IdentifierExpr* id, int value) const {
            if (!context.negated) {
                Structures::StateConstraints* s = new Structures::StateConstraints(context.net());
                s->setPlaceMin(id->offset(), value);
                s->setPlaceMax(id->offset(), value);
                assert(s);
                context.retval.push_back(s);
            } else {
                Structures::StateConstraints* s1 = new Structures::StateConstraints(context.net());
                Structures::StateConstraints* s2 = NULL;
                if (value != 0)
                    s2 = new Structures::StateConstraints(context.net());
                s1->setPlaceMin(id->offset(), value + 1);
                if (value != 0)
                    s2->setPlaceMax(id->offset(), value - 1);
                assert((s2 || value == 0) && s1);
                context.retval.push_back(s1);
                if (value != 0)
                    context.retval.push_back(s2);
            }
        }

        void EqualCondition::addConstraints(ConstraintAnalysisContext& context, int value, IdentifierExpr* id) const {
            addConstraints(context, id, value);
        }

        void NotEqualCondition::addConstraints(ConstraintAnalysisContext& context, IdentifierExpr* id, int value) const {
            if (context.negated) {
                Structures::StateConstraints* s = new Structures::StateConstraints(context.net());
                s->setPlaceMin(id->offset(), value);
                s->setPlaceMax(id->offset(), value);
                assert(s);
                context.retval.push_back(s);
            } else {
                Structures::StateConstraints* s1 = new Structures::StateConstraints(context.net());
                Structures::StateConstraints* s2 = NULL;
                if (value != 0)
                    s2 = new Structures::StateConstraints(context.net());
                s1->setPlaceMin(id->offset(), value + 1);
                if (value != 0)
                    s2->setPlaceMax(id->offset(), value - 1);
                assert((s2 || value == 0) && s1);
                context.retval.push_back(s1);
                if (value != 0)
                    context.retval.push_back(s2);
            }
        }

        void NotEqualCondition::addConstraints(ConstraintAnalysisContext& context, int value, IdentifierExpr* id) const {
            addConstraints(context, id, value);
        }

        void LessThanCondition::addConstraints(ConstraintAnalysisContext& context, IdentifierExpr* id, int value) const {
            Structures::StateConstraints* nc = new Structures::StateConstraints(context.net());
            if (!context.negated) {
                nc->setPlaceMax(id->offset(), value - 1);
            } else {
                nc->setPlaceMin(id->offset(), value);
            }
            context.retval.push_back(nc);
        }

        void LessThanCondition::addConstraints(ConstraintAnalysisContext& context, int value, IdentifierExpr* id) const {
            Structures::StateConstraints* nc = new Structures::StateConstraints(context.net());
            if (!context.negated) {
                nc->setPlaceMin(id->offset(), value + 1);
            } else {
                nc->setPlaceMax(id->offset(), value);
            }
            context.retval.push_back(nc);
        }

        void LessThanOrEqualCondition::addConstraints(ConstraintAnalysisContext& context, IdentifierExpr* id, int value) const {
            Structures::StateConstraints* nc = new Structures::StateConstraints(context.net());
            if (!context.negated) {
                nc->setPlaceMax(id->offset(), value);
            } else {
                nc->setPlaceMin(id->offset(), value + 1);
            }
            context.retval.push_back(nc);
        }

        void LessThanOrEqualCondition::addConstraints(ConstraintAnalysisContext& context, int value, IdentifierExpr* id) const {
            Structures::StateConstraints* nc = new Structures::StateConstraints(context.net());
            if (!context.negated) {
                nc->setPlaceMin(id->offset(), value);
            } else {
                nc->setPlaceMax(id->offset(), value - 1);
            }
            context.retval.push_back(nc);
        }

        void GreaterThanCondition::addConstraints(ConstraintAnalysisContext& context, IdentifierExpr* id, int value) const {
            Structures::StateConstraints* nc = new Structures::StateConstraints(context.net());
            if (!context.negated) {
                nc->setPlaceMin(id->offset(), value + 1);
            } else {
                nc->setPlaceMax(id->offset(), value);
            }
            context.retval.push_back(nc);
        }

        void GreaterThanCondition::addConstraints(ConstraintAnalysisContext& context, int value, IdentifierExpr* id) const {
            Structures::StateConstraints* nc = new Structures::StateConstraints(context.net());
            if (!context.negated) {
                nc->setPlaceMax(id->offset(), value - 1);
            } else {
                nc->setPlaceMin(id->offset(), value);
            }
            context.retval.push_back(nc);
        }

        void GreaterThanOrEqualCondition::addConstraints(ConstraintAnalysisContext& context, IdentifierExpr* id, int value) const {
            Structures::StateConstraints* nc = new Structures::StateConstraints(context.net());
            if (!context.negated) {
                nc->setPlaceMin(id->offset(), value);
            } else {
                nc->setPlaceMax(id->offset(), value - 1);
            }
            context.retval.push_back(nc);
        }

        void GreaterThanOrEqualCondition::addConstraints(ConstraintAnalysisContext& context, int value, IdentifierExpr* id) const {
            Structures::StateConstraints* nc = new Structures::StateConstraints(context.net());
            if (!context.negated) {                
                    nc->setPlaceMax(id->offset(), value);
            } else {
                    nc->setPlaceMin(id->offset(), value + 1);
            }
            context.retval.push_back(nc);
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


        /******************** Just-In-Time Compilation ********************/

    } // PQL
} // PetriEngine

