#ifndef LINEARPROGRAM_H
#define LINEARPROGRAM_H
#include "../PetriNet.h"
#include "../../lpsolve/lp_lib.h"
struct Member{
        std::vector<REAL> variables;
        REAL constant;
        
        Member(std::vector<REAL> vec, REAL marking) : variables(vec), constant(marking){
        }
        
        bool subtract(Member m){ 
            constant -= m.constant;
            for(int i = 0; i < m.variables.size(); i++){
                variables[i] -= m.variables[i];
            }
            return true;            
        }
        
        bool add(Member m){    
            constant += m.constant;
            for(int i = 0; i < variables.size(); i++){
                variables[i] += m.variables[i];
            }       
            return true;            
        }
        
        bool multiply(Member m){
            if(isConstant() && m.isConstant()){
                constant *= m.constant;
                return true;
            }
            return false;
        }
        
        bool isConstant(){
            for(REAL& v : variables){
                if(v>0) return false;
            }
            return true;
        }
    };
    
    struct Equation{
        Equation(Member lh, Member rh, int constr_type) : constr_type(constr_type){
            // put variables on left hand side
            for(int i = 0; i < rh.variables.size(); i++){
                row.push_back(lh.variables[i] - rh.variables[i]);
            }                    
            // put constant on right hand side
            constant = rh.constant - lh.constant; 
        }

        std::vector<REAL> row;
        int constr_type;
        REAL constant;
        bool canAnalyze = true;
    };
    
class LinearProgram {
public:
    LinearProgram();
    LinearProgram(std::vector<Equation> eqs) : equations(eqs){
    }
    void addEquation(Equation equation){
        equations.push_back(equation);
    }
    void addEquations(std::vector<Equation> eqs){
        for(Equation& eq : eqs){
            equations.push_back(eq);
        }
    }
    virtual ~LinearProgram();
    bool isimpossible(const PetriEngine::PetriNet& net, const PetriEngine::MarkVal* m0);
    
    static LinearProgram merge(LinearProgram& lp1, LinearProgram& lp2){
        LinearProgram res;
        res.addEquations(lp1.equations);
        res.addEquations(lp2.equations);
        return res;
    }
    
    std::vector<Equation> equations;
private:
    
};

#endif /* LINEARPROGRAM_H */

