#ifndef LINEARPROGRAM_H
#define LINEARPROGRAM_H
#include "../PetriNet.h"
#include "Member.h"
#include "Equation.h"
#include <algorithm>
    
namespace PetriEngine {
    namespace Simplification {
        
        class LinearProgram {
        private:
            enum result_t { UKNOWN, IMPOSSIBLE, POSSIBLE };
            result_t _result = result_t::UKNOWN;
            std::vector<Equation_ptr> equations;

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
                
                for(auto eq : lp1.equations) res.equations.push_back(eq);
                for(auto eq : lp2.equations) res.equations.push_back(eq);
                
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

