/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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

#include "PetriNetBuilder.h"
#include "PetriNet.h"
#include "PQL/PQLParser.h"
#include "PQL/PQL.h"
#include "PQL/Contexts.h"
#include "Reducer.h"

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

    void PetriNetBuilder::addInputArc(const string &place, const string &transition, int weight) {
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
        assert(t < _transitions.size());
        assert(p < _places.size());
        _transitions[t].pre.push_back(arc);
        _places[p].input.push_back(t);
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
        _places[p].output.push_back(t);
    }

    uint32_t PetriNetBuilder::nextPlaceId(std::vector<uint32_t>& counts, std::vector<uint32_t>& ids, bool reorder)
    {
        uint32_t cand = std::numeric_limits<uint32_t>::max();
        uint32_t cnt =  std::numeric_limits<uint32_t>::max();
        for(uint32_t i = 0; i < _places.size(); ++i)
        {
            if( ids[i] == std::numeric_limits<uint32_t>::max() &&
                counts[i] < cnt &&
                !_places[i].skip)
            {
                if(!reorder) return i;
                cand = i;
            }
        }        
        return cand;
    }
    
    PetriNet* PetriNetBuilder::makePetriNet(bool reorder) {
        
        uint32_t nplaces = _places.size() - reducer.RemovedPlaces();
        uint32_t ntrans = _transitions.size() - reducer.RemovedTransitions();
        
        std::vector<uint32_t> place_cons_count = std::vector<uint32_t>(_places.size());
        std::vector<uint32_t> place_idmap = std::vector<uint32_t>(_places.size());
        std::vector<uint32_t> trans_idmap = std::vector<uint32_t>(_transitions.size());
        
        uint32_t invariants = 0;
        
        for(uint32_t i = 0; i < _places.size(); ++i)
        {
            place_idmap[i] = std::numeric_limits<uint32_t>::max();
            if(!_places[i].skip)
            {
                place_cons_count[i] = _places[i].input.size();
                invariants += _places[i].input.size() + _places[i].output.size();
            }
        }

        for(uint32_t i = 0; i < _transitions.size(); ++i)
        {
            trans_idmap[i] = std::numeric_limits<uint32_t>::max();
        }
        
        PetriNet* net = new PetriNet(ntrans, invariants, nplaces);
        
        uint32_t next = nextPlaceId(place_cons_count, place_idmap, reorder);
        uint32_t free = 0;
        uint32_t freeinv = 0;
        uint32_t freetrans = 0;
        while(next != std::numeric_limits<uint32_t>::max())
        {
            place_idmap[next] = free;
            net->_placeToPtrs[free] = freetrans;
            for(auto t : _places[next].input)
            {
                Transition& trans = _transitions[t]; 
                if(trans.skip) continue;

                net->_transitions[freetrans].inputs = freeinv;

                // check first, we are going to change state later, but we can 
                // break here, so not statechange inside loop!
                bool ok = true;
                uint32_t cnt = 0;
                for(auto pre : trans.pre)
                {
                    if(     place_idmap[pre.place] < free || 
                            freeinv + cnt >= net->_ninvariants)
                    {
                        ok = false;
                        break;
                    }       
                    ++cnt;
                }

                if(!ok) continue;
                
                trans_idmap[t] = freeinv;
                
                // everything is good, change state!.
                for(auto pre : trans.pre)
                {
                    net->_invariants[freeinv].place = pre.place;
                    net->_invariants[freeinv].tokens = pre.weight;
                    ++freeinv;
                    assert(place_cons_count[pre.place] > 0);
                    --place_cons_count[pre.place];
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
                assert(freeinv <= invariants);
            }
            ++free;
            next = nextPlaceId(place_cons_count, place_idmap, reorder);
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
        for(auto& i : _placenames)
        {
            i.second = place_idmap[i.second];
            if(i.second != std::numeric_limits<uint32_t>::max())
            {
                net->_placenames[i.second] = i.first;
                assert(_placenames[net->_placenames[i.second]] == i.second);
            }
        }

        net->_transitionnames.resize(_transitionnames.size());
        for(auto& i : _transitionnames)
        {
            i.second = trans_idmap[i.second];
            if(i.second != std::numeric_limits<uint32_t>::max())
            {
                net->_transitionnames[i.second] = i.first;
            }
        }
        
        return net;
    }
    
    void PetriNetBuilder::reduce(   std::vector<std::shared_ptr<PQL::Condition> >& queries,
                                    std::vector<Reachability::ResultPrinter::Result>& results, 
                                    int reductiontype)
    {
        QueryPlaceAnalysisContext placecontext(getPlaceNames());
        for(uint32_t i = 0; i < queries.size(); ++i)
        {
            if(results[i] == Reachability::ResultPrinter::Unknown)
            {
                queries[i]->analyze(placecontext);        
            }
        }
        reducer.Reduce(placecontext.getQueryPlaceCount(), reductiontype);
    }


} // PetriEngine
