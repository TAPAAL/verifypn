/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TARAutomata.h
 * Author: petko
 *
 * Created on January 2, 2018, 10:06 AM
 */


#ifndef TARAUTOMATA_H
#define TARAUTOMATA_H
#include <cassert>
#include <algorithm>
#include <vector>
#include <string>
#include <set>

#include "range.h"
#include "PetriEngine/PetriNet.h"

namespace PetriEngine {
    namespace Reachability {
        class AutomataState;

        struct AutomataEdge
        {
            size_t edge;
            std::vector<size_t> to;

            bool operator == (const AutomataEdge& other) const
            {
                return edge == other.edge;
            }

            bool operator != (const AutomataEdge& other) const
            {
                return !(*this == other);
            }


            bool operator < ( const AutomataEdge& other) const
            {
                return edge < other.edge;
            };

            AutomataEdge(size_t e)
                : edge(e) {};

            AutomataEdge(const AutomataEdge& other) = default;
            AutomataEdge(AutomataEdge&&) = default;
            AutomataEdge& operator=(const AutomataEdge& other) = default;
            AutomataEdge& operator=(AutomataEdge&& other) = default;
            
            bool has_to(size_t i)
            {
                if(to.size() > 0 && to[0] == 0) return true;
                auto lb = std::lower_bound(to.begin(), to.end(), i);
                return lb != to.end() && *lb == i;
            }

            bool add(size_t i)
            {
                if(to.size() > 0 && to[0] == 0) return false;
                if(i == 0) { to.clear(); to.push_back(0); return true; }

                auto lb = std::lower_bound(to.begin(), to.end(), i);
                if(lb != to.end() && *lb == i)
                {
                    return false;
                }
                else
                {
                    to.insert(lb, i);
                    return true;
                }
            }

            bool remove(size_t i)
            {
                auto lb = std::lower_bound(to.begin(), to.end(), i);
                if(lb == to.end() || *lb != i)
                {
                    return false;
                }
                else
                {
                    to.erase(lb);
                    return true;
                }
            }
            
            std::ostream& operator<<(std::ostream& os) const
            {
                os << edge << " ==> ";
                for(size_t i : to) os << i << ", ";
                return os;
            }
        };        
        class TraceSet;
        struct AutomataState
        {
        private:
            std::vector<AutomataEdge> edges;
            bool accept = false;
            std::vector<size_t> simulates;
            std::vector<size_t> simulators;
            friend class TraceSet;
        public:
            prvector_t interpolant;
            AutomataState(prvector_t interpol) : interpolant(interpol) {};
            inline bool is_accepting() const
            {
                return accept;
            }

            inline void set_accepting(bool val = true)
            {
                accept = val;
            }

            inline bool has_edge(const size_t& e, size_t to)
            {
                AutomataEdge edge(e);
                auto lb = std::lower_bound(edges.begin(), edges.end(), edge);
                if(lb == edges.end() || *lb != edge)
                {
                    return false;
                }
                return lb->has_to(to);
            }

            inline bool has_any_edge(size_t prod, const size_t& e)
            {
                AutomataEdge edge(e);
                auto lb = std::lower_bound(edges.begin(), edges.end(), edge);
                if(lb == edges.end() || *lb != edge)
                {
                    return false;
                }
                return lb->to.size() > 0;
            }

            inline bool add_edge(const size_t& e, size_t to)
            {
                AutomataEdge edge(e);
                auto lb = std::lower_bound(edges.begin(), edges.end(), edge);
#ifndef NDEBUG
                bool isnew = false;
#endif
                if(lb == edges.end() || *lb != edge)
                {
                    assert(lb == edges.end() || *lb != edge);
                    lb = edges.insert(lb, edge);
#ifndef NDEBUG
                    isnew = true;
#endif
                }
                assert(*lb == edge);
                assert(is_sorted(edges.begin(), edges.end()));
                bool res = lb->add(to);
                assert(!isnew || res);
                assert(lb->to.size() >= 0);
                return res;
            }

            inline bool remove_edge(size_t e)
            {
                AutomataEdge edge(e);
                auto lb = std::lower_bound(edges.begin(), edges.end(), edge);
                if(lb == edges.end() || *lb != edge)
                    return false;
                edges.erase(lb);
                return true;
            }
            
