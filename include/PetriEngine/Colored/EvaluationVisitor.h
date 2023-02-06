/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   EvaluationVisitor.h
 * Author: Peter G. Jensen
 *
 * Created on 10 February 2022, 20.45
 */

#ifndef EVALUATIONVISITOR_H
#define EVALUATIONVISITOR_H

#include <string>
#include <unordered_map>
#include <set>
#include <stdlib.h>
#include <iostream>
#include <cassert>
#include <memory>
#include <typeinfo>


#include "Colors.h"
#include "Multiset.h"
#include "EquivalenceVec.h"
#include "utils/errors.h"
#include "ColorExpressionVisitor.h"
#include "Expressions.h"

namespace PetriEngine {
    namespace Colored {

        struct ExpressionContext {
            const BindingMap& binding;
            const ColorTypeMap& colorTypes;
            const Colored::EquivalenceVec& placePartition;

            const Color* findColor(const std::string& color) const {
                for (auto& elem : colorTypes) {
                    auto col = (*elem.second)[color];
                    if (col)
                        return col;
                }
                throw base_error("Could not find color: ", color.c_str(), "\nCANNOT_COMPUTE\n");
            }

            const ProductType* findProductColorType(const std::vector<const ColorType*>& types) const {
                for (auto& elem : colorTypes) {
                    auto* pt = dynamic_cast<const ProductType*> (elem.second);

                    if (pt && pt->containsTypes(types)) {
                        return pt;
                    }
                }
                return nullptr;
            }
        };

        class EvaluationVisitor : public ColorExpressionVisitor {
        private:
            const ExpressionContext& _context;
            const Color* _cres;
            bool _bres;
            Multiset _mres;
        public:

            EvaluationVisitor(const ExpressionContext& context) : _context(context) {
            }

            static Multiset evaluate(const ArcExpression& e, const ExpressionContext& context);
            static bool evaluate(const GuardExpression& e, const ExpressionContext& context);

            const Color* result() const {
                return _cres;
            }

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

        };

    }
}


#endif /* EVALUATIONVISITOR_H */

