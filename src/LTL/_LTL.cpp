/*
 * File:   _LTL.cpp
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 14/10/2020
 */

#include "LTL/LTL.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Expressions.h"
#include <utility>

using namespace PetriEngine::PQL;

namespace LTL {
/**
 * Converts a formula on the form A f, E f or f into just f, assuming f is an LTL formula.
 * In the case E f, not f is returned, and in this case the model checking result should be negated
 * (indicated by bool in return value)
 * @param formula - a formula on the form A f, E f or f
 * @return @code(ltl_formula, should_negate) - ltl_formula is the formula f if it is a valid LTL formula, nullptr otherwise.
 * should_negate indicates whether the returned formula is negated (in the case the parameter was E f)
 */
    std::pair<Condition_ptr, bool> to_ltl(const Condition_ptr &formula) {
        LTL::LTLValidator validator;
        bool should_negate = false;
        Condition_ptr converted;
        if (auto _formula = dynamic_cast<ECondition *>(formula.get())) {
            converted = std::make_shared<NotCondition>((*_formula)[0]);
            should_negate = true;
        } else if (auto _formula = dynamic_cast<ACondition *>(formula.get())) {
            converted = (*_formula)[0];
        } else {
            converted = formula;
        }
        converted->visit(validator);
        if (validator.bad()) {
            converted = nullptr;
        }
        return std::make_pair(converted, should_negate);
    }
}