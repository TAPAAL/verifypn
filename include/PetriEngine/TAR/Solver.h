/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   Solver.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 3, 2020, 8:08 PM
 */

#ifndef SOLVER_H
#define SOLVER_H

#include "TARAutomata.h"
#include "range.h"
#include "PetriEngine/PQL/PQL.h"

#include <utility>
#include <cinttypes>

namespace PetriEngine {
    namespace Reachability {
        using namespace PQL;
        class Solver {
        public:
            using inter_t = std::pair<prvector_t, bool>;
            using interpolant_t = std::vector<inter_t>;
            Solver(PetriNet& net, MarkVal* initial, Condition* query, std::vector<bool>& inq);
            std::pair<bool,interpolant_t>  check(trace_t& trace);
        private:
            std::pair<int64_t,int64_t> findFailure(trace_t& trace);
            void computeHoare(trace_t& trace, interpolant_t& ranges, int64_t fail);
            void computeTerminal(state_t& end, inter_t& last, int64_t place);
            PetriNet& _net;
            MarkVal* _initial;
            Condition* _query;
            std::vector<bool> _inq;
            std::unique_ptr<int64_t[]> _lastfailplace;
            std::unique_ptr<int64_t[]> _lfpc;
            std::unique_ptr<int64_t[]> _m;
            std::unique_ptr<MarkVal[]> _mark;
#ifndef NDEBUG
            SuccessorGenerator _gen;
#endif
        };
    }
}

#endif /* SOLVER_H */

