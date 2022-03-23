/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/ArcVarMultisetVisitor.h"

namespace PetriEngine::Colored {
    void ArcVarMultisetVisitor::accept(const VariableExpression * e) {
        _mres = VarMultiset(e->variable(), 1);
    }

    void ArcVarMultisetVisitor::accept(const TupleExpression * e) {
        // TODO Fuck
    }

    void ArcVarMultisetVisitor::accept(const NumberOfExpression * e) {
        // TODO
        _mres *= e->number();
    }

    void ArcVarMultisetVisitor::accept(const AddExpression * e) {
        VarMultiset ms;
        for (const auto& expr : *e) {
            expr->visit(*this);
            ms += _mres;
        }
        _mres = ms;
    }

    void ArcVarMultisetVisitor::accept(const ScalarProductExpression * e) {
        // TODO
    }

    VarMultiset ArcVarMultisetVisitor::extract(const ArcExpression &e) {
        ArcVarMultisetVisitor v;
        e.visit(v);
        return v._mres;
    }
}
