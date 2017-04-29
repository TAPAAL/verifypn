#ifndef RETVAL_H
#define RETVAL_H
#include "LinearPrograms.h"

namespace PetriEngine {
    namespace Simplification {
        class Retval {
        public:
            std::shared_ptr<PQL::Condition> formula = nullptr;
            LinearPrograms lps;      
            
            Retval (std::shared_ptr<PQL::Condition> formula, const LinearPrograms&& lps1) 
            : lps(lps1) {
                this->formula.swap(formula);
            }
            
            Retval (std::shared_ptr<PQL::Condition> formula) 
            : Retval(formula, LinearPrograms(LinearProgram())) {
            }
            
            Retval() {
                lps = LinearPrograms(LinearProgram());
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

