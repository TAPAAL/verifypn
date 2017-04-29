#ifndef EQUATION_H
#define EQUATION_H
#include "Member.h"

namespace PetriEngine {
    namespace Simplification {
        class Equation {
        public:
            Equation(Member&& lh, int constant, std::string op) : op(op), constant(constant) {

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

