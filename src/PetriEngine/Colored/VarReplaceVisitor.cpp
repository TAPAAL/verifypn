//
// Created by jespo on 06-05-2022.
//

#include "PetriEngine/Colored/VarReplaceVisitor.h"
#include "PetriEngine/Colored/Expressions.h"

namespace PetriEngine::Colored {

        void VarReplaceVisitor::accept(const VariableExpression* e) {
            const Variable* _varInQuestion = _varReplacementMap[e->variable()->name];
            assert(_varInQuestion != nullptr);
            if (_varInQuestion != nullptr) {
                _col_res = std::make_shared<PetriEngine::Colored::VariableExpression>(_varInQuestion);
            } else {
                _col_res = std::make_shared<PetriEngine::Colored::VariableExpression>(new Variable{"VAR_REPLACE_ERROR", e->variable()->colorType});
            }
        }
    }