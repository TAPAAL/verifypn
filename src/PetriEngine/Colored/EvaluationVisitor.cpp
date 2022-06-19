
#include "PetriEngine/Colored/EvaluationVisitor.h"
#include "PetriEngine/Colored/Expressions.h"

#include <sstream>

namespace PetriEngine {
    namespace Colored {

        void EvaluationVisitor::accept(const DotConstantExpression*) {
            _cres = &(*ColorType::dotInstance()->begin());
        }

        void EvaluationVisitor::accept(const VariableExpression* e) {
            _cres = _context.binding.find(e->variable())->second;
        }

        void EvaluationVisitor::accept(const UserOperatorExpression* e) {
            if(_context.placePartition.getEquivalenceClasses().empty()){
                _cres = e->user_operator();
            } else {
                std::vector<uint32_t> tupleIds;
                e->user_operator()->getTupleId(tupleIds);

                _context.placePartition.applyPartition(tupleIds);
                _cres = e->user_operator()->getColorType()->getColor(tupleIds);
            }
        }

        void EvaluationVisitor::accept(const SuccessorExpression* e) {
            e->child()->visit(*this);
            _cres = &++(*_cres);
        }

        void EvaluationVisitor::accept(const PredecessorExpression* e) {
            e->child()->visit(*this);
            _cres = &--(*_cres);
        }

        void EvaluationVisitor::accept(const TupleExpression* tup) {
            std::vector<const Color*> colors;
            colors.reserve(tup->size());
            std::vector<const ColorType*> types;
            types.reserve(tup->size());
            for (const auto& color : *tup) {
                color->visit(*this);
                colors.push_back(_cres);
                types.push_back(colors.back()->getColorType());
            }
            const ProductType* pt = _context.findProductColorType(types);
            assert(pt != nullptr);
            const Color* col = pt->getColor(colors);
            assert(col != nullptr);
            _cres = col;
        }

        // these comparisons work because we know the colors are allocated
        // consequtively in memory in correct order.
        void EvaluationVisitor::accept(const LessThanExpression*  e) {
            (*e)[0]->visit(*this);
            auto lhs = _cres;
            (*e)[1]->visit(*this);
            if (lhs->isTuple() || _cres->isTuple()) throw base_error("Tuple-tuple comparison are not allowed: Unknown semantics");
            _bres = lhs < _cres;
        }

        void EvaluationVisitor::accept(const LessThanEqExpression* e) {
            (*e)[0]->visit(*this);
            auto lhs = _cres;
            (*e)[1]->visit(*this);
            if (lhs->isTuple() || _cres->isTuple()) throw base_error("Tuple-tuple comparison are not allowed: Unknown semantics");
            _bres = lhs <= _cres;
        }

        void EvaluationVisitor::accept(const EqualityExpression* e) {
            (*e)[0]->visit(*this);
            auto lhs = _cres;
            (*e)[1]->visit(*this);
            if (lhs->isTuple() || _cres->isTuple()) throw base_error("Tuple-tuple comparison are not allowed: Unknown semantics");
            _bres = lhs == _cres;
        }

        void EvaluationVisitor::accept(const InequalityExpression* e) {
            (*e)[0]->visit(*this);
            auto lhs = _cres;
            (*e)[1]->visit(*this);
            if (lhs->isTuple() || _cres->isTuple()) throw base_error("Tuple-tuple comparison are not allowed: Unknown semantics");
            _bres = lhs != _cres;
        }

        void EvaluationVisitor::accept(const AndExpression* e) {
            (*e)[0]->visit(*this);
            auto lhs = _bres;
            (*e)[1]->visit(*this);
            _bres = lhs && _bres;
        }

        void EvaluationVisitor::accept(const OrExpression* e) {
            (*e)[0]->visit(*this);
            auto lhs = _bres;
            (*e)[1]->visit(*this);
            _bres = lhs || _bres;
        }

        void EvaluationVisitor::accept(const AllExpression* all) {
            throw base_error("AllExpression not to be visited by EvaluationVisitor");
        }

        void EvaluationVisitor::accept(const NumberOfExpression* no) {
            if (no->size() != 0) {
                std::vector<std::pair<const Color*,uint32_t>> col;
                col.reserve(no->size());
                for (const auto& elem : *no) {
                    elem->visit(*this);
                    col.push_back(std::make_pair(_cres, no->number()));
                }
                _mres = Multiset(col);
            } else if (no->is_all()) {
                std::vector<std::pair<const Color*,uint32_t>> colors;

                if(_context.placePartition.getEquivalenceClasses().empty() ||
                   _context.placePartition.isDiagonal()){
                    for (size_t i = 0; i < no->all()->sort()->size(); ++i) {
                        colors.push_back(std::make_pair(&(*no->all()->sort())[i], no->number()));
                    }
                } else {
                    for (const auto& eq_class : _context.placePartition.getEquivalenceClasses()){
                        colors.push_back(
                        std::make_pair(no->all()->sort()->getColor(eq_class.intervals().getLowerIds()),
                            eq_class.size()*no->number()));
                    }
                }
                _mres = Multiset(colors);
            }
        }

        void EvaluationVisitor::accept(const AddExpression* add) {
            Multiset ms;
            for (const auto& expr : *add) {
                expr->visit(*this);
                ms += _mres;
            }
            _mres = ms;
        }

        void EvaluationVisitor::accept(const SubtractExpression* sub) {
            (*sub)[0]->visit(*this);
            auto lhs = _mres;
            (*sub)[1]->visit(*this);
            if (!_mres.isSubsetOf(lhs))
                throw base_error("RHS of subtraction is not a subset of LHS");
            _mres = lhs - _mres;
        }

        void EvaluationVisitor::accept(const ScalarProductExpression* scalar) {
            scalar->child()->visit(*this);
            _mres = _mres * scalar->scalar();
        }

        Multiset EvaluationVisitor::evaluate(const ArcExpression& e, const ExpressionContext& context)
        {
            EvaluationVisitor v(context);
            e.visit(v);
            return v._mres;
        }

        bool EvaluationVisitor::evaluate(const GuardExpression& e, const ExpressionContext& context)
        {
            EvaluationVisitor v(context);
            e.visit(v);
            return v._bres;
        }
    }
}