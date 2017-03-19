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
private:
    
};

#endif /* LINEARPROGRAMS_H */