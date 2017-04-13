#ifndef RETVAL_H
#define RETVAL_H
#include "LinearPrograms.h"

namespace PetriEngine {
    namespace Simplification {
        class Retval {
        public:
            std::shared_ptr<PQL::Condition> formula;
            LinearPrograms lps;      
            
            Retval (std::shared_ptr<PQL::Condition> formula, LinearPrograms lps) : formula(formula), lps(lps) {           
            }
            Retval (std::shared_ptr<PQL::Condition> formula) : Retval(formula, LinearPrograms(LinearProgram())) {
            }
            Retval() : lps(LinearPrograms(LinearProgram())){
            }
            ~Retval(){
                
            }
            bool isSet(){
                return (formula.operator bool() && lps.lps.size() != 0);
            }
        private:

        };
    }
}

#endif /* RETVAL_H */

