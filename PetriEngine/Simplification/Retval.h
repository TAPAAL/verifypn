#ifndef RETVAL_H
#define RETVAL_H
#include "LinearPrograms.h"

namespace PetriEngine {
    namespace Simplification {
        struct Retval {
            std::shared_ptr<PQL::Condition> formula = nullptr;
            AbstractProgramCollection_ptr lps;
            AbstractProgramCollection_ptr neglps;
            
            Retval (const std::shared_ptr<PQL::Condition> formula, 
            AbstractProgramCollection_ptr&& lps1, 
            AbstractProgramCollection_ptr&& lps2) 
            : formula(formula) {
                lps = std::move(lps1);
                neglps = std::move(lps2);
                
            }
                        
            Retval (const std::shared_ptr<PQL::Condition> formula) 
            : formula(formula) {
                lps = std::make_shared<SingleProgram>();
                neglps  = std::make_shared<SingleProgram>();
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
                lps = std::make_shared<SingleProgram>();
                neglps  = std::make_shared<SingleProgram>();
            }
            
            ~Retval(){
            }
            
        private:

        };
    }
}

#endif /* RETVAL_H */

