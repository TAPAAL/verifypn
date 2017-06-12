#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include <set>
#include "LinearProgram.h"
#include "PetriEngine/PQL/Contexts.h"
#include "../PetriNet.h"
        
namespace PetriEngine {
    namespace Simplification {
        
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
                    reset();
                    if(context.timeout() || has_empty) return true;
                    if(_result != UNKNOWN)
                    {
                        if(!use_ilp || _result == IMPOSSIBLE)
                        {
                            return _result == POSSIBLE;
                        }
                    }
                    satisfiableImpl(context, true);
                    assert(_result != UNKNOWN);
                    return _result == POSSIBLE;
                }
                
                bool known_sat() { return _result == POSSIBLE; };
                bool known_unsat() { return _result == IMPOSSIBLE; };
                
                virtual void clear() = 0;
                
                virtual bool merge(bool& has_empty, LinearProgram& program) = 0;
                virtual std::set<LinearProgram> old_merge() = 0;
                virtual void reset() = 0;
        };

        typedef std::shared_ptr<AbstractProgramCollection> AbstractProgramCollection_ptr;

        class UnionCollection : public AbstractProgramCollection
        {
        protected:
            std::vector<AbstractProgramCollection_ptr> lps;
            int current = 0;
            
            virtual void satisfiableImpl(const PQL::SimplificationContext& context, bool use_ilp = false)
            {
                for(int i = lps.size() - 1; i >= 0; --i)
                {
                    if(lps[i]->satisfiable(context, use_ilp))
                    {
                        _result = POSSIBLE;
//                        return;
                    }
                    else
                    {
                        lps.erase(lps.begin() + i);
                    }
                }
                if(_result != POSSIBLE)
                    _result = IMPOSSIBLE;
                old_merge();
            }

        public:
            UnionCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B) :
            AbstractProgramCollection(), lps({A,B}) 
            {
                has_empty = false;
                for(auto& lp : lps)
                {
                    has_empty = has_empty || lp->empty();
                    if(lp->known_sat() || has_empty) _result = POSSIBLE;
                    if(_result == POSSIBLE) break;
                }
            };

            void clear()
            {
                lps.clear();
                current = 0;
            };
            
            virtual void reset()
            {
                for(auto& lp : lps) lp->reset();
                current = 0;
            }

            virtual bool merge(bool& has_empty, LinearProgram& program)
            {
                
                if(current >= lps.size()) 
                {
                    current = 0;
                }
                
                if(!lps[current]->merge(has_empty, program))
                {
                    ++current;
                }
                
                return current < lps.size();
            }

            virtual std::set<LinearProgram> old_merge()
            {
                std::set<LinearProgram> res;
                if(lps.size() == 0)
                {
                    assert(false);
                    return res;
                }
                
                for(auto& lp : lps)
                {
                    lp->reset();
                    auto r2 = lp->old_merge();
                    res.insert(r2.begin(), r2.end());
                }

                /*std::set<LinearProgram> res2;
                bool more = false;
                do{
                    bool he  = false;
                    LinearProgram p;
                    more = merge(he, p);
                    assert(res.count(p) > 0);
                    res2.insert(p);
                }
                while(more);
                
                assert(res2.size() == res.size());*/
                
                return res;
            }
            
        };
        
        class MergeCollection : public AbstractProgramCollection
        {
        protected:
            AbstractProgramCollection_ptr left = nullptr;
            AbstractProgramCollection_ptr right = nullptr;

            LinearProgram tmp_prog;
            bool merge_right = true;
            bool more_right  = true;
            bool rempty = false;

            virtual void satisfiableImpl(const PQL::SimplificationContext& context, bool use_ilp = false)
            {
                // this is where the magic needs to happen
                
                bool hasmore = false;
                do {
                    LinearProgram prog;
                    bool has_empty = false;
                    hasmore = merge(has_empty, prog);
                    if(has_empty) 
                    {
                        _result = POSSIBLE;
                        return;
                    }
                    else
                    {
                        if(!prog.isImpossible(context.net(), context.marking(), context.getLpTimeout(), true))
                        {
                            _result = POSSIBLE;
                            break;
                        }
                    }
                } while(hasmore);
                //old_merge();
                if(_result != POSSIBLE)
                    _result = IMPOSSIBLE;
                return;
            }

        public:
            MergeCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B) :
            AbstractProgramCollection(), left(A), right(B)
            {
                has_empty = left->empty() && right->empty();
            };

            virtual void reset()
            {
                if(left)  left->reset();
                if(right)  right->reset();
                
                merge_right = true;
                more_right  = true;
                rempty = false;
                
                tmp_prog = LinearProgram();
            }
            
            void clear()
            {
                left = nullptr;
                right = nullptr;
            };

            virtual bool merge(bool& has_empty, LinearProgram& program)
            {               
                bool lempty = false;
                
                if(merge_right)
                {
                    assert(more_right);
                    tmp_prog = LinearProgram();
                    more_right = right->merge(rempty, tmp_prog);
                    left->reset();
                    merge_right = false;
                }

                bool more_left = left->merge(lempty, program);
                if(!more_left) merge_right = true;
//                std::cout << "M1" << std::endl;
//                program.print(std::cout, 0);

//                std::cout << "M2" << std::endl;                
//                program.print(std::cout, 0);
//                std::cout << std::endl;

                program.make_union(tmp_prog);
//                program.print(std::cout, 0);
//                std::cout << std::endl;

                has_empty = lempty && rempty;
                return more_left || more_right;
            }
            
            virtual std::set<LinearProgram> old_merge()
            {
                reset();
                auto l = left->old_merge();
                auto r = right->old_merge();
                std::set<LinearProgram> res;
                for(auto& a : l)
                {
                    for(auto& b : r)
                    {   
                        LinearProgram p = a;
                        p.make_union(b);
                        res.insert(p);
                    }
                }
                
                auto r2 = res;
                bool more = false;
                reset();
                do{
                    bool he  = false;
                    LinearProgram p;
                    more = merge(he, p);
                    assert(res.count(p) > 0);
                    r2.erase(p);
                }
                while(more);
                
                assert(r2.size() == 0);
                
                return res;
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
                assert(!has_empty);
            }
            
            virtual ~SingleProgram(){
            }
            
            virtual void reset() {}
            
            virtual bool merge(bool& has_empty, LinearProgram& program)
            {
                assert(this->program.size() == 1);
                program.make_union(this->program);
                has_empty = this->program.equations().size() == 0;
                assert(has_empty == this->has_empty);
                assert(!has_empty);
                return false;
            }            
            
            virtual std::set<LinearProgram> old_merge()
            {
                std::set<LinearProgram> res;
                res.insert(program);
                return res;
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
