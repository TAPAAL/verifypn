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
                virtual void reset() = 0;
        };

        typedef std::shared_ptr<AbstractProgramCollection> AbstractProgramCollection_ptr;

        class UnionCollection : public AbstractProgramCollection
        {
        protected:
            std::vector<AbstractProgramCollection_ptr> lps;
            size_t current = 0;
            
            virtual void satisfiableImpl(const PQL::SimplificationContext& context, bool use_ilp = false)
            {
                for(int i = lps.size() - 1; i >= 0; --i)
                {
                    if(lps[i]->satisfiable(context, use_ilp) || context.timeout())
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
                            !prog.isImpossible(context.net(), context.marking(), context.getLpTimeout(), true))
                        {
                            _result = POSSIBLE;
                            break;
                        }
                    }
                } while(hasmore);
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
            
            void clear()
            {
            }
            
        };
    }
}

#endif /* LINEARPROGRAMS_H */
