// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
///////////////////////////////////////////////////////////////////////////////
//
// This file is a part of UPPAAL.
// Copyright (c) 1995 - 2006, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ANTICHAIN_H
#define ANTICHAIN_H

#include <vector>
#include <map>
#include <unordered_map>
#include <stack>
#include <algorithm>



template<typename T, typename U>
class AntiChain 
{
    using sset_t    = std::vector<U>;
    using smap_t    = std::vector<std::vector<sset_t>>;
    
    smap_t map;
    
    struct node_t {
        U key;
        std::vector<node_t*> children;
    };
    
//    std::unordered_map<T, std::vector<node_t*>> nmap;
    
    public:
        AntiChain(){};
        
        void clear()
        {
            map.clear();
        }
        
        bool subsumed(T& el, sset_t& set)
        {
            bool exists = false;
            if(map.size() > (size_t)el)
            {
                for(auto& s : map[el])
                {
                    if(std::includes(set.begin(), set.end(), s.begin(), s.end()))
                    {
                        /*std::cout << "SUBSUMBED BY ";
                        for(auto& e : s) std::cout << e << ",";
                        std::cout << std::endl;*/
                        exists = true;
                        break;
                    }
                }
            }
            
            
            /*{
                auto it = nmap.find(el);
                if(it != nmap.end() && it->first == el)
                {
                    assert(it->second != NULL);
                    stack<node_t*> waiting;
                    for(auto n : it->second) waiting.push(n);
                    while(!waiting.empty())
                    {
                        node_t* n = waiting.top();
                        waiting.pop();
                        auto sit = std::lower_bound(set.begin(), set.end(), n->key);
                        if(sit != set.end() && *sit == n->key)
                        {
                            if(n->children.size() == 0)
                            {
                                assert(exists);
                                return true;
                            }
                            else
                            {
                                for(node_t* c : n->children) waiting.push(c);
                            }
                        }
                    }
                }
            }*/
            
            return exists;
        }
        
        bool insert(T& el, sset_t& set)
        {
            bool inserted = false;
            if(map.size() <= (size_t)el) map.resize(el + 1);
/*            std::cout << "ANTI (" << (size_t)el << ") -> ";
            for(auto& e : set) std::cout << e << ",";
            std::cout << std::endl;*/
            if(!subsumed(el, set))
            {
                auto& chains = map[el];
                for(int i = chains.size() - 1; i >= 0; --i)
                {
                    if(std::includes(chains[i].begin(), chains[i].end(), set.begin(), set.end()))
                    {
                        chains.erase(chains.begin() + i);
                    }
                }
                chains.push_back(set);                
                inserted = true;
            }
            else
            {
                inserted = false;
            }
            
            /*if(inserted) {
                auto it = nmap.find(el);
                node_t* from = NULL;
                size_t index = 0;
                if(it != nmap.end() && it->first == el)
                {
                    std
                    it->second;
                    for(; index < set.size(); ++index)
                    {
                        if(from->key)
                    }
                }
                else
                {
                    from = new node_t;
                    from->key = set[0];
                    nmap[el] = from;
                    ++index;
                }
                
                for(; index < set.size(); ++index)
                {
                    next = new node_t;                    
                    next->key = set[index];
                    from->children.push_back(next);
                    from = next;
                }
            }*/
            
            return inserted;
        }  
};


#endif /* ANTICHAIN_H */

