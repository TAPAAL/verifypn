#ifndef EQUATION_H
#define EQUATION_H
#include <memory>
#include "Member.h"
#include "MurmurHash2.h"

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
            
            bool operator ==(const Equation& other) const
            {
                if(op != other.op || constant != other.constant || row.size() != other.row.size())
                {
                    return false;
                }
                
                return row == other.row;
            }
            
            std::vector<int> row;
            op_t op;
            int constant;
            virtual ~Equation(){}
        };

        typedef std::shared_ptr<Equation> Equation_ptr;
        
        struct EquationWrap
        {
            Equation_ptr equation;

            EquationWrap(Equation_ptr&& eq) : equation(std::move(eq)) {};

            EquationWrap(const Equation_ptr& eq) : equation(eq) {};
            
            bool operator==(const EquationWrap& other) const
            {
                return *equation == *other.equation;
            }
            
            Equation_ptr& operator->()
            {
                return equation;
            }

            const Equation_ptr& operator->() const
            {
                return equation;
            }

        };
        
    }
}

namespace std
{
    using namespace PetriEngine::Simplification;
    
    template <>
    struct hash<EquationWrap>
    {
        size_t operator()(const EquationWrap& k) const
        {
            return MurmurHash64A(k.equation->row.data(), 
                    k.equation->row.size() * sizeof(int), 
                    k.equation->constant ^ k.equation->op );
        }
    };
}

#endif /* EQUATION_H */

