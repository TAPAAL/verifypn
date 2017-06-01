#ifndef RETVAL_H
#define RETVAL_H
#include "LinearPrograms.h"

namespace PetriEngine {
    namespace Simplification {
        struct Retval {
            std::shared_ptr<PQL::Condition> formula = nullptr;
            LinearPrograms lps;
            LinearPrograms neglps;
            
            Retval (std::shared_ptr<PQL::Condition> formula, LinearPrograms&& lps1, LinearPrograms&& lps2) 
            : formula(formula), lps(std::move(lps1)), neglps(std::move(lps2)) {
            }
                        
            Retval (std::shared_ptr<PQL::Condition> formula) 
            : formula(formula) {
                lps.addEmpty();
                neglps.addEmpty();
            }
 
            Retval(Retval&& other) 
            : formula(other.formula), lps(std::move(other.lps)), neglps(std::move(other.neglps))
            {}

            Retval& operator=(Retval&& other) {
                lps = std::move(other.lps);
                neglps = std::move(other.neglps);
                formula = std::move(other.formula);
                return *this;
            }
            
            Retval() {
                lps.addEmpty();
                neglps.addEmpty();
            }
            
            ~Retval(){
            }
            
            bool isSet(){
                return (formula != nullptr && lps.size() != 0 && neglps.size() != 0);
            }
        private:

        };
    }
}

#endif /* RETVAL_H */

