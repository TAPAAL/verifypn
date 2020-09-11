/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/PQL/Expressions.h"

namespace PetriEngine {
    namespace PQL {

        Expr::~Expr()= default;
        
        bool Condition::isTriviallyTrue() {
            if (trivial == 1) {
                return true;
            }
            
            return false;
        }
        
        bool Condition::isTriviallyFalse() {
            if (trivial == 2) {
                return true;
            }
            
            return false;
        }
        
        Condition::~Condition() = default;

        Condition_ptr Condition::initialMarkingRW(const std::function<Condition_ptr()>& func, negstat_t& stats, const EvaluationContext& context, bool nested, bool negated, bool initrw)
        {
            auto res = func();
            if(!nested && initrw)
            {
                auto e = res->evaluate(context);
                if(e != Condition::RUNKNOWN) 
                {
                    if(res->getQuantifier() == E && res->getPath() == F)
                    {
                        auto ef = static_cast<EFCondition*>(res.get());
                        if(dynamic_cast<UnfoldedUpperBoundsCondition*>((*ef)[0].get()))
                        {
                            return res;
                        }
                    }
                    return BooleanCondition::getShared(e);
                }
            }
            return res;            
        }


    } // PQL
} // PetriEngine
