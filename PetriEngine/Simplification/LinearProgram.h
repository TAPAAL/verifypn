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
            std::vector<Equation> equations;

        public:
            LinearProgram();
            virtual ~LinearProgram();
            LinearProgram(Equation&& eq);
            
            void addEquations(std::vector<Equation>& eqs);
            bool isImpossible(const PetriEngine::PetriNet* net, const PetriEngine::MarkVal* m0, uint32_t timeout);
            int op(std::string op);    
            void swap(LinearProgram& other)
            {
                std::swap(_result, other._result);
                std::swap(equations, other.equations);
            }
            
            static LinearProgram lpUnion(LinearProgram& lp1, LinearProgram& lp2){
                LinearProgram res;
                res.addEquations(lp1.equations);
                res.addEquations(lp2.equations);
                return res;
            }            
            
        };
    }
}

#endif /* LINEARPROGRAM_H */

