/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   TraceSet.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 2, 2020, 6:03 PM
 */

#ifndef TRACESET_H
#define TRACESET_H

#include "range.h"
#include "TARAutomata.h"

#include <cinttypes>
#include <vector>
#include <map>

namespace PetriEngine {
    namespace Reachability {
        void inline_union(std::vector<size_t>& into, const std::vector<size_t>& other);
        class TraceSet {
        public:
            TraceSet(size_t max_vars);
            bool addTrace(trace_t& trace, std::vector<std::pair<prvector_t,bool>>& inter);
            void copyNonChanged(const std::vector<size_t>& from, const std::vector<int64_t>& modifiers, std::vector<size_t>& to) const;
            bool follow(const std::vector<size_t>& from, std::vector<size_t>& nextinter, size_t symbol);
            std::vector<size_t> maximize(const std::vector<size_t>& from);
            std::vector<size_t> minimize(const std::vector<size_t>& from);
            std::vector<size_t> initial() const { return _initial; }   
            std::ostream& print(std::ostream& out) const;
            void removeEdges(size_t edge);
        private:
            std::pair<bool, size_t> stateForPredicate(prvector_t& predicate);
            void computeSimulation(size_t index);
            std::map<prvector_t, size_t> _intmap;
            std::vector<AutomataState> _states;
            std::vector<size_t> _initial;
            const size_t _max_vars;
        };

    }
}

#endif /* TRACESET_H */