            inline bool remove_edge(size_t e, size_t to)
            {
                AutomataEdge edge(e);
                auto lb = std::lower_bound(edges.begin(), edges.end(), edge);
                if(lb == edges.end() || *lb != edge)
                {
                    assert(lb == edges.end() || *lb != edge);
                    assert(is_sorted(edges.begin(), edges.end()));
                    return false;
                }
                assert(*lb == edge);
                assert(is_sorted(edges.begin(), edges.end()));
                bool removed = lb->remove(to);
                if(removed && lb->to.size() == 0)
                {
                    edges.erase(lb);
                }
                else
                {
                    assert(lb->to.size() >= 0);
                }
                return removed;
            }

            inline auto first_edge(size_t& e) const
            {
                AutomataEdge edge(e);
                auto lb = std::lower_bound(edges.begin(), edges.end(), edge);        
                return lb;
            }

            inline auto last_edge() const
            {
                return edges.end();
            }

            inline auto& get_edges() const
            {
                return edges;
            }
            
            std::ostream& print(std::ostream& os) const {
                os << "\t PREDICATE\n";
                interpolant.print(os);
                os << "\n";
                os << "\tSIMS [ ";
                for(auto s : simulates)
                    os << s << ", ";
                os << "]\n\tSIMED [";
                for(auto s : simulators)
                    os << s << ", ";
                os << "]\n";
                os << "\t EDGES\n";
                for(auto& e : edges)
                {
                    os << "\t\t<" << (e.edge ? std::to_string(e.edge-1) : "Q") << "> --> [";
                    for(auto t : e.to)
                        os << t << ", ";
                    os << "]\n";
                }
                return os;
            }
        };


        struct state_t 
        {

        private:
            size_t edgecnt; 
            std::vector<size_t> interpolant;
        public:
            bool operator == (const state_t& other)
            {
                if((edgecnt == 0) != (other.edgecnt == 0)) return false;
                if( interpolant.size() == other.interpolant.size() &&
                    std::equal( interpolant.begin(),        interpolant.end(), 
                                other.interpolant.begin(),  other.interpolant.end()))
                {
                    return true;
                }
                return false;
            }

            bool operator != (const state_t& other)
            {
                return !(*this == other);
            }

            bool operator <= (const state_t& other)
            {
                if((edgecnt == 0) != (other.edgecnt == 0)) return false;
                if( interpolant.size() <= other.interpolant.size() &&
                    std::includes(other.interpolant.begin(), other.interpolant.end(),
                                  interpolant.begin(), interpolant.end()))
                {
                    return true;
                }
                return false;
            }

            size_t& get_edge_cnt()
            {
                return edgecnt;
            }

            bool next_edge(const PetriNet& net)
            {
                ++edgecnt;
                return edgecnt > net.numberOfTransitions();
            }

            bool reset_edges(const PetriNet& net)
            {
                edgecnt = 0;
                return edgecnt > net.numberOfTransitions();
            }

            inline void add_interpolant(size_t ninter)
            {
                assert(is_sorted(interpolant.begin(), interpolant.end()));
                assert(ninter != 0);
                if(interpolant.size() == 0 || interpolant.back() < ninter)
                {
                    assert(interpolant.size() == 0 || interpolant.back() != ninter);
                    interpolant.push_back(ninter);
                }
                else
                {
                    auto lb = std::lower_bound(interpolant.begin(), interpolant.end(), ninter);
                    if(lb != interpolant.end() && *lb == ninter) { return; }
                    interpolant.insert(lb, ninter);
                }
                assert(is_sorted(interpolant.begin(), interpolant.end()));
            }

            inline std::vector<size_t>& get_interpolants()
            {
                assert(interpolant.size() == 0 || interpolant[0] != 0);
                assert(is_sorted(interpolant.begin(), interpolant.end()));
                return interpolant;
            }

            inline void set_interpolants(std::vector<size_t>&& interpolants)
            {
                assert(is_sorted(interpolants.begin(), interpolants.end()));
                assert(interpolant.size() == 0 || interpolant[0] != 0);
                interpolant = std::move(interpolants);
                assert(interpolant.size() == 0 || interpolant[0] != 0);
            }
        };
        
        typedef std::vector<state_t> trace_t;
    }
}


#endif /* TARAUTOMATA_H */