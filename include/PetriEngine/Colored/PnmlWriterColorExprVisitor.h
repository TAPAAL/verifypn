//
// Created by mathi on 16/03/2022.
//

#ifndef VERIFYPN_PNMLWRITERCOLOREXPRVISITOR_H
#define VERIFYPN_PNMLWRITERCOLOREXPRVISITOR_H


#include <ostream>
#include "utils/errors.h"
#include "ColorExpressionVisitor.h"

namespace PetriEngine::Colored {
    void writeExpressionToPnml(std::ostream& out, uint32_t tabs, const Expression &expr);

    class PnmlWriterColorExprVisitor: public ColorExpressionVisitor {

    public:
        PnmlWriterColorExprVisitor(std::ostream& out, uint32_t tabs) : _out(out), _tabs(tabs) {}

        virtual void accept(const DotConstantExpression*);

        virtual void accept(const VariableExpression* e);

        virtual void accept(const UserOperatorExpression* e);

        virtual void accept(const SuccessorExpression* e);

        virtual void accept(const PredecessorExpression* e);

        virtual void accept(const TupleExpression* tup);

        virtual void accept(const LessThanExpression* lt);

        virtual void accept(const LessThanEqExpression* lte);

        virtual void accept(const EqualityExpression* eq);

        virtual void accept(const InequalityExpression* neq);

        virtual void accept(const AndExpression* andexpr);

        virtual void accept(const OrExpression* orexpr);

        virtual void accept(const AllExpression* all);

        virtual void accept(const NumberOfExpression* no);

        virtual void accept(const AddExpression* add);

        virtual void accept(const SubtractExpression* sub);

        virtual void accept(const ScalarProductExpression* scalar);

    private:
        std::ostream& _out;
        std::uint32_t _tabs;

        std::string getTabs() {
            std::string tabsString;
            for (uint32_t i=0; i<_tabs;i++) {
                tabsString += '\t';
            }
            return tabsString;
        }

        std::string increaseTabs() {
            _tabs += 1;
            return getTabs();
        }

        std::string decreaseTabs() {
            if (_tabs == 0) {
                throw base_error("Underflow in number of tabs when writing colored PNML");
            }
            _tabs -= 1;
            return getTabs();
        }
    };
}


#endif //VERIFYPN_PNMLWRITERCOLOREXPRVISITOR_H
