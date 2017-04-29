#ifndef RETVAL_H
#define RETVAL_H
#include "LinearPrograms.h"

namespace PetriEngine {
    namespace Simplification {
        class Retval {
        public:
            std::shared_ptr<PQL::Condition> formula = nullptr;
            LinearPrograms lps;      
            
            Retval (std::shared_ptr<PQL::Condition> formula, LinearPrograms&& lps1) 
            : lps(std::move(lps1)) {
                this->formula.swap(formula);
            }
                        
            Retval (std::shared_ptr<PQL::Condition> formula) 
            : Retval(formula, LinearPrograms(LinearProgram())) {
            }
 
            Retval(Retval&& other) : formula(formula), lps(std::move(other.lps))
            {}

            Retval& operator=(Retval&& other) {
                lps.lps.swap(other.lps.lps);
                return *this;
            }
            
            Retval() {
                lps.lps.emplace_back();
            }
            
            ~Retval(){
            }
            
            bool isSet(){
                return (formula != nullptr && lps.lps.size() != 0);
            }
        private:

        };
    }
}

#endif /* RETVAL_H */

