#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include "LinearProgram.h"
#include "../PetriNet.h"
        
namespace PetriEngine {
    namespace Simplification {

        class LinearPrograms {
        private:
            std::vector<LinearProgram> lps;
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
            
            bool satisfiable(const PetriEngine::PetriNet* net, const PetriEngine::MarkVal* m0, uint32_t timeout) {
                if(hasEmpty) return true;
                for(uint32_t i = 0; i < lps.size(); i++){
                    if(!lps[i].isImpossible(net, m0, timeout)){
                        return true;
                    }
                }
                return false;
            }
            
            size_t size() const {
                return lps.size();
            }

            void add(const LinearProgram&& lp){
                if(lp.size() == 0)
                {
                    hasEmpty = true;
                }
                else
                {
                    lps.push_back(std::move(lp));
                }
            }

            void add(Equation&& eq){
                lps.emplace_back(std::move(eq));
            }
            
            /**
             * Merges two linear programs, this invalidates lps2 to restrict 
             * temporary memory-overhead. 
             * @param lps2
             */
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
                        merged.push_back(std::move(LinearProgram::lpUnion(lp1, lp2)));
                    }   
                }
                if(lps2.hasEmpty)
                {
                    size_t osize = merged.size();
                    merged.resize(merged.size() + lps.size());
                    for(size_t i = 0; i < lps.size(); ++i) 
                        merged[osize + i].swap(lps[i]);
                }
                if(hasEmpty)
                {
                    size_t osize = merged.size();
                    merged.resize(merged.size() + lps2.size());
                    for(size_t i = 0; i < lps2.size(); ++i) 
                        merged[osize + i].swap(lps2.lps[i]);
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
                // move data, dont copy!
                size_t osize = lps.size();
                lps.resize(lps.size() + lps2.lps.size());
                for(size_t i = 0; i < osize; ++i)
                {
                    lps[osize + i].swap(lps2.lps[i]);
                }
                lps2.clear();
                hasEmpty = hasEmpty || lps2.hasEmpty;
            }
            
            void clear()
            {
                lps.clear();
            }
        };
    }
}

#endif /* LINEARPROGRAMS_H */