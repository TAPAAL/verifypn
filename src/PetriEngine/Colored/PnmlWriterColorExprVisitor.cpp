//
// Created by mathi on 16/03/2022.
//

#include "PetriEngine/Colored/PnmlWriterColorExprVisitor.h"
#include "PetriEngine/Colored/Expressions.h"

namespace PetriEngine::Colored {
    void writeExpressionToPnml(std::ostream &out, uint32_t tabs, const Expression &expr) {
        PnmlWriterColorExprVisitor visitor(out, tabs);
        expr.visit(visitor);
    }

    void PnmlWriterColorExprVisitor::accept(const DotConstantExpression *) {
        _out << increaseTabs() << "<dotconstant/>\n";
    }

    void PnmlWriterColorExprVisitor::accept(const VariableExpression *e) {
        _out << increaseTabs() << "<variable refvariable=\"" << e->variable()->name << "\"/>\n";
    }

    void PnmlWriterColorExprVisitor::accept(const UserOperatorExpression *e) {
        _out << increaseTabs() << "<useroperator declaration=\"" << e->user_operator()->getColorName()
             << "\"/>\n";
    }

    void PnmlWriterColorExprVisitor::accept(const SuccessorExpression *e) {
        _out << increaseTabs() << "<successor>" << "\n";
        _out << increaseTabs() << "<subterm>" << "\n";
        e->child()->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << decreaseTabs() << "</successor>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const PredecessorExpression *e) {
        _out << increaseTabs() << "<predecessor>" << "\n";
        _out << increaseTabs() << "<subterm>" << "\n";
        e->child()->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << decreaseTabs() << "</predecessor>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const TupleExpression *tup) {
        _out << increaseTabs() << "<tuple>\n";
        bool first = true;
        for (auto &e: *tup) {
            if (first) {
                _out << increaseTabs() << "<subterm>\n";
            } else {
                _out << getTabs() << "<subterm>\n";
            }
            first = false;

            e->visit(*this);
            _out << decreaseTabs() << "</subterm>\n";
        }

        _out << decreaseTabs() << "</tuple>\n";
    }

    void PnmlWriterColorExprVisitor::accept(const LessThanExpression *lt) {
        _out << increaseTabs() << "<lessthan>" << "\n";
        _out << increaseTabs() << "<subterm>" << "\n";
        (*lt)[0]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << getTabs() << "<subterm>" << "\n";
        (*lt)[1]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << decreaseTabs() << "</lessthan>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const LessThanEqExpression *lte) {
        _out << increaseTabs() << "<lessthanorequal>" << "\n";
        _out << increaseTabs() << "<subterm>" << "\n";
        (*lte)[0]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << getTabs() << "<subterm>" << "\n";
        (*lte)[1]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << decreaseTabs() << "</lessthanorequal>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const EqualityExpression *eq) {
        _out << increaseTabs() << "<equality>" << "\n";
        _out << increaseTabs() << "<subterm>" << "\n";
        (*eq)[0]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << getTabs() << "<subterm>" << "\n";
        (*eq)[1]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << decreaseTabs() << "</equality>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const InequalityExpression *neq) {
        _out << increaseTabs() << "<inequality>" << "\n";
        _out << increaseTabs() << "<subterm>" << "\n";
        (*neq)[0]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << getTabs() << "<subterm>" << "\n";
        (*neq)[1]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << decreaseTabs() << "</inequality>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const AndExpression *andexpr) {
        _out << increaseTabs() << "<and>" << "\n";
        _out << increaseTabs() << "<subterm>" << "\n";
        (*andexpr)[0]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << getTabs() << "<subterm>" << "\n";
        (*andexpr)[1]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << decreaseTabs() << "</and>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const OrExpression *orexpr) {
        _out << increaseTabs() << "<or>" << "\n";
        _out << increaseTabs() << "<subterm>" << "\n";
        (*orexpr)[0]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << getTabs() << "<subterm>" << "\n";
        (*orexpr)[1]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << decreaseTabs() << "</or>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const AllExpression *all) {
        _out << increaseTabs() << "<all>" << "\n";
        _out << increaseTabs() << "<usersort declaration=\"" << all->sort()->getName() << "\"/>\n";
        _out << decreaseTabs() << "</all>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const NumberOfExpression *no) {
        _out << increaseTabs() << "<numberof>" << "\n";
        if (no->is_all()) {
            _out << increaseTabs() << "<subterm>" << "\n";
            no->all()->visit(*this);
            _out << decreaseTabs() << "</subterm>" << "\n";
        } else {
            _out << increaseTabs() << "<subterm>" << "\n";
            _out << increaseTabs() << "<numberconstant value=\"" << no->number() << "\">" << "\n";
            //todo, check if always positive. No "negative" in mcc2021 looks like.
            _out << increaseTabs() << "<positive/>\n";
            _out << decreaseTabs() << "</numberconstant>" << "\n";
            _out << decreaseTabs() << "</subterm>" << "\n";

            for (auto &c: *no) {
                _out << getTabs() << "<subterm>" << "\n";
                c->visit(*this);
                _out << decreaseTabs() << "</subterm>" << "\n";
            }
        }
        _out << decreaseTabs() << "</numberof>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const AddExpression *add) {
        if (add->size() > 1) {
            _out << increaseTabs() << "<add>" << "\n";
        }
        for (auto &c: *add) {
            _out << increaseTabs() << "<subterm>" << "\n";
            c->visit(*this);
            _out << decreaseTabs() << "</subterm>" << "\n";
        }
        if (add->size() > 1) {
            _out << decreaseTabs() << "</add>" << "\n";
        }
    }

    void PnmlWriterColorExprVisitor::accept(const SubtractExpression *sub) {
        _out << increaseTabs() << "<subtract>" << "\n";
        _out << increaseTabs() << "<subterm>" << "\n";
        (*sub)[0]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << getTabs() << "<subterm>" << "\n";
        (*sub)[1]->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << decreaseTabs() << "</subtract>" << "\n";
    }

    void PnmlWriterColorExprVisitor::accept(const ScalarProductExpression *scalar) {
        _out << increaseTabs() << "<scalarproduct>" << "\n";
        _out << increaseTabs() << "<subterm>" << "\n";
        _out << increaseTabs() << "<numberconstant value=\"" << scalar->scalar() << "\"/>\n";
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << getTabs() << "<subterm>" << "\n";
        scalar->child()->visit(*this);
        _out << decreaseTabs() << "</subterm>" << "\n";
        _out << decreaseTabs() << "</scalarproduct>" << "\n";
    }
}