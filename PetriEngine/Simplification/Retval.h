#ifndef RETVAL_H
#define RETVAL_H
#include "LinearPrograms.h"

namespace PetriEngine {
    namespace Simplification {
        class Retval {
        public:
            std::shared_ptr<PQL::Condition> formula;
            std::shared_ptr<LinearPrograms> lps;      
            
            Retval (std::shared_ptr<PQL::Condition> formula, const LinearPrograms& lps1) : formula(formula) { 
                lps = std::make_shared<LinearPrograms>(lps1);
            }
            Retval (std::shared_ptr<PQL::Condition> formula) : Retval(formula, LinearPrograms(LinearProgram())) {
            }
            Retval() {
                lps = std::make_shared<LinearPrograms>(LinearPrograms(LinearProgram()));
            }
            ~Retval(){
            }
            bool isSet(){
                return (formula.operator bool() && lps.get()->lps.size() != 0);
            }
        private:

        };
    }
}

#endif /* RETVAL_H */

