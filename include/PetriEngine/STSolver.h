#ifndef STSOLVER_H
#define STSOLVER_H
#include "Structures/State.h"
#include "Reachability/ReachabilityResult.h"

#include <memory>
#include <chrono>
#include <glpk.h>

namespace PetriEngine {
    class STSolver {
    using REAL = double;
    struct STVariable {
        STVariable(int c, REAL v){
            colno=c;
            value=v;
        }
        int colno;
        REAL value;
    };
    
    struct place_t {
        uint32_t pre, post;
    };
        
    public:
        STSolver(Reachability::ResultPrinter& printer, const PetriNet& net, PQL::Condition * query, uint32_t depth);
        virtual ~STSolver();
        bool solve(uint32_t timeout);
        Reachability::ResultPrinter::Result printResult();
        
    private:    
        std::vector<size_t> computeTrap(std::vector<size_t>& siphon);
        bool siphonTrap(std::vector<size_t> siphon);
        uint32_t duration() const;
        bool timeout() const;
        void constructPrePost();
        bool _siphonPropperty = false;
        Reachability::ResultPrinter& printer;
        PQL::Condition * _query;
        std::unique_ptr<place_t[]> _places;
        std::unique_ptr<uint32_t[]> _transitions;
        const PetriNet& _net;
        const MarkVal* _m0;
        uint32_t _siphonDepth;
        uint32_t _timelimit;
        uint32_t _analysisTime;
        std::chrono::high_resolution_clock::time_point _start;
    };
}
#endif /* STSOLVER_H */

