#ifndef LINEARPROGRAM_H
#define LINEARPROGRAM_H
#include "../PetriNet.h"
#include "Member.h"
#include "Equation.h"
#include <algorithm>
    
namespace PetriEngine {
    namespace Structures {
        
        class LinearProgram {
        public:
            LinearProgram();
            virtual ~LinearProgram();
            LinearProgram(Equation eq);
            
            void addEquation(Equation eq);
            void addEquations(std::vector<Equation> eqs);
            bool isimpossible(const PetriEngine::PetriNet* net, const PetriEngine::MarkVal* m0);
            int op(std::string op);            
            
            static LinearProgram lpUnion(LinearProgram& lp1, LinearProgram& lp2){
                LinearProgram res;
                res.addEquations(lp1.equations);
                res.addEquations(lp2.equations);
                return res;
            }

            std::vector<Equation> equations;
        };
    }
}

#endif /* LINEARPROGRAM_H */

