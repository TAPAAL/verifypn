/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <algorithm>

#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reducer.h"
#include "PetriEngine/PQL/Expressions.h"

using namespace std;

namespace PetriEngine {

    PetriNetBuilder::PetriNetBuilder() : AbstractPetriNetBuilder(), 
    reducer(this){
    }
    PetriNetBuilder::PetriNetBuilder(const PetriNetBuilder& other)
    : _placenames(other._placenames), _transitionnames(other._transitionnames),
       _transitions(other._transitions), _places(other._places), 
       initialMarking(other.initialMarking), reducer(this)
    {

    }

    void PetriNetBuilder::addPlace(const string &name, int tokens, double, double) {
        if(_placenames.count(name) == 0)
        {
            uint32_t next = _placenames.size();
            _places.emplace_back();
            _placenames[name] = next;
        }
        
        uint32_t id = _placenames[name];
        
        while(initialMarking.size() <= id) initialMarking.emplace_back();
        initialMarking[id] = tokens;
        
    }

    void PetriNetBuilder::addTransition(const string &name,
            double, double) {
        if(_transitionnames.count(name) == 0)
        {
            uint32_t next = _transitionnames.size();
            _transitions.emplace_back();
            _transitionnames[name] = next;
        }
    }

    void PetriNetBuilder::addInputArc(const string &place, const string &transition, bool inhibitor, int weight) {
        if(_transitionnames.count(transition) == 0)
        {
            addTransition(transition,0.0,0.0);
        }
        if(_placenames.count(place) == 0)
        {
            addPlace(place,0,0,0);
        }
        uint32_t p = _placenames[place];
        uint32_t t = _transitionnames[transition];

        Arc arc;
        arc.place = p;
        arc.weight = weight;
        arc.skip = false;
        arc.inhib = inhibitor;
        assert(t < _transitions.size());
        assert(p < _places.size());
        _transitions[t].pre.push_back(arc);
        _transitions[t].inhib |= inhibitor;
        _places[p].consumers.push_back(t);
        _places[p].inhib |= inhibitor;
    }

    void PetriNetBuilder::addOutputArc(const string &transition, const string &place, int weight) {
        if(_transitionnames.count(transition) == 0)
        {
            addTransition(transition,0,0);
        }
        if(_placenames.count(place) == 0)
        {
            addPlace(place,0,0,0);
        }
        uint32_t p = _placenames[place];
        uint32_t t = _transitionnames[transition];

        assert(t < _transitions.size());
        assert(p < _places.size());
        
        Arc arc;
        arc.place = p;
        arc.weight = weight;
        arc.skip = false;
        _transitions[t].post.push_back(arc);
        _places[p].producers.push_back(t);
    }

    uint32_t PetriNetBuilder::nextPlaceId(std::vector<uint32_t>& counts, std::vector<uint32_t>& pcounts, std::vector<uint32_t>& ids, bool reorder)
    {
        uint32_t cand = std::numeric_limits<uint32_t>::max();
        uint32_t cnt =  std::numeric_limits<uint32_t>::max();
        for(uint32_t i = 0; i < _places.size(); ++i)
        {
            uint32_t nnum = (pcounts[i] == 0 ? 0 : (counts[0] == 0 ? 0 : std::max(counts[i], pcounts[i])));
            if( ids[i] == std::numeric_limits<uint32_t>::max() &&
                nnum < cnt &&
                !_places[i].skip)
            {
                if(!reorder) return i;
                cand = i;
                cnt = nnum;
            }
        }        
        return cand;
    }
    
