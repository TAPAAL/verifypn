/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   TraceSet.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on April 2, 2020, 6:03 PM
 */

#include "PetriEngine/TAR/TraceSet.h"


namespace PetriEngine
{
    namespace Reachability
    {

        void inline_union(std::vector<size_t>& into, const std::vector<size_t>& other)
        {
            into.reserve(into.size() + other.size());
            auto iit = into.begin();
            auto oit = other.begin();

            while (oit != other.end()) {
                while (iit != into.end() && *iit < *oit) ++iit;
                if (iit == into.end()) {
                    into.insert(iit, oit, other.end());
                    break;
                }
                else if (*iit != *oit) {
                    iit = into.insert(iit, *oit);
                }
                ++oit;
            }
        }

        TraceSet::TraceSet(const PetriNet& net)
        : _net(net)
        {
            init();
        }

        void TraceSet::init()
        {
            prvector_t truerange;
            prvector_t falserange;
            {
                for (size_t v = 0; v < _net.numberOfPlaces(); ++v)
                    falserange.find_or_add(v) &= 0;
            }
            assert(falserange.is_false(_net.numberOfPlaces()));
            assert(truerange.is_true());
            assert(!falserange.is_true());
            assert(falserange.is_false(_net.numberOfPlaces()));
            assert(truerange.compare(falserange).first);
            assert(!truerange.compare(falserange).second);
            assert(falserange.compare(truerange).second);
            assert(!falserange.compare(truerange).first);
            _states.emplace_back(falserange); // false
            _states.emplace_back(truerange); // true
            computeSimulation(0);

            assert(_states[1].simulates.size() == 0);
            assert(_states[1].simulators.size() == 1);
            assert(_states[0].simulates.size() == 1);
            assert(_states[0].simulators.size() == 0);

            assert(_states[1].simulators[0] == 0);
            assert(_states[0].simulates[0] == 1);

            _intmap.emplace(falserange, 0);
            _intmap.emplace(truerange, 1);

            _initial.insert(1);
        }

        void TraceSet::clear()
        {
            _initial.clear();
            _intmap.clear();
            _states.clear();
            init();
        }

        
        std::set<size_t> TraceSet::minimize(const std::set<size_t>& org) const
        {
            std::set<size_t> minimal = org;
            for(size_t i : org)
                for(auto e : _states[i].simulates)
                    minimal.erase(e);
            return minimal;
        }

        void TraceSet::copyNonChanged(const std::set<size_t>& from, const std::vector<int64_t>& modifiers, std::set<size_t>& to) const
        {
            for (auto p : from)
                if (!_states[p].interpolant.restricts(modifiers))
                    to.insert(p);
        }

        std::set<size_t> TraceSet::maximize(const std::set<size_t>& org) const
        {
            auto maximal = org;
            maximal.insert(1);
            for (size_t i : org) 
                maximal.insert(_states[i].simulates.begin(), _states[i].simulates.end());
            return maximal;
        }

        std::pair<bool, size_t> TraceSet::stateForPredicate(prvector_t& predicate)
        {
            predicate.compress();
            assert(predicate.is_compact());
            if (predicate.is_true()) {
                return std::make_pair(false, 1);
            }
            else if (predicate.is_false(_net.numberOfPlaces())) {
                return std::make_pair(false, 0);
            }

            auto astate = _states.size();
            auto res = _intmap.emplace(predicate, astate);
            if (!res.second) {
#ifndef NDEBUG
                if (!(_states[res.first->second].interpolant == predicate)) {
                    assert(!(_states[res.first->second].interpolant < predicate));
                    assert(!(predicate < _states[res.first->second].interpolant));
                    assert(false);
                }
                for (auto& e : _intmap) {
                    assert(e.first == _states[e.second].interpolant);
                }
#endif
                return std::make_pair(false, res.first->second);
            }
            else {
                _states.emplace_back(predicate);
                computeSimulation(astate);
                res.first->second = astate;
                assert(_states[astate].interpolant == predicate);
#ifndef NDEBUG
                for (auto& s : _states) {
                    if (s.interpolant == predicate) {
                        if (&s == &_states[astate]) continue;
                        assert((s.interpolant < predicate) ||
                               (predicate < s.interpolant));
                        assert(false);
                    }
                }
                for (auto& e : _intmap) {
                    assert(e.first == _states[e.second].interpolant);
                }
#endif
                bool ok = true;
                for(auto& r : predicate._ranges)
                {

                    if(_net.initial()[r._place] > r._range._upper ||
                       _net.initial()[r._place] < r._range._lower)
                    {
                        ok = false;
                        break;
                    }
                }
                if(ok)
                {
                    auto lb = std::lower_bound(_initial.begin(), _initial.end(), astate);
                    if (lb == std::end(_initial) || *lb != res.second)
                    {
                        _initial.insert(lb, astate);
                    }
                }
                // check which edges actually change the predicate, add rest to automata
                for(size_t t = 0; t < _net.numberOfTransitions(); ++t)
                {
                    auto pre = _net.preset(t);
                    bool ok = true;
                    bool changes = false;
                    for(; pre.first != pre.second; ++pre.first)
                    {
                        auto it = predicate[pre.first->place];
                        if(it)
                        {
                            changes = true;
                            auto post = _net.postset(t);
                            int64_t change = pre.first->tokens;
                            if(pre.first->tokens > it->_range._upper)
                                _states[astate].add_edge(t+1, 0);
                            change *= -1;
                            for(; post.first != post.second; ++post.first)
                            {
                                if(post.first->place == pre.first->place)
                                {
                                    change += post.first->tokens;
                                    break;
                                }
                            }
                            if(change < 0 && !it->_range.no_lower())
                                ok = false;
                            else if(change > 0 && !it->_range.no_upper())
                                ok = false;
                        }
                        if(!ok) break;
                    }
                    
                    auto post = _net.postset(t);
                    for(; post.first != post.second; ++post.first)
                    {
                        auto it = predicate[post.first->place];
                        if(it)
                        {
                            changes = true;
                            auto pre = _net.preset(t);
                            int64_t change = post.first->tokens;
                            for(; pre.first != pre.second; ++pre.first)
                            {
                                if(pre.first->place == post.first->place)
                                {
                                    change -= pre.first->tokens;
                                    break;
                                }
                            }
                            if(change < 0 && !it->_range.no_lower())
                                ok = false;
                            else if(change > 0 && !it->_range.no_upper())
                                ok = false;
                        }
                        if(!ok) break;
                    }
                    
                    if(changes && ok)
                    {
                        _states[astate].add_edge(t+1, astate);
                    }
                }
                
                
                return std::make_pair(true, astate);
            }
        }

