#ifndef LINEARPROGRAM_H
#define LINEARPROGRAM_H
#include <algorithm>
#include <unordered_set>
#include "../PetriNet.h"
#include "Member.h"
#include "Vector.h"
    
namespace PetriEngine {
    namespace Simplification {
        
        struct equation_t
        {
            double upper = std::numeric_limits<double>::infinity();
            double lower = -std::numeric_limits<double>::infinity();
            double operator [] (size_t i)
            {
                return i > 0 ? upper : lower;
            }
            Vector* row;
            
            bool operator <(const equation_t& other)
            {
                return row < other.row;
            }
        };
        
        class LinearProgram {
        private:
            enum result_t { UKNOWN, IMPOSSIBLE, POSSIBLE };
            result_t _result = result_t::UKNOWN;
            std::vector<equation_t> _equations;
            size_t ref = 0;
            LPCache* factory;
        public:
            virtual ~LinearProgram();
            LinearProgram(LPCache* factory) : factory(factory){};
            LinearProgram(Vector* vec, int constant, op_t op, LPCache* factory);
            size_t size() const
            {
                return _equations.size();
            }
            
            const std::vector<equation_t>& equations() const
            {
                return _equations;
            }
            
            bool operator ==(const LinearProgram& other) const
            {
                if(size() != other.size()) return false;
                return memcmp(        _equations.data(), 
                                other._equations.data(), 
                                _equations.size()*sizeof(equation_t)) == 0;
            }
            
            void inc();
            
            void free();

            size_t refs() const { return ref; }
            
            bool isImpossible(const PetriEngine::PetriNet* net, const PetriEngine::MarkVal* m0, uint32_t timeout);
            void swap(LinearProgram& other)
            {
                std::swap(_result, other._result);
                std::swap(_equations, other._equations);
            }
            
            static LinearProgram lpUnion(LinearProgram& lp1, LinearProgram& lp2){
                LinearProgram res(lp1.factory);

                if( lp1._result == result_t::IMPOSSIBLE ||
                    lp2._result == result_t::IMPOSSIBLE)
                {
                    // impossibility is compositional -- linear constraint-systems are big conjunctions.
                    res._result = result_t::IMPOSSIBLE;
                }

                res._equations.reserve(lp1._equations.size() + lp2._equations.size());
                auto it1 = lp1._equations.begin();
                auto it2 = lp2._equations.begin();
                
                while(it1 != lp1._equations.end() && it2 != lp2._equations.end())
                {
                    if(it1->row < it2->row)
                    {
                        res._equations.push_back(*it1);
                    }
                    else if(it2->row < it1->row)
                    {
                        res._equations.push_back(*it2);                        
                    }
                    else
                    {
                        equation_t n = *it1;
                        n.lower = std::max(n.lower, it2->lower);
                        n.upper = std::min(n.upper, it2->upper);
                        if(n.upper < n.lower)
                        {
                            res._result = result_t::IMPOSSIBLE;
                            res._equations.clear();
                            return res;
                        }
                    }
                }
                
                if(it1 != lp1._equations.end()) 
                    res._equations.insert(res._equations.end(), it1, lp1._equations.end());

                if(it2 != lp2._equations.end()) 
                    res._equations.insert(res._equations.end(), it2, lp2._equations.end());
                
                for(equation_t& el : res._equations) el.row->inc();

                return res;
            }            
            
        };
    }
}

namespace std
{
    using namespace PetriEngine::Simplification;
    
    template <>
    struct hash<LinearProgram>
    {
        size_t operator()(const LinearProgram& k) const
        {
            return MurmurHash64A(k.equations().data(), 
                    k.size() * sizeof(int), 
                    1337);
        }
    };
}



#endif /* LINEARPROGRAM_H */

