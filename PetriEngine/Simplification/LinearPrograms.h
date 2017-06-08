#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include <unordered_set>
#include "LinearProgram.h"
#include "PetriEngine/PQL/Contexts.h"
#include "../PetriNet.h"
        
namespace PetriEngine {
    namespace Simplification {
        
        class AbstractProgramCollection;
        typedef std::shared_ptr<AbstractProgramCollection> AbstractProgramCollection_ptr;
        
        
        class AbstractProgramCollection
        {
            protected:
                enum result_t { UNKNOWN, IMPOSSIBLE, POSSIBLE };
                result_t _result = result_t::UNKNOWN;
                
                virtual void satisfiableImpl(const PQL::SimplificationContext& context, bool use_ilp) = 0;
                bool has_empty = false;
            public:
                bool empty() { return has_empty; }
                
                virtual bool satisfiable(const PQL::SimplificationContext& context, bool use_ilp = false)
                {
                    if(context.timeout() || has_empty) return true;
                    if(_result != UNKNOWN)
                    {
                        if(!use_ilp || _result == IMPOSSIBLE)
                        {
                            return _result == POSSIBLE;
                        }
                    }
                    satisfiableImpl(context, use_ilp);
                    reset();
                    assert(_result != UNKNOWN);
                    return _result == POSSIBLE;
                }
                
                bool known_sat() { return _result == POSSIBLE; };
                bool known_unsat() { return _result == IMPOSSIBLE; };
                
                virtual void clear() = 0;
                
                virtual bool merge(bool& has_empty, LinearProgram& program) = 0;
                virtual void reset() = 0;
        };
        
        class UnionCollection : public AbstractProgramCollection
        {
        protected:
            AbstractProgramCollection_ptr left = nullptr;
            AbstractProgramCollection_ptr right = nullptr;
            AbstractProgramCollection_ptr current = nullptr;

            virtual void satisfiableImpl(const PQL::SimplificationContext& context, bool use_ilp = false)
            {
                if(left)
                {
                    if(left->satisfiable(context, use_ilp))
                    {
                        _result = POSSIBLE;
                        return;
                    }
                    left = nullptr;
                }
                if(right->satisfiable(context, use_ilp))
                {
                    _result = POSSIBLE;
                }
                else 
                {
                    right = nullptr;
                    _result = IMPOSSIBLE;
                }
            }

        public:
            UnionCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B) :
            AbstractProgramCollection(), left(A), right(B) 
            {
                this->has_empty = left->empty() || right->empty();
            };

            void clear()
            {
                left = nullptr;
                right = nullptr;
                current = nullptr;
            };
            
            virtual void reset()
            {
                if(left)  left->reset();
                if(right)  right->reset();
                current = nullptr;
            }

            virtual bool merge(bool& has_empty, LinearProgram& program)
            {
                if(current)
                {
                    if(!current->merge(has_empty, program))
                    {
                        if(current == left)
                        {
                            current = right;
                            return true;
                        }
                        else 
                        {
                            current = nullptr;
                            return false;
                        }
                    }
                    else
                    {
                        return true;
                    }
                }
                else
                {
                    current = left;
                    return merge(has_empty, program);
                }
            }
            
        };
        
        class MergeCollection : public AbstractProgramCollection
        {
        protected:
            AbstractProgramCollection_ptr left = nullptr;
            AbstractProgramCollection_ptr right = nullptr;

            LinearProgram tmp_prog;
            bool more_right = false;
            virtual void satisfiableImpl(const PQL::SimplificationContext& context, bool use_ilp = false)
            {
                // this is where the magic needs to happen
                LinearProgram prog;
                bool has_empty = false;
                bool hasmore = merge(has_empty, prog);
                do {
                    if(has_empty) 
                    {
                        _result = POSSIBLE;
                        return;
                    }
                    else
                    {
                        if(!prog.isImpossible(context.net(), context.marking(), context.getLpTimeout(), use_ilp))
                        {
                            _result = POSSIBLE;
                            return;
                        }
                    }
                    prog = LinearProgram();
                    has_empty = false;
                } while(hasmore);
                _result = IMPOSSIBLE;
                return;
            }

        public:
            MergeCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B) :
            AbstractProgramCollection(), left(A), right(B)
            {
                this->has_empty = left->empty() && right->empty();
                if( left->known_unsat() ||
                    right->known_unsat())
                {
                    clear();
                    _result = IMPOSSIBLE;
                }
                else
                {
                    _result = POSSIBLE;
                }
            };

            virtual void reset()
            {
                if(left)  left->reset();
                if(right)  right->reset();
                more_right = false;
                tmp_prog = LinearProgram();
            }
            
            void clear()
            {
                left = nullptr;
                right = nullptr;
            };

