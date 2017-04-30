#ifndef EQUATION_H
#define EQUATION_H
#include "Member.h"

namespace PetriEngine {
    namespace Simplification {
        class Equation {
        public:

            Equation(const Member& lh, int constant, const std::string& op) 
            : row(lh.variables()), op(op), constant(constant) {

            }

            Equation(Member&& lh, int constant, const std::string& op) 
            : row(std::move(lh.variables())), op(op), constant(constant) {

            }
            
            Equation(const Equation& eq) 
            : row(eq.row), op(eq.op), constant(eq.constant) {
            }      

            Equation(Equation&& other)
            :   row(std::move(other.row)), op(std::move(other.op)), 
                constant(other.constant)
            {
                
            }
            
            Equation(){
            }

            std::vector<int> row;
            std::string op;
            int constant;
            virtual ~Equation(){}
        };
    }
}

#endif /* EQUATION_H */

