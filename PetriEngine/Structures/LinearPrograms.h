#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include "LinearProgram.h"
#include "../PetriNet.h"

class LinearPrograms {
public:
    LinearPrograms();
    LinearPrograms(const LinearPrograms& orig);
    virtual ~LinearPrograms();
    std::vector<LinearProgram> lps;

    bool satisfiable(const PetriEngine::PetriNet& net, const PetriEngine::MarkVal* m0) {
       for(LinearProgram &lp : lps){
           if(!lp.isimpossible(net, m0)){
                   return true;
           }
       }
       return false;
    }
    
    static LinearPrograms merge(LinearPrograms& lps1, LinearPrograms& lps2){
        LinearPrograms res;
        for(LinearProgram& lp1 : lps1.lps){        
            for(LinearProgram& lp2 : lps2.lps){
                res.lps.push_back(LinearProgram::merge(lp1, lp2));
            }   
        }
        return res;
    }
private:
    
};

#endif /* LINEARPROGRAMS_H */