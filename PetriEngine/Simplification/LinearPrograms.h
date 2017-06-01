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
            std::vector<LinearProgram*> lps;
            bool hasEmpty = false;
            enum result_t { UKNOWN, IMPOSSIBLE, POSSIBLE };
            result_t _result = result_t::UKNOWN;
        public:
            LinearPrograms(){
            }
                        
            LinearPrograms(LinearPrograms&& other) 
            : lps(std::move(other.lps)), hasEmpty(other.hasEmpty), _result(other._result)
            {
                
            }
            
            LinearPrograms& operator = (LinearPrograms&& other)
            {
                _result = other._result;
                lps = std::move(other.lps);
                hasEmpty = other.hasEmpty;
                return *this;
            }
            
            void swap(LinearPrograms& other)
            {
                lps.swap(other.lps);
                std::swap(hasEmpty, other.hasEmpty);
                std::swap(_result, other._result);
            }
            
            virtual ~LinearPrograms(){
                for(auto& l : lps) l->free();
            }
            
            bool satisfiable(const PQL::SimplificationContext context, bool force = false) {
                if(hasEmpty) 
                {
                    assert(_result == POSSIBLE);
                    return true;
                }
                if(_result != UKNOWN) return _result == POSSIBLE;
                for(int i = lps.size() - 1 ; i >= 0; --i)
                {
                    if(context.timeout()) return true;
                    if(!lps[i]->isImpossible(context.net(), context.marking(), context.getLpTimeout())){
                        _result = POSSIBLE;
                    }
                    else
                    {
                        lps.erase(lps.begin() + i);
                    }
                }
                if(_result == POSSIBLE) return true;
                _result = IMPOSSIBLE;
                return false;
            }
            
            size_t size() const {
                return lps.size();
            }
            
            void addEmpty()
            {
                _result = POSSIBLE;
                hasEmpty = true;
            }

            void add(LPCache* factory, const Member& lh, int constant, op_t op){
                Vector* vec = factory->createAndCache(lh.variables());
                auto lp = factory->cacheProgram(LinearProgram(vec, constant, op, factory));
                if(lp->knownImpossible()) return;
                auto itt = std::lower_bound(lps.begin(), lps.end(), lp);
                if(itt == lps.end() || *itt != lp)
                {
                    lps.insert(itt, lp);
                }
            }
            
            /**
             * Merges two linear programs, this invalidates lps2 to restrict 
             * temporary memory-overhead. 
             * @param lps2
             */
            void merge(LinearPrograms& lps2, LPCache* factory){
                if(_result == IMPOSSIBLE)
                {
                    lps2.clear();
                    return;
                }
                
                if(lps2._result == IMPOSSIBLE)
                {
                    swap(lps2);
                    lps2.clear();
                }
                
                if (lps.size() == 0) {
                    swap(lps2);
                    lps2.clear();
                    return;
                }
                else if (lps2.lps.size() == 0) {
                    return;
                }

                std::vector<LinearProgram*> merged;
                merged.reserve(lps.size() * lps2.lps.size());
                for(LinearProgram* lp1 : lps){        
                    for(LinearProgram* lp2 : lps2.lps){
                        LinearProgram* prog =                             
                                LinearProgram::lpUnion(*lp1, *lp2);
                        if(!prog->knownImpossible())
                        {
                            merged.push_back(prog);
                        }
                    }   
                }
                if(lps2.hasEmpty)
                {
                    merged.insert(merged.end(), lps.begin(), lps.end());
                    for(auto& l : lps) l->inc();
                }
                if(hasEmpty)
                {
                    merged.insert(merged.end(), lps2.lps.begin(), lps2.lps.end());
                    for(auto& l : lps2.lps) l->inc();
                }

                std::sort(merged.begin(), merged.end());
                merged.erase( unique( merged.begin(), merged.end() ), merged.end() );

                lps.swap(merged);
                lps2.clear();
                hasEmpty = hasEmpty && lps2.hasEmpty;
                if(hasEmpty)
                {
                    _result = POSSIBLE;
                }
                else if(_result == IMPOSSIBLE || lps2._result == IMPOSSIBLE)
                {
                    _result = IMPOSSIBLE;
                }
            }
            

            /**
             * Unions two linear programs, this invalidates lps2 to restrict 
             * temporary memory-overhead. 
             * @param lps2
             */
            void makeUnion(LinearPrograms& lps2)
            {
                if(_result == IMPOSSIBLE && lps2._result == IMPOSSIBLE)
                {
                    lps.clear();
                    return;
                }
                else if(_result == IMPOSSIBLE)
                {
                    swap(lps2);
                    return;
                }
                else if(lps2._result == IMPOSSIBLE)
                {
                    return;
                }
                
                lps.insert(lps.end(), lps2.lps.begin(), lps2.lps.end());
                std::sort(lps.begin(), lps.end());
                lps.erase( unique( lps.begin(), lps.end() ), lps.end() );

                lps2.clear();
                hasEmpty = hasEmpty || lps2.hasEmpty;
                if(hasEmpty)
                {
                    _result = POSSIBLE;
                }
                else
                {
                    if(_result == IMPOSSIBLE && lps2._result == IMPOSSIBLE)
                    {
                        _result = IMPOSSIBLE;
                    }
                    else
                    {
                        _result = result_t::UKNOWN;
                    }
                }
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
