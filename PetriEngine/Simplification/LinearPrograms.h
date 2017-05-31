#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include <unordered_set>
#include "LinearProgram.h"
#include "PetriEngine/PQL/Contexts.h"
#include "../PetriNet.h"
        
namespace PetriEngine {
    namespace Simplification {

        class LinearPrograms {
        private:
            std::unordered_set<LinearProgram*> lps;
            bool hasEmpty = false;
        public:
            LinearPrograms(){
            }
                        
            LinearPrograms(LinearPrograms&& other) 
            : lps(std::move(other.lps)), hasEmpty(other.hasEmpty)
            {
                
            }
            
            LinearPrograms& operator = (LinearPrograms&& other)
            {
                lps = std::move(other.lps);
                hasEmpty = other.hasEmpty;
                return *this;
            }
            
            virtual ~LinearPrograms(){
            }
            
            bool satisfiable(const PQL::SimplificationContext context) {
                if(hasEmpty) return true;
                for(auto& itt : lps){
                    if(context.timeout()) return true;
                    if(!itt->isImpossible(context.net(), context.marking(), context.getLpTimeout())){
                        return true;
                    }
                }
                return false;
            }
            
            size_t size() const {
                return lps.size();
            }
            
            void addEmpty()
            {
                hasEmpty = true;
            }

            void add(LPCache* factory, const Member& lh, int constant, op_t op){
                Vector* vec = factory->createAndCache(lh.variables());
                lps.insert(factory->cacheProgram(LinearProgram(vec, constant, op, factory)));
            }
            
            /**
             * Merges two linear programs, this invalidates lps2 to restrict 
             * temporary memory-overhead. 
             * @param lps2
             */
            void merge(LinearPrograms& lps2, LPCache* factory){
                if (lps.size() == 0) {
                    lps.swap(lps2.lps);
                    return;
                }
                else if (lps2.lps.size() == 0) {
                    return;
                }
                
                std::unordered_set<LinearProgram*> merged;
                merged.reserve(lps.size() * lps2.lps.size());
                for(LinearProgram* lp1 : lps){        
                    for(LinearProgram* lp2 : lps2.lps){
                        merged.insert(
                            factory->cacheProgram(
                                LinearProgram::lpUnion(*lp1, *lp2)));
                    }   
                }
                if(lps2.hasEmpty)
                {
                    merged.insert(lps.begin(), lps.end());
                }
                if(hasEmpty)
                {
                    merged.insert(lps2.lps.begin(), lps2.lps.end());
                }
                lps.swap(merged);
                lps2.clear();
                hasEmpty = hasEmpty && lps2.hasEmpty;
            }
            

            /**
             * Unions two linear programs, this invalidates lps2 to restrict 
             * temporary memory-overhead. 
             * @param lps2
             */
            void makeUnion(LinearPrograms& lps2)
            {
                lps.insert(lps2.lps.begin(), lps2.lps.end());
                lps2.clear();
                hasEmpty = hasEmpty || lps2.hasEmpty;
            }
            
            void clear()
            {
                for(auto& l : lps)
                {
                    l->free();
                }
                lps.clear();
            }
        };
    }
}

#endif /* LINEARPROGRAMS_H */
