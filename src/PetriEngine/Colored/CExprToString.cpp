
#include "PetriEngine/Colored/CExprToString.h"
#include "PetriEngine/Colored/Expressions.h"

#include <sstream>

namespace PetriEngine {
    namespace Colored {

        std::ostream& operator<<(std::ostream& os, const PetriEngine::Colored::Expression& x) {
            CExprToString v(os);
            x.visit(v);
            return os;
        }

        std::string to_string(const PetriEngine::Colored::Expression& x) {
            std::stringstream ss;
            ss << x;
            return ss.str();
        }

        void CExprToString::accept(const DotConstantExpression*) {
            _out << "dot";
        }

        void CExprToString::accept(const VariableExpression* e) {
            _out << e->variable()->name;
        }

        void CExprToString::accept(const UserOperatorExpression* e) {
            _out << e->user_operator()->toString();
        }

        void CExprToString::accept(const SuccessorExpression* e) {
            e->child()->visit(*this);
            _out << "++";
        }

        void CExprToString::accept(const PredecessorExpression* e) {
            e->child()->visit(*this);
            _out << "--";
        }

        void CExprToString::accept(const TupleExpression* tup) {
            if(tup->size() > 1) _out << "(";
            bool first = true;
            for (auto& e : *tup) {
                if (!first) _out << ",";
                first = false;
                e->visit(*this);
            }
            if(tup->size() > 1) _out << ")";
        }

        void CExprToString::accept(const LessThanExpression* lt) {
            (*lt)[0]->visit(*this);
            _out << " < ";
            (*lt)[1]->visit(*this);
        }

        void CExprToString::accept(const LessThanEqExpression* lte) {
            (*lte)[0]->visit(*this);
            _out << " <= ";
            (*lte)[1]->visit(*this);
        }

        void CExprToString::accept(const EqualityExpression* eq) {
            (*eq)[0]->visit(*this);
            _out << " == ";
            (*eq)[1]->visit(*this);
        }

        void CExprToString::accept(const InequalityExpression* neq) {
            (*neq)[0]->visit(*this);
            _out << " != ";
            (*neq)[1]->visit(*this);
        }

        void CExprToString::accept(const AndExpression* andexpr) {
            _out << "(";
            (*andexpr)[0]->visit(*this);
            _out << " && ";
            (*andexpr)[1]->visit(*this);
            _out << ")";
        }

        void CExprToString::accept(const OrExpression* orexpr) {
            _out << "(";
            (*orexpr)[0]->visit(*this);
            _out << " || ";
            (*orexpr)[1]->visit(*this);
            _out << ")";
        }

        void CExprToString::accept(const AllExpression* all) {
            _out << all->sort()->getName() + ".all";
        }

        void CExprToString::accept(const NumberOfExpression* no) {
            if (no->isAll()) {
                _out << no->number() << "'(";
                no->all()->visit(*this);
                _out << ")";

            } else {
                _out << no->number() << "'(";
                bool first = true;
                for (auto& c : *no) {
                    if (!first) _out << " + ";
                    first = false;
                    c->visit(*this);
                }
                _out << ")";
            }
        }

        void CExprToString::accept(const AddExpression* add) {
            bool first = true;
            _out << "(";
            for (auto& e : *add) {
                if (!first) _out << " + ";
                first = false;
                e->visit(*this);
            }
            _out << ")";
        }

        void CExprToString::accept(const SubtractExpression* sub) {
            _out << "(";
            (*sub)[0]->visit(*this);
            _out << " - ";
            (*sub)[1]->visit(*this);
            _out << ")";
        }

        void CExprToString::accept(const ScalarProductExpression* scalar) {
            _out << "(";
            _out << scalar->scalar();
            _out << " * ";
            scalar->child()->visit(*this);
            _out << ")";
        }
    }
}