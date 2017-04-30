#ifndef EQUATION_H
#define EQUATION_H
#include <memory>
#include "Member.h"

namespace PetriEngine {
    namespace Simplification {
        class Equation {
        public:
            enum op_t 
            {
                OP_EQ,
                OP_LE,
                OP_GE,
                OP_LT,
                OP_GT,
                OP_NE
            };

            Equation(const Member& lh, int constant, op_t op) 
            : row(lh.variables()), op(op), constant(constant) {

            }

            Equation(Member&& lh, int constant, op_t op) 
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
            op_t op;
            int constant;
            virtual ~Equation(){}
        };

        typedef std::shared_ptr<Equation> Equation_ptr;
    }
}

#endif /* EQUATION_H */

