#ifndef LINEARPROGRAM_H
#define LINEARPROGRAM_H
#include <algorithm>
#include <unordered_set>
#include <memory>
#include "../PetriNet.h"
#include "Member.h"
#include "Vector.h"
#include "PetriEngine/PQL/Contexts.h"
    
namespace PetriEngine {
    namespace Simplification {
        
        struct equation_t
        {
            double upper = std::numeric_limits<double>::infinity();
            double lower = -std::numeric_limits<double>::infinity();
            double operator [] (size_t i) const
            {
                return i > 0 ? upper : lower;
            }
            Vector* row;
            
            bool operator <(const equation_t& other) const
            {
                return row < other.row;
            }
        };
        
        class LinearProgram {
        private:
            enum result_t { UKNOWN, IMPOSSIBLE, POSSIBLE };
            result_t _result = result_t::UKNOWN;
            std::vector<equation_t> _equations;
        public:
            void swap(LinearProgram& other)
            {
                std::swap(_result, other._result);
                std::swap(_equations, other._equations);
            }
            virtual ~LinearProgram();
            LinearProgram()
            {
            };
            
            LinearProgram(const LinearProgram& other)
            : _result(other._result), _equations(other._equations)
            {
                
            }
            
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
            
            bool operator < (const LinearProgram& other) const
            {
                if(size() != other.size()) return size() < other.size();
                int res = memcmp(   _equations.data(), other._equations.data(), 
                                    _equations.size()*sizeof(equation_t));
                if(res != 0) return res < 0;
                return false;
            }
            
            bool knownImpossible() const { return _result == result_t::IMPOSSIBLE; }
            bool knownPossible() const { return _result == result_t::POSSIBLE; }
            bool isImpossible(const PQL::SimplificationContext& context);

            void make_union(const LinearProgram& other);
            
            std::ostream& print(std::ostream& ss, size_t indent = 0) const
            {
                for(size_t i = 0; i < indent ; ++i) ss << "\t";
                ss << "### LP\n";
                
                for(const equation_t& eq : _equations)
                {
                    for(size_t i = 0; i < indent ; ++i) ss << "\t";
                    eq.row->print(ss);
                    ss << " IN [" << eq.lower << ", " << eq.upper << "]\n";
                }
                
                for(size_t i = 0; i < indent ; ++i) ss << "\t";                
                ss << "### LP DONE";
                return ss;
            }
            
        };
        
        struct LPWrap
        {
            LPWrap(const std::shared_ptr<LinearProgram>& prog) : prog(prog) {};
            std::shared_ptr<LinearProgram> prog;
            
            bool operator < (const LPWrap& other) const
            {
                return (*prog) < (*other.prog);
            }
            
            bool operator ==(const LPWrap& other) const
            {
                return (*prog) == (*other.prog);
            }
            
            LinearProgram& operator*()
            {
                return *prog;
            }
            
            const LinearProgram& operator*() const
            {
                return *prog;
            }
            
            LinearProgram* operator ->()
            {
                return &(*prog);
            }

            const LinearProgram* operator ->() const
            {
                return &(*prog);
            }
            
        };
    }   
}

namespace std
{
    using namespace PetriEngine::Simplification;
    
    template <>
    struct hash<LPWrap>
    {
        size_t operator()(const LPWrap& k) const
        {
            return MurmurHash64A(k->equations().data(), 
                    k->size() * sizeof(int), 
                    1337);
        }
    };
}



#endif /* LINEARPROGRAM_H */

