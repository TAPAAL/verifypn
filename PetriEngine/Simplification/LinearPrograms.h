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
            std::vector<LPWrap> lps;
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
            }
            
            bool satisfiable(const PQL::SimplificationContext context, bool use_ilp = false) {
                if(hasEmpty) 
                {
                    assert(_result == POSSIBLE);
                    return true;
                }
                if(context.timeout()) return true;
                if(_result != UKNOWN)
                {
                    if(!use_ilp || _result == IMPOSSIBLE)
                        return _result == POSSIBLE;
                }
                
                for(int i = lps.size() -1; i >= 0 ; --i)
                {
                    if(context.timeout()) return true;
                    if(!lps[i]->isImpossible(context.net(), context.marking(), context.getLpTimeout(), use_ilp)){
                        _result = POSSIBLE;
                        break;
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
                auto lp = LPWrap(std::make_shared<LinearProgram>(vec, constant, op, factory));
                if(lp->knownImpossible()) return;
                auto lb = std::lower_bound(lps.begin(), lps.end(), lp);
                if(lb == lps.end() || !(*lb == lp))
                {
                    lps.insert(lb, lp);
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
                
                auto& small = size() < lps2.size() ? lps : lps2.lps;
                auto& large = !(size() < lps2.size()) ? lps : lps2.lps;
                
                std::vector<LPWrap> hadempty;
                if(lps2.hasEmpty) hadempty = large;
                // do everything inline
                for(auto it = large.begin(); it != large.end(); ++it){ 
                    for(size_t i = 0; i < small.size(); ++i){
                        if(i == (small.size() - 1))
                        {
                            (*it)->make_union(*small[i]);
                            if((*it)->knownImpossible())
                            {
                                it = large.erase(it);
                            }
                        }
                        else
                        {
                            LPWrap lw(std::make_shared<LinearProgram>(*(*it)));
                            lw->make_union(*small[i]);
                            hadempty.push_back(lw);
                        }
                    }
                }
                
                if(hasEmpty)
                {
                    large.insert(large.end(), small.begin(), small.end());
                }
                small.clear();
                
                if(hadempty.size() > 0)
                {
                    large.insert(large.end(), hadempty.begin(), hadempty.end());
                }

                std::sort(large.begin(), large.end());
                large.erase( unique( large.begin(), large.end() ), large.end() );

                if(&large != &lps) lps.swap(large);
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

                auto& small = lps2.size() < size() ? lps2.lps : lps;
                auto& large = !(lps2.size() < size()) ? lps2.lps : lps;
                
                if(std::min(lps2.lps.size(), size()) <= 5)
                {
                    
                    auto lit = large.begin();
                    for(auto& el : small)
                    {
                        auto lb = std::lower_bound(large.begin(), large.end(), el);
                        lit = large.insert(lb, el);
                    }
                    small.clear();
                }
                else
                {
                    large.insert(large.begin(), small.begin(), small.end());
                    small.clear();
                    std::sort(large.begin(), large.end());
                    lps.erase( unique( large.begin(), large.end() ), large.end() );
                }
                
                if(&lps != &large) lps.swap(large);
                
                hasEmpty = hasEmpty || lps2.hasEmpty;
                if(hasEmpty)
                {
                    _result = POSSIBLE;
                }
                else
                {
                    _result = result_t::UKNOWN;
                }
            }
            
            void clear()
            {
                lps.clear();
            }
            
            std::ostream& print(std::ostream& ss) const
            {
                ss << "### LPS IS ";
                switch(_result)
                {
                    case POSSIBLE:
                        ss << "POSSIBLE";
                        break;
                    case IMPOSSIBLE:
                        ss << "IMPOSSIBLE";
                        break;
                    case result_t::UKNOWN:
                        ss << "UNKNOWN";
                        break;
                }
                ss << "\n";
                for(auto& l : lps)
                {
                    l->print(ss, 1) << "\n";
                }
                ss << "### DONE";
                return ss;
            }
        };
    }
}

#endif /* LINEARPROGRAMS_H */