    PetriNet* PetriNetBuilder::makePetriNet(bool reorder) {
        
        /*
         * The basic idea is to construct three arrays, the first array, 
         * _invariants points to "arcs" - they are triplets (weight, place, inhibitor)
         * _transitions are pairs, (input, output) are indexes in the _invariants array
         * _placeToPtrs is an indirection going from a place-index to the FIRST transition
         *              with a non-inhibitor arc consuming from the given place.
         * 
         * For all the indexes and indirections, notice that we only track the 
         * beginning. We can naturally use the "next" value as the end. eg. the 
         * inputs of a transition are between "input" and "output". The outputs 
         * are between "output" and the "input" of the next transition.
         * 
         * This allows us to quickly skip a lot of checks when generating successors
         * Beware that currently "orphans" and "inhibitor orphans" are special-cases
         * and currently handled as "consuming" from place id=0.
         * 
         * If anybody wants to spend time on it, this is the first step towards
         * a decision-tree like construction, possibly improving successor generation. 
         */
        
        uint32_t nplaces = _places.size() - reducer.RemovedPlaces();
        uint32_t ntrans = _transitions.size() - reducer.RemovedTransitions();
        
        std::vector<uint32_t> place_cons_count = std::vector<uint32_t>(_places.size());
        std::vector<uint32_t> place_prod_count = std::vector<uint32_t>(_places.size());
        std::vector<uint32_t> place_idmap = std::vector<uint32_t>(_places.size());
        std::vector<uint32_t> trans_idmap = std::vector<uint32_t>(_transitions.size());
        
        uint32_t invariants = 0;
        
        for(uint32_t i = 0; i < _places.size(); ++i)
        {
            place_idmap[i] = std::numeric_limits<uint32_t>::max();
            if(!_places[i].skip)
            {
                place_cons_count[i] = _places[i].consumers.size();
                place_prod_count[i] = _places[i].producers.size();
                invariants += _places[i].consumers.size() + _places[i].producers.size();
            }
        }

        for(uint32_t i = 0; i < _transitions.size(); ++i)
        {
            trans_idmap[i] = std::numeric_limits<uint32_t>::max();
        }
        
        PetriNet* net = new PetriNet(ntrans, invariants, nplaces);
        
        uint32_t next = nextPlaceId(place_cons_count, place_prod_count, place_idmap, reorder);
        uint32_t free = 0;
        uint32_t freeinv = 0;
        uint32_t freetrans = 0;
        
        // first handle orphans
        
        if(place_idmap.size() > next) place_idmap[next] = free;
        net->_placeToPtrs[free] = freetrans;
        for(size_t t = 0; t < _transitions.size(); ++t)
        {
            Transition& trans = _transitions[t]; 
            if (std::all_of(trans.pre.begin(), trans.pre.end(), [](Arc& a){return a.inhib;}))
            {
                // ALL have to be inhibitor, if any. Otherwise not orphan
                
                if(trans.skip) continue;
                net->_transitions[freetrans].inputs = freeinv;
                
                // add inhibitors
                for(auto pre : trans.pre)
                {
                    Invariant& iv = net->_invariants[freeinv];
                    iv.place = pre.place;
                    iv.tokens = pre.weight;
                    iv.inhibitor = pre.inhib;
                    assert(pre.inhib);
                    assert(place_cons_count[pre.place] > 0);
                    --place_cons_count[pre.place];
                    ++freeinv;
                }
                
                net->_transitions[freetrans].outputs = freeinv;
                
                for(auto post : trans.post)
                {
                    assert(freeinv < net->_ninvariants);
                    net->_invariants[freeinv].place = post.place;
                    net->_invariants[freeinv].tokens = post.weight;                    
                    ++freeinv;
                }
                
                trans_idmap[t] = freetrans;
                
                ++freetrans;
            }
        }
        
        
        bool first = true;
        while(next != std::numeric_limits<uint32_t>::max())
        {
            if(first) // already set for first iteration to handle orphans
            {
                first = false;
            }
            else
            {
                place_idmap[next] = free;
                net->_placeToPtrs[free] = freetrans;
            }
            
            for(auto t : _places[next].consumers)
            {
                Transition& trans = _transitions[t]; 
                if(trans.skip) continue;

                net->_transitions[freetrans].inputs = freeinv;

                // check first, we are going to change state later, but we can 
                // break here, so no statechange inside loop!
                bool ok = true;
                bool all_inhib = true;
                uint32_t cnt = 0;
                for(const Arc& pre : trans.pre)
                {
                    all_inhib &= pre.inhib;
                    
                    // if transition belongs to previous place
                    if(     (!pre.inhib && place_idmap[pre.place] < free) || 
                            freeinv + cnt >= net->_ninvariants)
                    {
                        ok = false;
                        break;
                    }  
                    
                    // or arc from place is an inhibitor
                    if(pre.place == next &&  pre.inhib)
                    {
                        ok = false;
                        break;
                    }
                    ++cnt;
                }

                // skip for now, either T-a->P is inhibitor, or was allready added for other P'
                // or all a's are inhibitors. 
                if(!ok || all_inhib) continue; 
                
                trans_idmap[t] = freeinv;
                
                // everything is good, change state!.
                for(auto pre : trans.pre)
                {
                    Invariant& iv = net->_invariants[freeinv];
                    iv.place = pre.place;
                    iv.tokens = pre.weight;
                    iv.inhibitor = pre.inhib;
                    ++freeinv;
                    assert(place_cons_count[pre.place] > 0);
                    --place_cons_count[pre.place];
                }
                
                net->_transitions[freetrans].outputs = freeinv;
                for(auto post : trans.post)
                {
                    assert(freeinv < net->_ninvariants);
                    auto& post_inv = net->_invariants[freeinv];
                    post_inv.place = post.place;
                    post_inv.tokens = post.weight;    
                    --place_prod_count[post.place];
                    ++freeinv;
                }
                
                trans_idmap[t] = freetrans;
                
                ++freetrans;
                assert(freeinv <= invariants);
            }
            ++free;
            next = nextPlaceId(place_cons_count, place_prod_count, place_idmap, reorder);
        }
        
        // Reindex for great justice!
        for(uint32_t i = 0; i < freeinv; i++)
        {
            net->_invariants[i].place = place_idmap[net->_invariants[i].place];
            assert(net->_invariants[i].place < nplaces);
            assert(net->_invariants[i].tokens > 0);
        }
        
//        std::cout << "init" << std::endl;
        for(uint32_t i = 0; i < _places.size(); ++i)
        {
            if(place_idmap[i] != std::numeric_limits<uint32_t>::max())
            {
                net->_initialMarking[place_idmap[i]] = initialMarking[i];
//                std::cout << place_idmap[i] << " : " << initialMarking[i] << std::endl;
            }
        }
        
        // reindex place-names

        net->_placenames.resize(_placenames.size());
        int rindex = _placenames.size() - 1;
        for(auto& i : _placenames)
        {
            i.second = place_idmap[i.second];
            if(i.second != std::numeric_limits<uint32_t>::max())
            {
                net->_placenames[i.second] = i.first;
                assert(_placenames[net->_placenames[i.second]] == i.second);
            }
            else
            {
                net->_placenames[rindex] = i.first;
                --rindex;
            }
        }

        net->_transitionnames.resize(_transitionnames.size());
        int trindex = _transitionnames.size() - 1;
        for(auto& i : _transitionnames)
        {
            i.second = trans_idmap[i.second];
            if(i.second != std::numeric_limits<uint32_t>::max())
            {
                net->_transitionnames[i.second] = i.first;
            }
            else
            {
                net->_transitionnames[trindex] = i.first;
                --trindex;
            }
        }
        net->sort();
        
        for(size_t t = 0; t < net->numberOfTransitions(); ++t)
        {
            {
                auto tiv = std::make_pair(&net->_invariants[net->_transitions[t].inputs], &net->_invariants[net->_transitions[t].outputs]);
                for(; tiv.first != tiv.second; ++tiv.first)
                {
                    tiv.first->direction = tiv.first->inhibitor ? 0 : -1;
                    bool found = false;
                    auto tov = std::make_pair(&net->_invariants[net->_transitions[t].outputs], &net->_invariants[net->_transitions[t + 1].inputs]);                    
                    for(; tov.first != tov.second; ++tov.first)
                    {
                        if(tov.first->place == tiv.first->place)
                        {
                            found = true;
                            if(tiv.first->inhibitor)                        tiv.first->direction = tov.first->direction = 1;
                            else if(tiv.first->tokens < tov.first->tokens)  tiv.first->direction = tov.first->direction = 1;
                            else if(tiv.first->tokens == tov.first->tokens) tiv.first->direction = tov.first->direction = 0;
                            else if(tiv.first->tokens > tov.first->tokens)  tiv.first->direction = tov.first->direction = -1;
                            break;
                        }
                    }
                    if(!found) assert(tiv.first->direction < 0 || tiv.first->inhibitor);
                }
            }
            {
                auto tiv = std::make_pair(&net->_invariants[net->_transitions[t].outputs], &net->_invariants[net->_transitions[t + 1].inputs]);
                for(; tiv.first != tiv.second; ++tiv.first)
                {
                    tiv.first->direction = 1;
                    bool found = false;
                    auto tov = std::make_pair(&net->_invariants[net->_transitions[t].inputs], &net->_invariants[net->_transitions[t].outputs]);
                    for(; tov.first != tov.second; ++tov.first)
                    {
                        found = true;
                        if(tov.first->place == tiv.first->place)
                        {
                            if     (tov.first->inhibitor)                   tiv.first->direction = tov.first->direction = 1;
                            else if(tiv.first->tokens > tov.first->tokens)  tiv.first->direction = tov.first->direction = 1;
                            else if(tiv.first->tokens == tov.first->tokens) tiv.first->direction = tov.first->direction = 0;
                            else if(tiv.first->tokens < tov.first->tokens)  tiv.first->direction = tov.first->direction = -1;
                            break;
                        }
                    }
                    if(!found) assert(tiv.first->direction > 0);
                }
            }
        }
        
        return net;
    }
    
