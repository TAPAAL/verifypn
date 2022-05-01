/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/ArcVarMultisetVisitor.h"

namespace PetriEngine::Colored {
    void ArcVarMultisetVisitor::accept(const VariableExpression *e) {
        if (_inTuple)
            _varRes = e->variable();
        else
            _tupRes = {e->variable()};
    }

    void ArcVarMultisetVisitor::accept(const TupleExpression *e) {
        assert(!_inTuple);
        _inTuple = true;
        std::vector<const Variable *> tuple;
        for (auto &v : *e) {
            v->visit(*this);
            if (!_ok) return;
            tuple.emplace_back(_varRes);
        }
        _tupRes = tuple;
        _inTuple = false;
    }

    void ArcVarMultisetVisitor::accept(const NumberOfExpression *e) {
        VarMultiset ms;
        for (auto &t : *e) {
            t->visit(*this);
            if (!_ok) return;
            ms += VarMultiset(_tupRes, e->number());
        }
        _msRes = ms;
    }

    void ArcVarMultisetVisitor::accept(const AddExpression *e) {
        VarMultiset ms;
        for (const auto &expr : *e) {
            expr->visit(*this);
            if (!_ok) return;
            ms += _msRes;
        }
        _msRes = ms;
    }

    void ArcVarMultisetVisitor::accept(const ScalarProductExpression *e) {
        e->child()->visit(*this);
        if (!_ok) return;
        _msRes *= e->scalar();
    }

    std::optional<VarMultiset> ArcVarMultisetVisitor::getResult() const {
        return _ok ? std::optional { _msRes } : std::nullopt;
    }

    std::optional<VarMultiset> extractVarMultiset(const ArcExpression &e) {
        ArcVarMultisetVisitor v;
        e.visit(v);
        return v.getResult();
    }
}
