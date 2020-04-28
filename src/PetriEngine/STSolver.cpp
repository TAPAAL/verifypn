#include "PetriEngine/STSolver.h"

#include <glpk.h>
#include <cassert>

namespace PetriEngine {     
    
    STSolver::STSolver(Reachability::ResultPrinter& printer, const PetriNet& net, PQL::Condition * query, uint32_t depth) : printer(printer), _query(query), _net(net){
        if(depth == 0){
            _siphonDepth = _net._nplaces;
        } else {
            _siphonDepth = depth;
        }
        
        _m0 = _net._initialMarking;
        _analysisTime=0;
        constructPrePost(); // TODO: Refactor this out...
    }

    STSolver::~STSolver() {
    }

    bool STSolver::solve(uint32_t timelimit){
        _timelimit=timelimit;
        _start = std::chrono::high_resolution_clock::now();
        for(size_t t = 0; t < _net.numberOfTransitions(); ++t)
        {
            // Check that net is un-weighted and non-inhibited
            auto pre = _net.preset(t);
            if(pre.first == pre.second)
            {
                return false; // can always fire!
            }
            for(; pre.first != pre.second; ++pre.first)
            {
                if(pre.first->inhibitor) return false;
                if(pre.first->tokens != 1) return false;
            }
            auto post = _net.postset(t);
            for(; post.first != post.second; ++post.first)
            {
                if(post.first->tokens != 1) return false;
            }
        }
        
        for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
        {
            std::vector<size_t> siphon{p};
            if(!siphonTrap(siphon))
                return 0;
        }
        
        
        return GLP_INFEAS;
    }
    
    std::vector<size_t> STSolver::computeTrap(std::vector<size_t>& siphon)
    {
        if(siphon.empty()) return siphon;
        std::set<size_t> preset, postset;
        for(auto p : siphon)
        {
            preset.insert(_transitions.get() + _places[p].pre, _transitions.get() + _places[p].post);
            postset.insert(_transitions.get() + _places[p].post, _transitions.get() + _places[p+1].pre);
        }
        std::vector<size_t> diff(std::max(postset.size(), preset.size()));
        auto eit = std::set_difference( postset.begin(), postset.end(), 
                                        preset.begin(), preset.end(), diff.begin());
        if(eit == diff.begin())
        {
            std::cerr << "SIPHON!" << std::endl;
            return siphon;
        }
        else
        {
            for(auto t : diff)
            {
                auto pre = _net.preset(t);
                auto sit = siphon.begin();
                for(; pre.first != pre.second; ++pre.first)
                {
                    while(sit != std::end(siphon) && *sit < pre.first->place) ++sit;
                    if(sit == std::end(siphon)) 
                        break;
                    if(*sit == pre.first->place)
                        sit = siphon.erase(sit);
                }
            }
            if(siphon.empty())
                return siphon;
            else 
                return computeTrap(siphon);
        }
    }
    
    bool STSolver::siphonTrap(std::vector<size_t> siphon)
    {
        if(timeout())
            return false;
        std::set<size_t> preset, postset;
        for(auto p : siphon)
        {
            preset.insert(_transitions.get() + _places[p].pre, _transitions.get() + _places[p].post);
            postset.insert(_transitions.get() + _places[p].post, _transitions.get() + _places[p+1].pre);
        }
        std::vector<size_t> diff(std::max(postset.size(), preset.size()));
        auto eit = std::set_difference(preset.begin(), preset.end(), 
                                 postset.begin(), postset.end(), diff.begin()); 
        if(eit == diff.begin())
        {
            bool ok = false;
            auto trap = computeTrap(siphon);
            for(auto p : trap)
            {
                if(_m0[p] != 0) { ok = true; break; }
            }
            if(!ok) return false;
        }
        else if(diff.size())
        {
            auto t = diff.front();
            auto pre = _net.preset(t);
            auto sit = siphon.begin();
            for(; pre.first != pre.second; ++pre.first)
            {
                while(sit != siphon.end() && *sit < pre.first->place) ++sit;
                if(sit != siphon.end() && *sit == pre.first->place) continue;
                sit = siphon.insert(sit, pre.first->place);
                if(!siphonTrap(siphon))
                    return false;
                else
                    sit = siphon.erase(sit);
            }
        }
        _siphonPropperty = true;
        return true;
    }
    
    Reachability::ResultPrinter::Result STSolver::printResult(){
        if(_siphonPropperty){
            return printer.handle(0, _query, Reachability::ResultPrinter::NotSatisfied).first;
        } else {
            return Reachability::ResultPrinter::Unknown;
        }
    }
    bool STSolver::timeout() const {
        return (duration() >= _timelimit);
    }
    uint32_t STSolver::duration() const {
        auto end = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _start);
        return diff.count();
    }
    
    // TODO: Refactor this out... Copy paste from ReducingSuccessorGenerator.cpp
    // Also, we dont need the preset here.
    void STSolver::constructPrePost() {
        std::vector<std::pair<std::vector<uint32_t>, std::vector < uint32_t>>> tmp_places(_net._nplaces);
        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            const TransPtr& ptr = _net._transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; finv++) { // Post set of places
                tmp_places[_net._invariants[finv].place].second.push_back(t);
            }

            finv = linv;
            linv = _net._transitions[t + 1].inputs;
            for (; finv < linv; finv++) { // Pre set of places
                tmp_places[_net._invariants[finv].place].first.push_back(t);
            }
        }

        // flatten
        size_t ntrans = 0;
        for (auto p : tmp_places) {
            ntrans += p.first.size() + p.second.size();
        }
        _transitions = std::make_unique<uint32_t[]>(ntrans);

        _places = std::make_unique<place_t[]>(_net._nplaces + 1);
        uint32_t offset = 0;
        uint32_t p = 0;
        for (; p < _net._nplaces; ++p) {
            std::vector<uint32_t>& pre = tmp_places[p].first;
            std::vector<uint32_t>& post = tmp_places[p].second;

            // keep things nice for caches
            std::sort(pre.begin(), pre.end());
            std::sort(post.begin(), post.end());

            _places[p].pre = offset;
            offset += pre.size();
            _places[p].post = offset;
            offset += post.size();
            for (size_t tn = 0; tn < pre.size(); ++tn) {
                _transitions[tn + _places[p].pre] = pre[tn];
            }

            for (size_t tn = 0; tn < post.size(); ++tn) {
                _transitions[tn + _places[p].post] = post[tn];
            }

        }
        assert(offset == ntrans);
        _places[p].pre = offset;
        _places[p].post = offset;
    }
}