            virtual bool merge(bool& has_empty, LinearProgram& program)
            {
                bool rempty = false;
                bool lempty = false;
                bool more_left = left->merge(lempty, program);

                if(tmp_prog.size() == 0 || !more_left)
                {
                    more_right = right->merge(rempty, tmp_prog);
                }
                
                program.make_union(tmp_prog);
                
                has_empty = lempty && rempty;
                
                return more_left || more_right;
            }

        };
        
        class SingleProgram : public AbstractProgramCollection {
        private:
            LinearProgram program;
        protected:
            virtual void satisfiableImpl(const PQL::SimplificationContext& context, bool use_ilp = false)
            {
                // this is where the magic needs to happen
                if(!program.isImpossible(context.net(), context.marking(), context.getLpTimeout(), use_ilp))
                {
                    _result = POSSIBLE;
                }
                else 
                {
                    _result = IMPOSSIBLE;
                }
            }


        public:
            SingleProgram()
            :AbstractProgramCollection()
            {
                has_empty = true;
            }
            
            SingleProgram(LPCache* factory, const Member& lh, int constant, op_t op)
            :   AbstractProgramCollection(),
                program(factory->createAndCache(lh.variables()), constant, op, factory)
            {
                has_empty = program.size() == 0;
            }
            
            virtual ~SingleProgram(){
            }
            
            virtual void reset() {}
            
            virtual bool merge(bool& has_empty, LinearProgram& program)
            {
                program.make_union(program);
                has_empty = program.equations().size() == 0;
                return false;
            }            
            
            /**
             * Merges two linear programs, this invalidates lps2 to restrict 
             * temporary memory-overhead. 
             * @param lps2
             */
            
            /*void merge(SingleProgram& lps2, LPCache* factory){
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
                
                if (lps_union.size() == 0) {
                    swap(lps2);
                    lps2.clear();
                    return;
                }
                else if (lps2.lps_union.size() == 0) {
                    return;
                }

                std::vector<LPWrap> hadempty;
                
                auto& small = size() < lps2.size() ? lps_union : lps2.lps_union;
                auto& large = !(size() < lps2.size()) ? lps_union : lps2.lps_union;

                bool large_hadempty = &large == &lps_union ? hasEmpty : lps2.hasEmpty;
                bool small_hadempty = &small == &lps_union ? hasEmpty : lps2.hasEmpty;
                
                if(small_hadempty)
                {
                    hadempty = large;
                    for(auto it = large.begin(); it != large.end(); ++it){ 
                        for(size_t i = 0; i < small.size(); ++i){
                            LPWrap lw(std::make_shared<LinearProgram>(*(*it)));
                            lw->make_union(*small[i]);
                            if(!lw->knownImpossible()) 
                                hadempty.push_back(lw);
                        }
                    }
                }
                else
                {
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
                                if(!lw->knownImpossible()) 
                                    hadempty.push_back(lw);
                            }
                        }
                    }
                }

                

                if(large_hadempty)
                {
                    hadempty.insert(hadempty.end(), small.begin(), small.end());
                }
                
                if(large.size() < hadempty.size()) hadempty.swap(large);
                
                large.insert(large.end(), hadempty.begin(), hadempty.end());

                std::sort(large.begin(), large.end());
                large.erase( unique( large.begin(), large.end() ), large.end() );
               
                if(&large != &lps_union) lps_union.swap(large);

                hasEmpty = small_hadempty && large_hadempty;
                if(hasEmpty)
                {
                    _result = POSSIBLE;
                }
            }
            */


            /**
             * Unions two linear programs, this invalidates lps2 to restrict 
             * temporary memory-overhead. 
             * @param lps2
             */
/*            void makeUnion(LinearPrograms_ptr& other)
            {
                if(_result == IMPOSSIBLE && other->_result == IMPOSSIBLE)
                {
                    clear();
                    return;
                }
                else if(_result == IMPOSSIBLE)
                {
                    swap(*other);
                    other->clear();
                    return;
                }
                else if(other->_result == IMPOSSIBLE)
                {
                    other->clear();
                    return;
                }
                
                if(lps_merge.size() == 0) swap(*other);
                
                if(other->lps_merge.size() == 0)
                {
                    lps_union.insert(lps_union.end(), 
                            other->lps_union.begin(), 
                            other->lps_union.end());
                    
                    other->lps_union.clear();
                    std::sort(lps_union.begin(), lps_union.end());
                    lps_union.erase( unique( lps_union.begin(), lps_union.end() ), lps_union.end() );

                    lps.insert(lps.end(), 
                            other->lps.begin(), 
                            other->lps.end());
                    
                    other->lps.clear();
                    std::sort(lps.begin(), lps.end());
                    lps.erase( unique( lps.begin(), lps.end() ), lps.end() );                    
                }
                else
                {
                    lps_union.insert(other);
                }
                
                
                hasEmpty = hasEmpty || other->hasEmpty;
                if(hasEmpty)
                {
                    _result = POSSIBLE;
                }
                else
                {
                    _result = result_t::UKNOWN;
                }
            }*/
            
            void clear()
            {
            }
            
        };
    }
}

#endif /* LINEARPROGRAMS_H */
