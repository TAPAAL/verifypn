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

#ifdef VERIFYPN_TAR

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


                
        struct range_t {
            
            range_t() {};
            explicit range_t(uint32_t val) : _lower(val), _upper(val) {}
            range_t(uint32_t l, uint32_t u) : _lower(l), _upper(u) 
            {
                assert(_lower <= _upper);
            } 
            
            uint32_t _lower = std::numeric_limits<uint32_t>::min();
            uint32_t _upper = std::numeric_limits<uint32_t>::max();
            
            bool no_upper() const {
                return _upper == std::numeric_limits<uint32_t>::max();
            }
            bool no_lower() const {
                return _lower == std::numeric_limits<uint32_t>::min();
            }
            bool unbound() const {
                return no_lower() && no_upper();
            }
            std::ostream& print(std::ostream& os) const {
                if(no_lower())
                    os << "[-inf";
                else
                    os << "[" << _lower;
                os << ", ";
                if(no_upper())
                    os << "inf]";
                else
                    os << _upper << "]";
                return os;
            }
            
            std::pair<bool,bool> compare(const range_t& other) const {
                return std::make_pair(
                        _lower <= other._lower && _upper >= other._upper,
                        _lower >= other._lower && _upper <= other._upper
                        );
            }
            
            range_t& operator|=(uint32_t val) 
            {
                _lower = std::min(val, _lower);
                _upper = std::max(val, _upper);
                return *this;
            }
            
            range_t& operator&=(uint32_t val)
            {
                _lower = val;
                _upper = val;
                return *this;
            }
            
            range_t& operator -= (uint32_t val)
            {
                if(_lower < std::numeric_limits<uint32_t>::min() + val)
                    _lower = std::numeric_limits<uint32_t>::min();
                else
                    _lower -= val;
                if(_upper != std::numeric_limits<uint32_t>::max())
                    _upper -= val;
                return *this;
            }

            range_t& operator += (uint32_t val)
            {
                if(no_upper())
                    return *this;
                if(_lower != std::numeric_limits<uint32_t>::min())
                    _lower += val;

                if(_upper >= std::numeric_limits<uint32_t>::max()-val)
                    _upper = std::numeric_limits<uint32_t>::max();
                else
                    _upper += val;
                return *this;
            }            
        };
        
        struct placerange_t {
            range_t _range;
            uint32_t _place = std::numeric_limits<uint32_t>::max();
            placerange_t() {}
            placerange_t(uint32_t place) : _place(place) {};
            placerange_t(uint32_t place, const range_t& r) : _range(r), _place(place) {};
            placerange_t(uint32_t place, range_t&& r) : _range(r), _place(place) {};
            placerange_t(uint32_t place, uint32_t v) : _range(v), _place(place) {};
            placerange_t(uint32_t place, uint32_t l, uint32_t u) : _range(l, u), _place(place) {};
            std::ostream& print(std::ostream& os) const {
                os << "<P" << _place << "> in ";
                return _range.print(os);
            }
            std::pair<bool,bool> compare(const range_t& other) const
            {
                return _range.compare(other);
            }
            std::pair<bool,bool> compare(const placerange_t& other) const
            {
                assert(other._place == _place);
                if(other._place != _place)
                    return std::make_pair(false, false);
                return _range.compare(other._range);
            }
            
            placerange_t& operator|= (uint32_t val)
            {
                _range |= val;
                return *this;
            }
            
            placerange_t& operator&= (uint32_t val)
            {
                _range &= val;
                return *this;
            }
            
            placerange_t& operator -=(uint32_t val)
            {
                _range -= val;
                return *this;
            }

            placerange_t& operator +=(uint32_t val)
            {
                _range += val;
                return *this;
            }
            
            // used for sorting only!
            bool operator<(const placerange_t& other) const {
                return _place < other._place;
            }
        };
        
        struct prvector_t {
            std::vector<placerange_t> _ranges;
            const placerange_t* operator[](uint32_t place) const
            {
                auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
                if(lb == _ranges.end() || lb->_place != place)
                {
                    return nullptr;
                }
                else
                {
                    return &(*lb);
                }
            }
            
            placerange_t& find_or_add(uint32_t place)
            {
                auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
                if(lb == _ranges.end() || lb->_place != place)
                {
                    lb = _ranges.emplace(lb, place);
                }
                return *lb;
            }
            
            uint32_t lower(uint32_t place) const
            {
                auto* pr = (*this)[place];
                if(pr == nullptr)
                    return std::numeric_limits<uint32_t>::min();
                return pr->_range._lower;
            }
            
            uint32_t upper(uint32_t place) const
            {
                auto* pr = (*this)[place];
                if(pr == nullptr)
                    return std::numeric_limits<uint32_t>::max();
                return pr->_range._upper;
            }
            
            bool unbound(uint32_t place) const 
            {
                auto* pr = (*this)[place];
                if(pr == nullptr)
                    return true;
                return pr->_range.unbound();
            }
            
            void copy(const prvector_t& other)
            {
                _ranges = other._ranges;
                compact();
            }

            void compact() {
                for(size_t i = 0; i < _ranges.size(); ++i)
                {
                    if(_ranges[i]._range.unbound())
                        _ranges.erase(_ranges.begin() + i);
                }
            }
            
            bool is_compact() const {
                for(auto& e : _ranges)
                    if(e._range.unbound())
                        return false;
                return true;
            }
            
            std::pair<bool,bool> compare(const prvector_t& other) const
            {
                assert(is_compact());
                assert(other.is_compact());
                auto sit = _ranges.begin();
                auto oit = other._ranges.begin();
                std::pair<bool,bool> incl = std::make_pair(true, true);
                
                while(true)
                {
                    if(sit == _ranges.end())
                    {
                        incl.second = incl.second && (oit == other._ranges.end());
                        break;
                    }
                    else if(oit == other._ranges.end())
                    {
                        incl.first = false;
                        break;
                    }
                    else if(sit->_place == oit->_place)
                    {
                        auto r = sit->compare(*oit);
                        incl.first = incl.first && r.first;
                        incl.second &= incl.second && r.second;
                        ++sit;
                        ++oit;
                    }
                    else if(sit->_place < oit->_place)
                    {
                        incl.first = false;
                        ++sit;
                    }
                    else if(sit->_place > oit->_place)
                    {
                        incl.second = false;
                        ++oit;
                    }
                    if(!incl.first && !incl.second)
                        return incl;
                }
                return incl;
            }
            
            std::ostream& print(std::ostream& os) const
            {
                
                os << "{\n";
                for(auto& pr : _ranges)
                {
                    os << "\t";
                    pr.print(os) << "\n";
                }
                os << "}\n";
                return os;
            }
            
            bool is_true() const {
                return _ranges.empty();
            }
            
            bool is_false(size_t nplaces) const {
                if(_ranges.size() != nplaces) return false;
                for(auto& p : _ranges)
                {
                    if(p._range._lower != 0 ||
                       p._range._upper != 0)
                        return false;
                }
                return true;
            }
            
            bool operator<(const prvector_t& other) const {
                if(_ranges.size() != other._ranges.size())
                    return _ranges.size() < other._ranges.size();
                for(size_t i = 0; i < _ranges.size(); ++i)
                {
                    auto& r = _ranges[i];
                    auto& otr = other._ranges[i];
                    
                    if(r._place != otr._place)
                        return r._place < otr._place;
                    if(r._range._lower != otr._range._lower)
                        return r._range._lower < otr._range._lower;
                    if(r._range._upper != otr._range._upper)
                        return r._range._upper < otr._range._upper;
                }
                return false;
            }
            
            bool operator==(const prvector_t& other) const {
                auto r = compare(other);
                return r.first && r.second;
            }
            
            bool restricts(const std::vector<uint32_t>& writes) const
            {
                auto rit = _ranges.begin();
                for(auto p : writes)
                {
                    while(rit != std::end(_ranges) && 
                            (rit->_place < p || rit->_range.unbound() ))
                        ++rit;
                    if(rit == std::end(_ranges)) break;
                    if(rit->_place == p)
                        return true;
                }
                return false;
            }
        };
        

        struct AutomataState
        {
        private:
            std::vector<AutomataEdge> edges;
            bool accept = false;
        public:
            prvector_t interpolant;
            AutomataState(prvector_t interpol) : interpolant(interpol) {};
            std::vector<size_t> simulates;
            std::vector<size_t> simulators;
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