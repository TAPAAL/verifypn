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

#ifdef ENABLE_TAR

#ifndef TARAUTOMATA_H
#define TARAUTOMATA_H
#include <cassert>
#include <algorithm>
#include <vector>
#include <string>
#include <set>

namespace PetriEngine {
    using namespace PQL;
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

            AutomataEdge(const AutomataEdge& other)
                    : edge(other.edge), to(other.to){};

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



        struct AutomataState
        {
        private:
            std::vector<AutomataEdge> edges;
            bool accept = false;
        public:
            z3::expr interpolant;
            AutomataState(z3::expr interpol) : interpolant(interpol) {};
            std::vector<size_t> simulates;
            std::vector<size_t> simulators;
            std::vector<size_t> restricts;
            std::vector<AutomataEdge> pre;
            uint16_t type = std::numeric_limits<uint16_t>::max();
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

            inline bool remove_edge(size_t& e, size_t to)
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

            inline auto first_edge(size_t& e)
            {
                AutomataEdge edge(e);
                auto lb = std::lower_bound(edges.begin(), edges.end(), edge);        
                return lb;
            }

            inline auto last_edge() const
            {
                return edges.end();
            }

            inline std::vector<AutomataEdge>& get_edges()
            {
                return edges;
            }
        };


        struct state_t 
        {

        private:
            size_t edgecnt; 
            std::vector<size_t> interpolant;
        public:
//            key_t location = 0;
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

            /*bool operator < (const state_t& other)
            {
                if(location != other.location) return location < other.location;
                if(edgecnt != other.edgecnt) return edgecnt < other.edgecnt;
                if(interpolant.size() != other.interpolant.size()) return interpolant.size() < other.interpolant.size();

            }*/



            size_t& get_edge_cnt()
            {
                return edgecnt;
            }

            bool next_edge(const PetriNet& net)
            {
//                std::string tname = "t0";
                ++edgecnt;
                /*std::set<std::string> names{"k56", "k31", "k33", "k57", "k34"};
                while(edgecnt <= net.numberOfTransitions() && names.count(net.transitionNames()[edgecnt - 1]) == 0)
                {
                    ++edgecnt;
                }*/

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

            inline void set_interpolants(std::vector<size_t> & interpolants)
            {
                assert(is_sorted(interpolants.begin(), interpolants.end()));
                assert(interpolant.size() == 0 || interpolant[0] != 0);
                interpolant = interpolants;
                assert(interpolant.size() == 0 || interpolant[0] != 0);
            }
        };
    }
}


#endif /* TARAUTOMATA_H */

#endif