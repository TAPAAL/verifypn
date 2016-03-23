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
#include "PetriNet.h"
#include "PQL/PQL.h"
#include "PQL/Contexts.h"
#include "Structures/State.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;

namespace PetriEngine {

    PetriNet::PetriNet(uint32_t trans, uint32_t invariants, uint32_t places)
    : _ninvariants(invariants), _ntransitions(trans), _nplaces(places),
            _transitions(_ntransitions+1),
            _invariants(_ninvariants),            
            _placeToPtrs(_nplaces+1) {

        // to avoid special cases
        _transitions[_ntransitions].inputs = _ninvariants;
        _transitions[_ntransitions].outputs = _ninvariants;
        _placeToPtrs[_nplaces] = _ntransitions;
        _initialMarking = new MarkVal[_nplaces];
    }

    PetriNet::~PetriNet() {
        delete[] _initialMarking;
    }

    int PetriNet::inArc(unsigned int place, unsigned int transition) const
    {
        uint32_t min = _placeToPtrs[place];
        uint32_t max = _placeToPtrs[place+1];
        if(transition < min || transition >= max) return 0;
        
        uint32_t imin = _transitions[transition].inputs;
        uint32_t imax = _transitions[transition].outputs;
        for(;imin < imax; ++imin)
        {
            if(_invariants[imin].place == place) return _invariants[imin].tokens;
        }
        return 0;
    }
    int PetriNet::outArc(unsigned int transition, unsigned int place) const
    {
        uint32_t min = _placeToPtrs[place];
        uint32_t max = _placeToPtrs[place+1];
        if(transition < min || transition >= max) return 0;
        
        uint32_t imin = _transitions[transition].outputs;
        uint32_t imax = _transitions[transition+1].inputs;
        for(;imin < imax; ++imin)
        {
            if(_invariants[imin].place == place) return _invariants[imin].tokens;
        }
        return 0;   
    }
    
    bool PetriNet::deadlocked(const MarkVal* m) const {
        
        //Check that we can take from the marking
        for (size_t i = 0; i < _nplaces; i++) {
            if(m[i] > 0)
            {
                uint32_t first = _placeToPtrs[i];
                uint32_t last = _placeToPtrs[i+1];
                for(;first != last; ++first)
                {
                    const TransPtr& ptr = _transitions[first];
                    uint32_t finv = ptr.inputs;
                    uint32_t linv = ptr.outputs;
                    for(;finv != linv; ++finv)
                    {
                        if(m[_invariants[finv].place] < _invariants[finv].tokens)
                        {
                            break;
                        }
                        return false;
                    }
                }
            }
        }

        return true;
    }    
    
    void PetriNet::reset(const Structures::State* p)
    {
        parent = p;
        _suc_pcounter = 0;
        _suc_tcounter = std::numeric_limits<uint32_t>::max();       
//        std::cout << "RESET:" << std::endl;
//        print(p->marking());
    }
    
    bool PetriNet::next(Structures::State* write)
    {
//        std::cout << "BEGIN "<< std::endl;
        for (; _suc_pcounter < _nplaces; _suc_pcounter++) {
            if(this->parent->marking()[_suc_pcounter] > 0)
            {
                if(_suc_tcounter == std::numeric_limits<uint32_t>::max())
                {
                    _suc_tcounter = _placeToPtrs[_suc_pcounter];
                }
                uint32_t last = _placeToPtrs[_suc_pcounter+1];
                for(;_suc_tcounter != last; ++_suc_tcounter)
                {
                    memcpy(write->marking(), parent->marking(), _nplaces*sizeof(MarkVal));
                    
                    TransPtr& ptr = _transitions[_suc_tcounter];
                    uint32_t finv = ptr.inputs;
                    uint32_t linv = ptr.outputs;
                    bool ok = true;
                    for(;finv < linv; ++finv)
                    {
                        if(this->parent->marking()[_invariants[finv].place] < _invariants[finv].tokens)
                        {
                            ok = false;
                            break;
                        }
                        write->marking()[_invariants[finv].place] -= _invariants[finv].tokens;
                    }
                    if(!ok) continue;
                    // else fire
                    finv = linv;
                    linv = _transitions[_suc_tcounter+1].inputs;
                    for(;finv < linv; ++finv)
                    {
                        write->marking()[_invariants[finv].place] += _invariants[finv].tokens;
                    }
                    ++_suc_tcounter;
//                    std::cout << "NEXT" << std::endl;
//                    print(write->marking());
                    return true;
                }
                _suc_tcounter = std::numeric_limits<uint32_t>::max();
            }
            _suc_tcounter = std::numeric_limits<uint32_t>::max();
        }
        return false;
    }  

    MarkVal* PetriNet::makeInitialMarking()
    {
        MarkVal* marking = new MarkVal[_nplaces];
        memcpy(marking, _initialMarking, sizeof(MarkVal)*_nplaces);
        return marking;
    }
    
} // PetriEngine
