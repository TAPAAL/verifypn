#ifndef LINEARPROGRAM_H
#define LINEARPROGRAM_H
#include <algorithm>
#include <unordered_set>

#include "../PetriNet.h"
#include "Member.h"
#include "Equation.h"
    
namespace PetriEngine {
    namespace Simplification {
        
        class LinearProgram {
        private:
            enum result_t { UKNOWN, IMPOSSIBLE, POSSIBLE };
            result_t _result = result_t::UKNOWN;
            std::unordered_set<EquationWrap> equations;

        public:
            LinearProgram();
            virtual ~LinearProgram();
            LinearProgram(Equation&& eq);
            size_t size() const
            {
                return equations.size();
            }
            
            bool isImpossible(const PetriEngine::PetriNet* net, const PetriEngine::MarkVal* m0, uint32_t timeout);
            void swap(LinearProgram& other)
            {
                std::swap(_result, other._result);
                std::swap(equations, other.equations);
            }
            
            static LinearProgram lpUnion(LinearProgram& lp1, LinearProgram& lp2){
                LinearProgram res;
                res.equations.reserve(lp1.equations.size() + lp2.equations.size());
                res.equations.insert(lp1.equations.begin(), lp1.equations.end());
                res.equations.insert(lp2.equations.begin(), lp2.equations.end());
                
                if( lp1._result == result_t::IMPOSSIBLE ||
                    lp2._result == result_t::IMPOSSIBLE)
                {
                    // impossibility is compositional -- linear constraint-systems are big conjunctions.
                    res._result = result_t::IMPOSSIBLE;
                }
                return res;
            }            
            
        };
    }
}

#endif /* LINEARPROGRAM_H */

