#ifndef LINEARPROGRAM_H
#define LINEARPROGRAM_H
#include "../PetriNet.h"
#include "../../lpsolve/lp_lib.h"
#include "Member.h"
#include "Equation.h"
#include <algorithm>
    
class LinearProgram {
public:
    LinearProgram();
    LinearProgram(Equation eq){
        addEquation(eq);
    }
    
    LinearProgram(std::vector<Equation> eqs) : equations(eqs){
    }
    
    void addEquation(Equation eq){
        if(eq.op == "<"){
            eq.constant += 1; 
            eq.op = "<="; 
        } else if(eq.op == ">"){
            eq.constant -= 1; 
            eq.op = ">="; 
        } else if(eq.op == "!="){
            Equation eq2(eq);
            eq2.op = "<";
            eq.op = ">"; 
            addEquation(eq);
            addEquation(eq2);
            return;
        }
        
        equations.push_back(eq);
    }
    
    void addEquations(std::vector<Equation> eqs){
        for(Equation& eq : eqs){
            equations.push_back(eq);
        }
    }
    
    virtual ~LinearProgram();
    bool isimpossible(const PetriEngine::PetriNet& net, const PetriEngine::MarkVal* m0);
    
    int op(std::string op){
        if(op == "<="){ return 1; }
        if(op == ">="){ return 2; }
        if(op == "=="){ return 3; }
        return -1;
    }
    
    static LinearProgram lpUnion(LinearProgram& lp1, LinearProgram& lp2){
        LinearProgram res;
        res.addEquations(lp1.equations);
        res.addEquations(lp2.equations);
        return res;
    }
    
    std::vector<Equation> equations;
private:
    
};

#endif /* LINEARPROGRAM_H */

