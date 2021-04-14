/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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