    void PetriNetBuilder::sort()
    {
        for(Place& p : _places)
        {
            std::sort(p.consumers.begin(), p.consumers.end());
            std::sort(p.producers.begin(), p.producers.end());
        }
        
        for(Transition& t : _transitions)
        {
            std::sort(t.pre.begin(), t.pre.end());
            std::sort(t.post.begin(), t.post.end());
        }
    }
    
    void PetriNetBuilder::reduce(   std::vector<std::shared_ptr<PQL::Condition> >& queries,
                                    std::vector<Reachability::ResultPrinter::Result>& results, 
                                    int reductiontype, bool reconstructTrace, const PetriNet* net, int timeout, std::vector<uint32_t>& reductions)
    {
        QueryPlaceAnalysisContext placecontext(getPlaceNames(), getTransitionNames(), net);
        bool all_reach = true;
        bool remove_loops = true;
        bool contains_next = false;
        for(uint32_t i = 0; i < queries.size(); ++i)
        {
            if(results[i] == Reachability::ResultPrinter::Unknown ||
               results[i] == Reachability::ResultPrinter::CTL )
            {
                queries[i]->analyze(placecontext);
                all_reach &= (results[i] != Reachability::ResultPrinter::CTL);
                remove_loops &= !queries[i]->isLoopSensitive();
                // There is a deadlock somewhere, if it is not alone, we cannot reduce.
                // this has similar problems as nested next.                        
                contains_next |= queries[i]->containsNext() || queries[i]->nestedDeadlock();                        
            }
        }
        reducer.Reduce(placecontext, reductiontype, reconstructTrace, timeout, remove_loops, all_reach, contains_next, reductions);
    }


} // PetriEngine
