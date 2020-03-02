#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include "LinearProgram.h"
#include "../PQL/Contexts.h"
#include "../PetriNet.h"
        
#include <set>

namespace PetriEngine {
    namespace Simplification {
        
        class AbstractProgramCollection
        {
            protected:
                enum result_t { UNKNOWN, IMPOSSIBLE, POSSIBLE };
                result_t _result = result_t::UNKNOWN;
                
                virtual void satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime) = 0;
                bool has_empty = false;
            public:
                virtual ~AbstractProgramCollection() {};
                bool empty() { return has_empty; }
                
                virtual bool satisfiable(const PQL::SimplificationContext& context, uint32_t solvetime = std::numeric_limits<uint32_t>::max())
                {
                    reset();
                    if(context.timeout() || has_empty || solvetime == 0) return true;
                    if(_result != UNKNOWN)
                    {
                        if(_result == IMPOSSIBLE)
                        {
                            return _result == POSSIBLE;
                        }
                    }
                    satisfiableImpl(context, solvetime);
                    assert(_result != UNKNOWN);
                    return _result == POSSIBLE;
                }
                
                bool known_sat() { return _result == POSSIBLE; };
                bool known_unsat() { return _result == IMPOSSIBLE; };
                
                virtual void clear() = 0;
                
                virtual bool merge(bool& has_empty, LinearProgram& program, bool dry_run = false) = 0;
                virtual void reset() = 0;
                virtual size_t size() const = 0;
        };

        typedef std::shared_ptr<AbstractProgramCollection> AbstractProgramCollection_ptr;

        class UnionCollection : public AbstractProgramCollection
        {
        protected:
            std::vector<AbstractProgramCollection_ptr> lps;
            size_t current = 0;
            size_t _size = 0;
            
            virtual void satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime)
            {
                for(int i = lps.size() - 1; i >= 0; --i)
                {
                    if(lps[i]->satisfiable(context, solvetime) || context.timeout())
                    {
                        _result = POSSIBLE;
                        return;
                    }
                    else
                    {
                        lps.erase(lps.begin() + i);
                    }
                }
                if(_result != POSSIBLE)
                    _result = IMPOSSIBLE;
            }

        public:
            UnionCollection(std::vector<AbstractProgramCollection_ptr>&& programs) :
            AbstractProgramCollection(), lps(std::move(programs)) 
            {
                for(auto& p : lps) _size += p->size();
            }
            
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
                for(auto& p : lps) _size += p->size();
            };

            void clear()
            {
                lps.clear();
                current = 0;
            };
            
            virtual void reset()
            {
                lps[0]->reset();
                current = 0;
            }

            virtual bool merge(bool& has_empty, LinearProgram& program, bool dry_run = false)
            {
                
                if(current >= lps.size()) 
                {
                    current = 0;
                }
                
                if(!lps[current]->merge(has_empty, program, dry_run))
                {
                    ++current;
                    if(current < lps.size()) lps[current]->reset();
                }
                
                return current < lps.size();
            }
            virtual size_t size() const
            {
                return _size;
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
            size_t nsat = 0;
            size_t curr = 0;
            size_t _size = 0;

            virtual void satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime)
            {
                // this is where the magic needs to happen
                bool hasmore = false;
                do {
                    if(context.timeout()) { _result = POSSIBLE; break; }
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
                        if( context.timeout() ||
                            !prog.isImpossible(context, solvetime))
                        {
                            _result = POSSIBLE;
                            break;
                        }
                    }
                    ++nsat;
                } while(hasmore);
                if(_result != POSSIBLE)
                    _result = IMPOSSIBLE;
                return;
            }

        public:
            MergeCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B) :
            AbstractProgramCollection(), left(A), right(B)
            {
                assert(A);
                assert(B);
                has_empty = left->empty() && right->empty();
                _size = left->size() * right->size();
            };

            virtual void reset()
            {
                if(right)  right->reset();
                
                merge_right = true;
                more_right  = true;
                rempty = false;
                
                tmp_prog = LinearProgram();
                curr = 0;
            }
            
            void clear()
            {
                left = nullptr;
                right = nullptr;
            };

            virtual bool merge(bool& has_empty, LinearProgram& program, bool dry_run = false)
            {               
                if(program.knownImpossible()) return false;
                bool lempty = false;
                bool more_left;
                while(true)
                {
                    lempty = false;
                    LinearProgram prog = program;
                    if(merge_right)
                    {
                        assert(more_right);
                        rempty = false;
                        tmp_prog = LinearProgram();
                        more_right = right->merge(rempty, tmp_prog, false);
                        left->reset();
                        merge_right = false;
                    }
                    ++curr;
                    assert(curr <= _size);
                    more_left = left->merge(lempty, prog/*, dry_run || curr < nsat*/);
                    if(!more_left) merge_right = true;
                    if(curr >= nsat || !(more_left || more_right))
                    {
                        if((!dry_run && prog.knownImpossible()) && (more_left || more_right))
                        {
                            continue;
                        }
                        if(!dry_run) program.swap(prog);
                        break;
                    }
                }
                if(!dry_run) program.make_union(tmp_prog);
                has_empty = lempty && rempty;
                return more_left || more_right;
            }

            virtual size_t size() const
            {
                return _size - nsat;
            }

            
        };
        
        class SingleProgram : public AbstractProgramCollection {
        private:
            LinearProgram program;
        protected:
            virtual void satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime)
            {
                // this is where the magic needs to happen
                if(!program.isImpossible(context, solvetime ))
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
            
            virtual bool merge(bool& has_empty, LinearProgram& program, bool dry_run = false)
            {
                if(dry_run) return false;
                program.make_union(this->program);
                has_empty = this->program.equations().size() == 0;
                assert(has_empty == this->has_empty);
                return false;
            }            
            
            void clear()
            {
            }
            
            virtual size_t size() const
            {
                return 1;
            }
            
        };
    }
}

#endif /* LINEARPROGRAMS_H */
