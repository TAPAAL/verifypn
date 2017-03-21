#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include "LinearProgram.h"
#include "../PetriNet.h"
#include "../PQL/PQL.h"
        
struct Retval {
    Condition_ptr formula;
    LinearPrograms lps;       
    Retval (Condition_ptr formula, LinearPrograms lps) : formula(formula), lps(lps) {           
    }
    Retval (Condition_ptr formula) : Retval(formula, LinearPrograms(LinearProgram())) {
    }
};

class LinearPrograms {
public:
    LinearPrograms(){
    }
    LinearPrograms(LinearProgram lp){
        add(lp);
    }
    virtual ~LinearPrograms(){
        
    }
    std::vector<LinearProgram> lps;

    bool satisfiable(const PetriEngine::PetriNet& net, const PetriEngine::MarkVal* m0) {
       for(LinearProgram &lp : lps){
           if(!lp.isimpossible(net, m0)){
                return true;
           }
       }
       return false;
    }
    
    void add(LinearProgram lp){
        lps.push_back(lp);
    }
    
    static LinearPrograms lpsMerge(LinearPrograms& lps1, LinearPrograms& lps2){
        LinearPrograms res;
        for(LinearProgram& lp1 : lps1.lps){        
            for(LinearProgram& lp2 : lps2.lps){
                res.add(LinearProgram::lpUnion(lp1, lp2));
            }   
        }
        return res;
    }
    
    static LinearPrograms lpsUnion(LinearPrograms& lps1, LinearPrograms& lps2){
        LinearPrograms res;
        for(LinearProgram& lp : lps1.lps){        
            res.add(lp);
        }
        for(LinearProgram& lp : lps2.lps){        
            res.add(lp);
        }
        return res;
    }
private:
    
};

#endif /* LINEARPROGRAMS_H */