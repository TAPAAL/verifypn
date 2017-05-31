#ifndef RETVAL_H
#define RETVAL_H
#include "LinearPrograms.h"

namespace PetriEngine {
    namespace Simplification {
        struct Retval {
            std::shared_ptr<PQL::Condition> formula = nullptr;
            LinearPrograms lps;      
            
            Retval (std::shared_ptr<PQL::Condition> formula, LinearPrograms&& lps1) 
            : formula(formula), lps(std::move(lps1)) {
            }
                        
            Retval (std::shared_ptr<PQL::Condition> formula) 
            : formula(formula) {
                lps.addEmpty();
            }
 
            Retval(Retval&& other) 
            : formula(other.formula), lps(std::move(other.lps))
            {}

            Retval& operator=(Retval&& other) {
                lps = std::move(other.lps);
                formula = std::move(other.formula);
                return *this;
            }
            
            Retval() {
                lps.addEmpty();
            }
            
            ~Retval(){
            }
            
            bool isSet(){
                return (formula != nullptr && lps.size() != 0);
            }
        private:

        };
    }
}

#endif /* RETVAL_H */

