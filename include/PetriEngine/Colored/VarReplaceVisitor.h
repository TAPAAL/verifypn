//
// Created by jespo on 06-05-2022.
//

#ifndef VERIFYPN_VARREPLACEVISITOR_H
#define VERIFYPN_VARREPLACEVISITOR_H

#endif //VERIFYPN_VARREPLACEVISITOR_H

#include "CloningVisitor.h"
#include "Expressions.h"

namespace PetriEngine::Colored {

    class VarReplaceVisitor : public CloningVisitor {
    private:
        std::unordered_map<std::string, const Variable*>& _varReplacementMap;
    public:
        VarReplaceVisitor(std::unordered_map<std::string ,const Variable*>& varReplacementMap)
        : _varReplacementMap(varReplacementMap) {}

        void accept(const VariableExpression* e) override;

        GuardExpression_ptr makeReplacementGuard(const GuardExpression_ptr& e){
            e->visit(*this);
            return _guard_res;
        }

        ArcExpression_ptr makeReplacementArcExpr(const ArcExpression_ptr& e){
            e->visit(*this);
            return _arc_res;
        }
    };
}