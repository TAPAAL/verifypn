#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include "LinearProgram.h"
#include "../PetriNet.h"
        
namespace PetriEngine {
    namespace Simplification {

        class LinearPrograms {
        public:
            LinearPrograms(){
            }
            LinearPrograms(const LinearProgram& lp){
                add(lp);
            }
            virtual ~LinearPrograms(){
            }
            std::vector<LinearProgram> lps;
            
            bool satisfiable(const PetriEngine::PetriNet* net, const PetriEngine::MarkVal* m0, uint32_t timeout) {
                for(uint32_t i = 0; i < lps.size(); i++){
                    if(!lps[i].isImpossible(net, m0, timeout)){
                        return true;
                    }
                }
                return false;
            }

            void add(const LinearProgram& lp){
                lps.push_back(lp);
            }

            static LinearPrograms lpsMerge(LinearPrograms& lps1, LinearPrograms& lps2){
                if (lps1.lps.size() == 0) {
                    return lps2;
                }
                else if (lps2.lps.size() == 0) {
                    return lps1;
                }
                
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
        };
    }
}

#endif /* LINEARPROGRAMS_H */