        void TraceSet::computeSimulation(size_t index)
        {
            AutomataState& state = _states[index];
            assert(index == _states.size() - 1 || index == 0);
            for (size_t i = 0; i < _states.size(); ++i) {
                if (i == index) continue;
                AutomataState& other = _states[i];
                std::pair<bool, bool> res = other.interpolant.compare(state.interpolant);
                assert(!res.first || !res.second);
                if (res.first) {
                    state.simulates.emplace_back(i);
                    auto lb = std::lower_bound(other.simulators.begin(), other.simulators.end(), index);
                    if (lb == std::end(other.simulators) || *lb != index)
                        other.simulators.insert(lb, index);
                    other.interpolant.compare(state.interpolant);
                }
                if (res.second) {
                    state.simulators.emplace_back(i);
                    auto lb = std::lower_bound(other.simulates.begin(), other.simulates.end(), index);
                    if (lb == std::end(other.simulates) || *lb != index)
                        other.simulates.insert(lb, index);
                    other.interpolant.compare(state.interpolant);
                }
            }

            assert(_states[1].simulates.size() == 0);
            assert(_states[0].simulators.size() == 0);
        }

        bool TraceSet::follow(const std::set<size_t>& from, std::set<size_t>& nextinter, size_t symbol)
        {
            nextinter.insert(1);
            for (size_t i : from) {
                if (i == 0) {
                    assert(false);
                    continue;
                }
                AutomataState& as = _states[i];
                if (as.is_accepting()) {
                    assert(false);
                    break;
                }

                auto it = as.first_edge(symbol);

                while (it != as.get_edges().end()) {
                    if (it->edge != symbol) {
                        break;
                    }
                    auto& ae = *it;
                    ++it;
                    assert(ae.to.size() > 0);
                    if (ae.to.front() == 0) 
                        return true;
                    nextinter.insert(ae.to.begin(), ae.to.end());
                }
            }
            return false;
        }

        void TraceSet::removeEdges(size_t edge)
        {
            for(auto& s : _states)
            {
                s.remove_edge(edge);
            }
            // TODO back color here to remove non-accepting end-components.
        }
        
        bool TraceSet::addTrace(std::vector<std::pair<prvector_t, size_t>>& inter)
        {
            assert(inter.size() > 0);
            bool some = false;

            size_t last = 1;
            {
                auto res = stateForPredicate(inter[0].first);
                some |= res.first;
                last = res.second;
                //inter[0].first.print(std::cerr) << std::endl;
            }
#ifndef NDEBUG
            bool added_terminal = false;
#endif
            for (size_t i = 0; i < inter.size(); ++i) {
                size_t j = i + 1;
//                std::cerr << " >> T" << inter[i].second << " <<" << std::endl;
                if (j == inter.size()) {
                    some |= _states[last].add_edge(inter[i].second, 0);
#ifndef NDEBUG                    
                    added_terminal = true;
#endif
//                    std::cerr << "FALSE" << std::endl;
                }
                else {
//                    inter[j].first.print(std::cerr) << std::endl;
                    /*if (!inter[i].second)
                        trace[j].add_interpolant(last);
                    else*/ {
                        auto res = stateForPredicate(inter[j].first);
                        some |= res.first;
//                        assert(inter[i].second || res.second == last);
                        some |= _states[last].add_edge(inter[i].second, res.second);
                        last = res.second;
                    }
                }
            }
            assert(added_terminal);
            return some;
        }

        std::ostream& TraceSet::print(std::ostream& out) const
        {
            out << "digraph graphname {\n";
            for (size_t i = 0; i < _states.size(); ++i) {
                auto& s = _states[i];
                out << "\tS" << i << " [label=\"";
                if (s.interpolant.is_true()) {
                    out << "TRUE";
                }
                else if (s.interpolant.is_false(_net.numberOfPlaces())) {
                    out << "FALSE";
                }
                else {
                    s.interpolant.print(out);
                }
                out << "\",shape=";
                auto lb = std::lower_bound(_initial.begin(), _initial.end(), i);
                if (lb != std::end(_initial) && *lb == i)
                    out << "box,color=green";
                else
                    out << "box";
                out << "];\n";
            }
            for (size_t i = 0; i < _states.size(); ++i) {
                auto& s = _states[i];
                for (auto& e : s.get_edges()) {
                    out << "\tS" << i << "_" << e.edge << " [label=\"" <<
                            (e.edge == 0 ? "Q" : std::to_string(e.edge - 1))
                            << "\",shape=diamond,style=dashed];\n";
                    out << "\tS" << i << " -> S" << i << "_" << e.edge << ";\n";
                    for (auto& t : e.to) {
                        out << "\tS" << i << "_" << e.edge << " -> S" << t << ";\n";
                    }
                }
            }

            out << "}\n";
            return out;
        }
    }
}

