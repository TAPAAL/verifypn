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

            _initial.push_back(1);
        }

        std::vector<size_t> TraceSet::minimize(const std::vector<size_t>& org)
        {
            std::vector<size_t> buffer;
            std::vector<size_t> minimal = org;
            size_t cur = 2;
            while (true) {
                buffer.clear();
                auto lb = std::lower_bound(minimal.begin(), minimal.end(), cur);
                if (lb == minimal.end()) break;
                cur = *lb;
                assert(!std::any_of(_states[cur].simulates.begin(), _states[cur].simulates.end(), [cur](auto a) { return cur == a; }));
                std::set_difference(minimal.begin(), minimal.end(),
                                    _states[cur].simulates.begin(), _states[cur].simulates.end(),
                                    std::back_inserter(buffer));
                minimal.swap(buffer);
                ++cur;
            }
            return minimal;
        }

        void TraceSet::copyNonChanged(const std::vector<size_t>& from, const std::vector<int64_t>& modifiers, std::vector<size_t>& to) const
        {
            std::vector<size_t> move;
            for (auto p : from) {
                if (!_states[p].interpolant.restricts(modifiers)) {
                    move.push_back(p);
                }
            }
            inline_union(to, move);
        }

        std::vector<size_t> TraceSet::maximize(const std::vector<size_t>& org)
        {
            std::vector<size_t> maximal = org;
            assert(is_sorted(maximal.begin(), maximal.end()));

            if (maximal.size() == 0 || maximal[0] != 1)
                maximal.insert(maximal.begin(), 1);

            assert(maximal.size() == 0 || maximal[0] != 0);
            assert(is_sorted(maximal.begin(), maximal.end()));
            for (size_t i : org) {
                inline_union(maximal, _states[i].simulates);
            }
#ifndef NDEBUG
            auto tmp = maximal;
            for (size_t i : maximal) {
                inline_union(tmp, _states[i].simulates);
            }
            assert(maximal == tmp);
#endif
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

        bool TraceSet::follow(const std::vector<size_t>& from, std::vector<size_t>& nextinter, size_t symbol)
        {
            if (nextinter.size() == 0 || nextinter[0] != 1) nextinter.insert(nextinter.begin(), 1);
            assert(is_sorted(from.begin(), from.end()));
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
                    auto other = nextinter.begin();
                    auto next = ae.to.begin();
                    if (*next == 0) {
                        return true;
                    }
                    for (; next != ae.to.end(); ++next) {
                        while (*other < *next && other != nextinter.end()) {
                            ++other;
                        }
                        if (other != nextinter.end() && *next == *other) {
                            ++other;
                        }
                        else {
                            other = nextinter.insert(other, *next);
                            assert(*next < _states.size());
                            if (_states[*next].simulates.size() > 0) {
                                inline_union(nextinter, _states[*next].simulates);
                                other = std::lower_bound(nextinter.begin(), nextinter.end(), (*next) + 1);
                            }
                            else {
                                ++other;
                            }
                        }
                        assert(nextinter.size() == 0 || nextinter[0] != 0);
                    }
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
                auto lb = std::lower_bound(_initial.begin(), _initial.end(), res.second);
                if (lb == std::end(_initial) || *lb != res.second) {
                    _initial.insert(lb, res.second);
                    some = true;
//                    trace[0].add_interpolant(res.second);
//                    assert(_initial == trace[0].get_interpolants());
                }
                last = res.second;
//                inter[0].first.print(std::cerr) << std::endl;
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
                        if(res.first)
                        {
                            // check if initial
                            for(size_t p = 0; p < _net.numberOfPlaces(); ++p)
                            {
                                if(_net.initial()[p] <= inter[j].first.upper(p) &&
                                   _net.initial()[p] >= inter[j].first.lower(p))
                                {
                                    auto lb = std::lower_bound(_initial.begin(), _initial.end(), res.second);
                                    if (lb == std::end(_initial) || *lb != res.second)
                                        _initial.insert(lb, res.second);
                                }
                            }
                        }
                        some |= res.first;
                        assert(inter[i].second || res.second == last);
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

