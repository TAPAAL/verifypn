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

            void merge(LinearPrograms& lps2){
                if (lps.size() == 0) {
                    lps.swap(lps2.lps);
                    return;
                }
                else if (lps2.lps.size() == 0) {
                    return;
                }
                
                std::vector<LinearProgram> merged;
                merged.reserve(lps.size() * lps2.lps.size());
                for(LinearProgram& lp1 : lps){        
                    for(LinearProgram& lp2 : lps2.lps){
                        merged.push_back(LinearProgram::lpUnion(lp1, lp2));
                    }   
                }
                lps.swap(merged);
                lps2.clear();
            }
            
            void makeUnion(LinearPrograms& lps2)
            {
                lps.insert(lps.end(), lps2.lps.begin(), lps2.lps.end());
                lps2.clear();
            }
            
            void clear()
            {
                lps.clear();
            }
        };
    }
}

#endif /* LINEARPROGRAMS_H */