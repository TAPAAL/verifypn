/* 
 * File:   Encoder.h
 * Author: Peter G. Jensen
 * 
 * Created on March 7, 2018, 1:51 PM
 */

#include <queue>

#include "CTL/SearchStrategy/HeuristicSearch.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "LTL/LTLValidator.h"
#include "CTL/PetriNets/PetriConfig.h"
#include "PetriEngine/PQL/FormulaSize.h"

namespace SearchStrategy {

    size_t HeuristicSearch::Wsize() const {
        return W.size();
    }

    void HeuristicSearch::pushToW(DependencyGraph::Edge* edge) {
        W.emplace(edge);
    }

    DependencyGraph::Edge* HeuristicSearch::popFromW() {
        auto res = W.top();
        W.pop();
        return res;
    }

    bool hcomp_t::operator()(const DependencyGraph::Edge* a, const DependencyGraph::Edge* b) const {
        // this is more than a little hacky - but we only use DGs for petri-nets for now
        auto* oa = static_cast<const PetriNets::PetriConfig*> (a->source);
        auto* ob = static_cast<const PetriNets::PetriConfig*> (b->source);
        LTL::LTLValidator is_a;
        PetriEngine::PQL::Visitor::visit(is_a, oa->query);
        LTL::LTLValidator is_b;
        PetriEngine::PQL::Visitor::visit(is_b, ob->query);
        if (is_b.bad() == is_a.bad()) {
            PetriEngine::PQL::FormulaSizeVisitor s_a;
            PetriEngine::PQL::FormulaSizeVisitor s_b;
            PetriEngine::PQL::Visitor::visit(s_a, oa->query);
            PetriEngine::PQL::Visitor::visit(s_b, ob->query);
            if (s_a.depth() == s_b.depth()) {
                if (s_a.size() == s_b.size())
                    return ((size_t) oa ^ (size_t) a) < ((size_t) ob ^ (size_t) b); // use pointers for *some* "randomness"
                return s_a.size() < s_b.size();
            }
            return s_a.depth() < s_b.depth();
        } else return is_b.bad() > is_a.bad();
    }
}
