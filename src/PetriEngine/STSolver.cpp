#include "PetriEngine/STSolver.h"

#include <cassert>

namespace PetriEngine {     
    
    STSolver::STSolver(Reachability::ResultPrinter& printer, const PetriNet& net, PQL::Condition * query, uint32_t depth) : printer(printer), _query(query), _net(net){
        if(depth == 0){
            _siphonDepth = _net._nplaces;
        } else {
            _siphonDepth = depth;
        }
        
        _m0 = _net._initialMarking;
        _analysisTime = 0;
        _diff.resize(_net.numberOfTransitions());
        constructPrePost(); // TODO: Refactor this out...
    }

    STSolver::~STSolver() {
    }

    bool STSolver::solve(uint32_t timelimit){
        if(_net.numberOfPlaces() == 0) return false;
        _timelimit = timelimit;
        _start = std::chrono::high_resolution_clock::now();
        
        // check that constraints on net are valid
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
        
        // construct the siphon starting at each place
        std::vector<bool> has_st(_net.numberOfPlaces());
        for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
        {
            std::vector<size_t> siphon{p};
            std::set<size_t> preset, postset;
            extend(p, preset, postset);
            if(!siphonTrap(siphon, has_st, preset, postset))
            {
                if(timeout())
                {
                    std::cout << "TIMEOUT OF SIPHON" << std::endl;
                }
                return false;
            }
            has_st[p] = true;
        }
        _siphonPropperty = true;
        return true;
    }
    
    size_t STSolver::computeTrap(std::vector<size_t>& trap, const std::set<size_t>& preset, const std::set<size_t>& postset, size_t marked_count)
    {
        if(trap.empty()) return 0;
        // compute DIFF = T* \ *T
        auto eit = std::set_difference( postset.begin(), postset.end(), 
                                        preset.begin(), preset.end(), _diff.begin());
        if(eit == _diff.begin())
        {
            // DIFF = empty
            if(marked_count > 0)
            {                
                auto it = trap.begin() + (std::rand() % trap.size());
                size_t dummy = 0;
                _antichain.insert(dummy, trap);
                if(_m0[*it] == 0 || marked_count > 1)
                {
                    // try to compute a random smaller trap
                    auto rm = (_m0[*it] > 0 ? 1 : 0);
                    trap.erase(it);
                    std::set<size_t> npreset, npostset;
                    for(auto p : trap)
                        extend(p, npreset, npostset);
                    computeTrap(trap, npreset, npostset, marked_count - rm);
                }
            }
            return marked_count;
        }
        else
        {
            // run through every transition in DIFF (as it cannot be in trap)
            // and remove preset (i.e. T'=T \ *DIFF)
            for(auto it = _diff.begin(); it != eit; ++it)
            {
                auto t = *it;
                auto pre = _net.preset(t);
                auto sit = trap.begin();
                for(; pre.first != pre.second; ++pre.first)
                {
                    while(sit != std::end(trap) && *sit < pre.first->place) ++sit;
                    if(sit == std::end(trap)) 
                        break;
                    if(*sit == pre.first->place)
                    {
                        
                        sit = trap.erase(sit);
                        if(_m0[pre.first->place] != 0)
                        {
                            assert(marked_count > 0);
                            --marked_count;
                            if(marked_count == 0)
                                return 0;
                        }
                    }
                }
            }
            if(trap.empty() || marked_count == 0)
            {
                // no trap
                assert(marked_count == 0);
                return 0;
            }
            else 
            {
                // rebuild pre and postset, then try to compute new fixpoint
                // i.e. build trap with smaller set.
                std::set<size_t> npreset, npostset;
                for(auto p : trap)
                    extend(p, npreset, npostset);
                return computeTrap(trap, npreset, npostset, marked_count);
            }
        }
    }
    
    void STSolver::extend(size_t place, std::set<size_t>& pre, std::set<size_t>& post)
    {
        pre.insert(_transitions.get() + _places[place].pre, _transitions.get() + _places[place].post);
        post.insert(_transitions.get() + _places[place].post, _transitions.get() + _places[place+1].pre);
    }
    
    bool STSolver::siphonTrap(std::vector<size_t> siphon, const std::vector<bool>& has_st, const std::set<size_t>& preset, const std::set<size_t>& postset)
    {
        if(timeout())
            return false;

        // we can use an inclussion-check to avoid recomputation 
        // (we abuse the antichain structure here)
        size_t dummy = 0;
        if(_antichain.subsumed(dummy, siphon))
            return true;

        auto eit = std::set_difference(preset.begin(), preset.end(), 
                                 postset.begin(), postset.end(), _diff.begin()); 
        if(eit == _diff.begin())
        {
            size_t marked_count = 0;
            for(auto p : siphon)
                if(_m0[p] != 0) ++marked_count;
            if(marked_count == 0) return false;
            marked_count = computeTrap(siphon, preset, postset, marked_count);
            if(marked_count == 0) return false;
            else return true;
        }
        else
        {
            auto t = _diff.front();
            auto pre = _net.preset(t);
            auto sit = siphon.begin();
            for(; pre.first != pre.second; ++pre.first)
            {
                while(sit != siphon.end() && *sit < pre.first->place) ++sit;
                if(sit != siphon.end() && *sit == pre.first->place) continue;
                if(has_st[pre.first->place])
                {
                    // we know that all siphons generated as fixpoints starting
                    // in pre.first->place have the st property, so by transitivity
                    // any fixpoint containing pre.first->place will also have
                    // this property
                    // this is quicker than the antichain check.
                    continue;
                }
                sit = siphon.insert(sit, pre.first->place);
                auto npre = preset;
                auto npost = postset;
                extend(pre.first->place, npre, npost);
                if(!siphonTrap(siphon, has_st, npre, npost))
                    return false;
                else
                    sit = siphon.erase(sit);
            }
        }
        
        // Any super-siphon has a marked trap, insert into antichain.
        _antichain.insert(dummy, siphon);
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
